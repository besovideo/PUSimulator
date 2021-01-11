#include <stdio.h>
#include <string.h>
#include "session.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

CPUSessionBase::CPUSessionBase(const char* ID)
{
    memset(&m_deviceInfo, 0x00, sizeof(m_deviceInfo));
    strncpy_s(m_deviceInfo.szID, ID, _TRUNCATE);
    strncpy_s(m_deviceInfo.szManufacturer, "simulator", _TRUNCATE);
    strncpy_s(m_deviceInfo.szProductName, "PUSimulator G1B2", _TRUNCATE);
    strncpy_s(m_deviceInfo.szSoftwareVersion, "0.0.1", _TRUNCATE);
    strncpy_s(m_deviceInfo.szHardwareVersion, "0.0.1", _TRUNCATE);
    // 默认设置无效的经纬度地址。
    m_deviceInfo.iLatitude = 200 * 10000000;
    m_deviceInfo.iLongitude = 200 * 10000000;
    // 通道信息
    memset(m_avChannels, 0x00, sizeof(m_avChannels));
    memset(m_GPSChannels, 0x00, sizeof(m_GPSChannels));
    memset(m_TSPChannels, 0x00, sizeof(m_TSPChannels));
    m_session = 0;
    m_bOnline = false;
    // 登录参数
    memset(&m_sesParam, 0x00, sizeof(m_sesParam));
    m_sesParam.iSize = sizeof(m_sesParam);
    m_sesParam.iTimeOut = 8000;
    strncpy_s(m_sesParam.szClientID, ID, _TRUNCATE);
    strncpy_s(m_sesParam.szUserAgent, "PUSimulatorG1B2", _TRUNCATE);
    m_sesParam.iClientType = BVCSP_CLIENT_TYPE_PU;
    m_sesParam.OnCommand = OnCommand;
    m_sesParam.OnDialogCmd = OnDialogCmd;
    m_sesParam.OnEvent = OnSessionEvent;
    m_sesParam.OnNotify = OnNotify;
    m_sesParam.stEntityInfo.pPUInfo = &m_deviceInfo;
    // 自动获取开机时间
#ifdef _MSC_VER
    m_sesParam.stEntityInfo.iBootDuration = GetTickCount64() / 1000;
#endif
}

CPUSessionBase::~CPUSessionBase()
{
    Logout();
}

void CPUSessionBase::SetName(const char* Name)
{
    strncpy_s(m_deviceInfo.szName, Name, _TRUNCATE);
}

void CPUSessionBase::SetManufacturer(const char* Manufacturer)
{
    strncpy_s(m_deviceInfo.szManufacturer, Manufacturer, _TRUNCATE);
}

void CPUSessionBase::SetProductName(const char* ProductName)
{
    strncpy_s(m_deviceInfo.szProductName, ProductName, _TRUNCATE);
}

void CPUSessionBase::SetSoftwareVersion(const char* SoftwareVersion)
{
    strncpy_s(m_deviceInfo.szSoftwareVersion, SoftwareVersion, _TRUNCATE);
}

void CPUSessionBase::SetHardwareVersion(const char* HardwareVersion)
{
    strncpy_s(m_deviceInfo.szHardwareVersion, HardwareVersion, _TRUNCATE);
}

void CPUSessionBase::SetWIFICount(int count)
{
    m_deviceInfo.iWIFICount = count;
}

void CPUSessionBase::SetRadioCount(int count)
{
    m_deviceInfo.iRadioCount = count;
}

void CPUSessionBase::SetVideoInCount(int count)
{
    m_deviceInfo.iVideoInCount = count;
}

void CPUSessionBase::SetAudioInCount(int count)
{
    m_deviceInfo.iAudioInCount = count;
}

void CPUSessionBase::SetAudioOutCount(int count)
{
    m_deviceInfo.iAudioOutCount = count;
}

void CPUSessionBase::SetAlertInCount(int count)
{
    m_deviceInfo.iAlertInCount = count;
}

void CPUSessionBase::SetAlertOutCount(int count)
{
    m_deviceInfo.iAlertOutCount = count;
}

void CPUSessionBase::SetStorageCount(int count)
{
    m_deviceInfo.iStorageCount = count;
}

void CPUSessionBase::SetBootDuration(int duration)
{
    m_sesParam.stEntityInfo.iBootDuration = duration;
}

void CPUSessionBase::SetDevicePosition(int lat, int lng)
{
    m_deviceInfo.iLatitude = lat;
    m_deviceInfo.iLongitude = lng;
}

void CPUSessionBase::SetServer(const char* IP, int port, int proto, int timeout, int iKeepaliveInterval)
{
    strncpy_s(m_sesParam.szServerAddr, IP, _TRUNCATE);
    m_sesParam.iServerPort = port;
    m_sesParam.iCmdProtoType = proto == 0? BVCU_PROTOTYPE_UDP: BVCU_PROTOTYPE_TCP;
    m_sesParam.iTimeOut = timeout;
    m_sesParam.iKeepaliveInterval = iKeepaliveInterval;
}

int CPUSessionBase::Login(int through, int lat, int lng)
{
    Logout();
    m_sesParam.pUserData = this;
    // 通道信息
    int iChannelCount = 0;
    for (int i = 0; i < MAX_AV_CHANNEL_COUNT; ++ i)
    {
        if (m_avChannels[i] != NULL)
            ++iChannelCount;
    }
    for (int i = 0; i < MAX_GPS_CHANNEL_COUNT; ++i)
    {
        if (m_GPSChannels[i] != NULL)
            ++iChannelCount;
    }
    for (int i = 0; i < MAX_TSP_CHANNEL_COUNT; ++i)
    {
        if (m_GPSChannels[i] != NULL)
            ++iChannelCount;
    }
    if (iChannelCount > 0)
    {
    }
    else
        m_sesParam.stEntityInfo.iChannelCount = 0;
    m_sesParam.stEntityInfo.iOnlineThrough = through;
    m_sesParam.stEntityInfo.iLatitude = lat;
    m_sesParam.stEntityInfo.iLongitude = lng;
    BVCU_Result bvResult = BVCSP_Login(&m_session, &m_sesParam);
    printf("Call BVCSP_login(%s:%d %s) code:%d session:%p\n", m_sesParam.szServerAddr, m_sesParam.iServerPort
        , m_deviceInfo.szID, bvResult, m_session);
    return bvResult;
}

int CPUSessionBase::Logout()
{
    if (m_session)
    {
        BVCU_Result bvResult = BVCSP_Logout(m_session); 
        printf("Call BVCSP_Logout(%s:%d %s) code:%d session:%p\n", m_sesParam.szServerAddr, m_sesParam.iServerPort
            , m_deviceInfo.szID, bvResult, m_session);
        m_bOnline = false;
        m_session = 0;
    }
    m_bOnline = false;
    return 0;
}

void CPUSessionBase::OnSessionEvent(BVCSP_HSession hSession, int iEventCode, void* pParam)
{
    BVCU_Result iResult = (BVCU_Result)(int(pParam));
    printf("session event hSession:%p iEventCode:%d result:%d \n", hSession, iEventCode, iResult);
    BVCSP_SessionInfo sesInfo;
    if (BVCU_Result_SUCCEEDED(BVCSP_GetSessionInfo(hSession, &sesInfo)))
    {
        CPUSessionBase* pSession = (CPUSessionBase*)sesInfo.stParam.pUserData;
        if (pSession == 0 || hSession != pSession->m_session)
            return;
        if (iEventCode == BVCSP_EVENT_SESSION_OPEN)
        {
            if (BVCU_Result_SUCCEEDED(iResult))
            { // 登录成功
                printf("Login succeeded\n");
                pSession->m_bOnline = true;
            }
            else
            {
                printf("Login failed: %d\n", iResult);
                pSession->m_session = 0;
                pSession->m_bOnline = false;
            }
            pSession->OnLoginEvent(iResult);
        }
        else
        {
            printf("Offline code: %d\n", iResult);
            pSession->m_session = 0;
            pSession->m_bOnline = false;
            if (BVCU_Result_FAILED(iResult))
            {
                pSession->OnOfflineEvent(iResult);
            }
        }
    }
}
BVCU_Result CPUSessionBase::OnNotify(BVCSP_HSession hSession, BVCSP_NotifyMsgContent* pData)
{
    printf("On Notify callback. submethod-%d \n", pData->iSubMethod);
    return BVCU_RESULT_S_OK;
}
BVCU_Result CPUSessionBase::OnCommand(BVCSP_HSession hSession, BVCSP_Command* pCommand)
{
    BVCSP_Event_SessionCmd szResult;
    memset(&szResult, 0x00, sizeof(szResult));
    szResult.iPercent = 100;
    szResult.iResult = BVCU_RESULT_S_OK;
    if (pCommand->iMethod == BVCU_METHOD_QUERY)
    {
    }
    else if (pCommand->iMethod == BVCU_METHOD_CONTROL)
    {
    }
    return BVCU_RESULT_E_FAILED;// 不接受这个命令处理
}
BVCU_Result CPUSessionBase::OnDialogCmd(BVCSP_HDialog hDialog, int iEventCode, BVCSP_DialogParam* pParam)
{
    return BVCU_RESULT_E_FAILED;// 不接受这个命令处理
}