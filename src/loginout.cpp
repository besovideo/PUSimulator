#include <string>

#include "BVCSP.h"
#include "config.h"

#include "base/session.h"
#include "gps.h"
#include "tsp.h"
#include "media.h"

class CPUSession : public CPUSessionBase
{  // 设备对象
protected:
    CGPSChannel* m_pGPS;     // GPS通道对象。
    CTSPChannel* m_pTSP;     // 串口通道对象。
    CMediaChannel* m_pMedia; // 媒体通道对象。
    time_t m_lastReloginTime;// 最后一次尝试登录时间
public:
    CPUSession(const char* ID);
    ~CPUSession();
    void RegisterChannel();
    void HandleEvent(time_t nowTime);

private:
    virtual BVCU_Result OnSetInfo(const char* name, int lat, int lng);
    virtual void OnLoginEvent(BVCU_Result iResult);
    virtual void OnOfflineEvent(BVCU_Result iResult);
    virtual void OnCommandReply(BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam);
};

CPUSession::CPUSession(const char* ID)
    : CPUSessionBase(ID)
    , m_lastReloginTime(0)
    , m_pGPS(NULL)
    , m_pMedia(NULL)
    , m_pTSP(NULL)
{
}
CPUSession::~CPUSession()
{
}

static CPUSession* pSession[1024] = { 0,0,0,0 };  // 全局Session
static int relogintime = 0; // 是否断线重连，0：否，其他值：间隔时间，秒
static bool needOnline = false; // 是否需要在线，调用login和logout时修改

void CPUSession::RegisterChannel() {
    // 注册通道
    m_pGPS = new CGPSChannel();
    AddGPSChannel(m_pGPS);
    m_pTSP = new CTSPChannel();
    AddTSPChannel(m_pTSP);
    m_pMedia = new CMediaChannel();
    AddAVChannel(m_pMedia);
    SetPTZCount(1);
}
void CPUSession::HandleEvent(time_t nowTime)
{
    if (!BOnline()) {
        if (needOnline && relogintime > 0 && !BLogining() && (m_lastReloginTime == 0 || nowTime - m_lastReloginTime > relogintime)) {
            m_lastReloginTime = nowTime;
            Login(BVCU_PU_ONLINE_THROUGH_ETHERNET, 200 * 10000000, 200 * 10000000);
        }
        return;
    }
    if (m_pGPS)
        m_pGPS->UpdateData();
    if (m_pTSP)
        m_pTSP->SendData();
    if (m_pMedia)
        m_pMedia->SendData();
}

// 重构设置设备信息接口，保存平台下发的配置信息，回复0：成功。
BVCU_Result CPUSession::OnSetInfo(const char* name, int lat, int lng)
{
    PUConfig puconfig;
    LoadConfig(&puconfig);
    strncpy_s(puconfig.Name, sizeof(puconfig.Name), name, _TRUNCATE);
    puconfig.lat = lat;
    puconfig.lng = lng;
    SetConfig(&puconfig);
    SetName(name);
    return BVCU_RESULT_S_OK;
}
// 在线状态变化通知，登录/退出 服务器
void CPUSession::OnLoginEvent(BVCU_Result iResult)
{
    m_lastReloginTime = time(NULL);
    if (BOnline())
    {
        printf("============================ pu online ==============================\n");
        // 登录成功后，获取token和http api地址
        SendCommand(BVCU_METHOD_QUERY, BVCU_SUBMETHOD_CMS_HTTPAPI, NULL, NULL, NULL);
    }
    else
    {
        printf("======================== login faild: %d ==========================\n", iResult);
    }
}
void CPUSession::OnOfflineEvent(BVCU_Result iResult)
{
    printf("======================== server offline: %d ==========================\n", iResult);
}
void CPUSession::OnCommandReply(BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam)
{
    printf("======================== on command reply: %d ==========================\n", pParam->iResult);
    if (BVCU_Result_SUCCEEDED(pParam->iResult)) {
        if (pCommand->iSubMethod == BVCU_SUBMETHOD_CMS_HTTPAPI && pParam->stContent.pData != NULL) {
            BVCU_CMSCFG_HttpApi* pHttpApi = (BVCU_CMSCFG_HttpApi*)pParam->stContent.pData;
            printf("=========================== http api ===========================\n");
            printf("====http:  %s\n", pHttpApi->szHttpUrl);
            printf("====https: %s\n", pHttpApi->szHttpsUrl);
            printf("====token: %s\n", pHttpApi->szToken);
        }
    }
}

// 登录服务器。从配置文件中读取设备信息，和服务器信息；请提前设置好。
int Login(bool autoOption)
{
    needOnline = true;
    if (autoOption && pSession[0] != 0)
    {  // 自动操作不能重复登录。
        return 0;
    }
    if (pSession[0] == 0)
    {
        PUConfig puconfig;
        LoadConfig(&puconfig);
        int startID = 1;
        char puid[32];
        char puName[32];
        if (sscanf_s(puconfig.ID, "PU_%X", &startID) != 1)
            startID = 0x55AA0000;
        relogintime = puconfig.relogin;
        for (int i = 0; i < puconfig.PUCount && i < sizeof(pSession) / sizeof(pSession[0]); i++)
        {
            sprintf_s(puid, "PU_%X", startID++);
            pSession[i] = new CPUSession(puid);
            if (pSession[i])
            {
                // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
                // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
                // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
                sprintf_s(puName, "%s-%d", puconfig.Name, i + 1);
                pSession[i]->SetName(puName);
                pSession[i]->SetServer(puconfig.serverIP, puconfig.serverPort, puconfig.protoType, 60 * 1000, 4 * 1000);
                pSession[i]->SetDevicePosition(puconfig.lat, puconfig.lng);
                pSession[i]->RegisterChannel();
            }
        }
    }
    if (pSession[0] == 0)
        return -1;
    // 上线服务器，lat,lng 应该改为当前设备定位位置。

    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0)
        {
            pSession[i]->Login(BVCU_PU_ONLINE_THROUGH_ETHERNET, 200 * 10000000, 200 * 10000000);
        }
    }
    return 0;
}

// 退出登录
int Logout()
{
    needOnline = false;
    bool bFind = false;
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0)
        {
            pSession[i]->Logout();
            bFind = true;
        }
    }
    if (bFind)
        return 0;
    return -1;
}

// 发送报警信息
int SendAlarm()
{
    bool bFind = false;
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0)
        {
            pSession[i]->SendAlarm(
                BVCU_EVENT_TYPE_ALERTIN,
                0,
                0,
                0,
                "test"
            );
            bFind = true;
        }
    }
    if (bFind)
        return 0;
    return -1;
}

// 发送命令
int SendCommand()
{
    bool bFind = false;
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0)
        {
            pSession[i]->SendCommand(BVCU_METHOD_QUERY, BVCU_SUBMETHOD_CMS_HTTPAPI, NULL, NULL, NULL);
            bFind = true;
        }
    }
    if (bFind)
        return 0;
    return -1;
}
void HandleEvent()
{
    time_t nowTime = time(NULL);
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0)
            pSession[i]->HandleEvent(nowTime);
    }
}
