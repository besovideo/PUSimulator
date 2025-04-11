#include <string>

#include "BVCSP.h"
#include "utils.h"
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
    , m_bSubGPS(false)
    , m_iInterval(5)
    , m_lastgpstime(0)
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
    if (m_pGPS) {
        const BVCU_PUCFG_GPSData* newGps = m_pGPS->UpdateData();
        if (m_bSubGPS) {
            time_t now = time(NULL);
            int dely = now - m_lastgpstime;
            if (dely >= m_iInterval)
            {
                m_lastgpstime = now;
                if (newGps == NULL)
                    newGps = m_pGPS->OnGetGPSData();

                BVCU_PUCFG_GPSDatas gpss;
                memset(&gpss, 0, sizeof(gpss));
                gpss.iCount = 1;
                gpss.pGPSDatas = (BVCU_PUCFG_GPSData*)newGps;
                SendNotify(BVCU_SUBMETHOD_SUBSCRIBE_GPS, &gpss);
            }
        }
    }
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
    m_bSubGPS = false;
    m_lastgpstime = 0;
    return CPUSessionBase::Logout();
}

// 重构设置设备信息接口，保存平台下发的配置信息，回复0：成功。
BVCU_Result CPUSession::OnSetInfo(const char* name, int lat, int lng)
{
    /*PUConfig puconfig;
    LoadConfig(&puconfig);
    strncpy_s(puconfig.Name, sizeof(puconfig.Name), name, _TRUNCATE);
    puconfig.lat = lat;
    puconfig.lng = lng;
    SetConfig(&puconfig);*/
    SetName(name);
    return BVCU_RESULT_S_OK;
}
// 收到服务器(取消)订阅GPS
BVCU_Result CPUSession::OnSubscribeGPS(int bStart, int iInterval)
{
    m_bSubGPS = bStart;
    m_iInterval = iInterval;
    if (m_iInterval < 3) {
        m_iInterval = 3;
        if (m_pGPS != NULL) {
            const BVCU_PUCFG_GPSParam* param = m_pGPS->OnGetGPSParam();
            m_iInterval = param->iReportInterval;
        }
    }
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
        printf("================== login faild: %d  %s %d ==================\n", iResult, m_sesParam.szServerAddr, m_sesParam.iServerPort);
    }
}
void CPUSession::OnOfflineEvent(BVCU_Result iResult)
{
    m_bSubGPS = false;
    m_lastgpstime = 0;
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

