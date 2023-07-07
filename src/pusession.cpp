#include <string>

#include "BVCSP.h"
#include "config.h"

#include "pusession.h"
#include "gps.h"
#include "tsp.h"
#include "media.h"

CPUSession::CPUSession(const char* ID, int relogintime)
    : CPUSessionBase(ID)
    , m_lastReloginTime(0)
    , m_pGPS(NULL)
    , m_pMedia(NULL)
    , m_pTSP(NULL)
    , m_bNeedOnline(false)
    , m_relogintime(relogintime)
{
}
CPUSession::~CPUSession()
{
}

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
        if (m_bNeedOnline && m_relogintime > 0 && !BLogining() && m_lastReloginTime > 0 && nowTime - m_lastReloginTime > m_relogintime) {
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
bool CPUSession::BFirst() {
    return m_lastReloginTime == 0 || !m_bNeedOnline;
}

int CPUSession::Login(int through, int lat, int lng)
{
    m_bNeedOnline = true;
    m_lastReloginTime = time(NULL);
    return CPUSessionBase::Login(through, lat, lng);
}

int CPUSession::Logout()
{
    m_bNeedOnline = false;
    m_lastReloginTime = 0;
    return CPUSessionBase::Logout();
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

