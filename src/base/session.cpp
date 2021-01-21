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

int CPUSessionBase::AddAVChannel(CAVChannelBase* pChannel)
{
    for (int i = 0; i < MAX_AV_CHANNEL_COUNT; ++i)
    {
        if (m_avChannels[i] == 0)
        {
            m_avChannels[i] = pChannel;
            pChannel->SetIndex(i);
            return i;
        }
    }
    return -1;
}
int CPUSessionBase::AddGPSChannel(CGPSChannelBase* pChannel)
{
    for (int i = 0; i < MAX_GPS_CHANNEL_COUNT; ++ i)
    {
        if (m_GPSChannels[i] == 0)
        {
            m_GPSChannels[i] = pChannel;
            pChannel->SetIndex(i);
            return i;
        }
    }
    return -1;
}
int CPUSessionBase::AddTSPChannel(CTSPChannelBase* pChannel)
{
    for (int i = 0; i < MAX_TSP_CHANNEL_COUNT; ++i)
    {
        if (m_TSPChannels[i] == 0)
        {
            m_TSPChannels[i] = pChannel;
            pChannel->SetIndex(i);
            return i;
        }
    }
    return -1;
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
    BVCU_PUCFG_ChannelInfo* pChannelList = NULL;
    int iChannelCount = 0;
    for (int i = 0; i < MAX_AV_CHANNEL_COUNT; ++ i)
    {
        if (m_avChannels[i] != NULL)
            ++iChannelCount;
    }
    m_deviceInfo.iGPSCount = 0;
    for (int i = 0; i < MAX_GPS_CHANNEL_COUNT; ++i)
    {
        if (m_GPSChannels[i] != NULL)
        {
            ++iChannelCount;
            m_deviceInfo.iGPSCount++;
        }
    }
    m_deviceInfo.iSerialPortCount = 0;
    for (int i = 0; i < MAX_TSP_CHANNEL_COUNT; ++i)
    {
        if (m_TSPChannels[i] != NULL)
        {
            ++iChannelCount;
            m_deviceInfo.iSerialPortCount++;
        }
    }
    m_deviceInfo.iChannelCount = iChannelCount;
    if (iChannelCount > 0)
    {
        pChannelList = new BVCU_PUCFG_ChannelInfo[iChannelCount];
        if (pChannelList)
        {
            int channelIndex = 0;
            for (int i = 0; i < MAX_AV_CHANNEL_COUNT; ++i)
            {
                if (m_avChannels[i] != NULL)
                {
                    memset(&pChannelList[channelIndex], 0x00, sizeof(pChannelList[channelIndex]));
                    pChannelList[channelIndex].iChannelIndex = m_avChannels[i]->GetChannelIndex();
                    pChannelList[channelIndex].iMediaDir = m_avChannels[i]->GetSupportMediaDir();
                    strncpy_s(pChannelList[channelIndex].szName, sizeof(pChannelList[0].szName), m_avChannels[i]->GetName(), _TRUNCATE);
                    ++channelIndex;
                }
            }
            for (int i = 0; i < MAX_GPS_CHANNEL_COUNT; ++i)
            {
                if (m_GPSChannels[i] != NULL)
                {
                    memset(&pChannelList[channelIndex], 0x00, sizeof(pChannelList[channelIndex]));
                    pChannelList[channelIndex].iChannelIndex = m_GPSChannels[i]->GetChannelIndex();
                    pChannelList[channelIndex].iMediaDir = m_GPSChannels[i]->GetSupportMediaDir();
                    strncpy_s(pChannelList[channelIndex].szName, sizeof(pChannelList[0].szName), m_GPSChannels[i]->GetName(), _TRUNCATE);
                    ++channelIndex;
                }
            }
            for (int i = 0; i < MAX_TSP_CHANNEL_COUNT; ++i)
            {
                if (m_TSPChannels[i] != NULL)
                {
                    memset(&pChannelList[channelIndex], 0x00, sizeof(pChannelList[channelIndex]));
                    pChannelList[channelIndex].iChannelIndex = m_TSPChannels[i]->GetChannelIndex();
                    pChannelList[channelIndex].iMediaDir = m_TSPChannels[i]->GetSupportMediaDir();
                    strncpy_s(pChannelList[channelIndex].szName, sizeof(pChannelList[0].szName), m_TSPChannels[i]->GetName(), _TRUNCATE);
                    ++channelIndex;
                }
            }
            m_sesParam.stEntityInfo.iChannelCount = channelIndex;
            m_sesParam.stEntityInfo.pChannelList = pChannelList;
        }
    }
    else
        m_sesParam.stEntityInfo.iChannelCount = 0;
    m_sesParam.stEntityInfo.iOnlineThrough = through;
    m_sesParam.stEntityInfo.iLatitude = lat;
    m_sesParam.stEntityInfo.iLongitude = lng;
    BVCU_Result bvResult = BVCSP_Login(&m_session, &m_sesParam);
    printf("Call BVCSP_login(%s:%d %s) code:%d session:%p\n", m_sesParam.szServerAddr, m_sesParam.iServerPort
        , m_deviceInfo.szID, bvResult, m_session);
    if (pChannelList)
        delete pChannelList;
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

CChannelBase* CPUSessionBase::GetChannelBase(int channelIndex)
{
    CChannelBase* pChannel = 0;
    if (BVCU_SUBDEV_INDEXMAJOR_MIN_CHANNEL <= channelIndex && channelIndex <= BVCU_SUBDEV_INDEXMAJOR_MAX_CHANNEL)
    {
        int index = channelIndex - BVCU_SUBDEV_INDEXMAJOR_MIN_CHANNEL;
        if (index < MAX_AV_CHANNEL_COUNT)
            pChannel = m_avChannels[index];
    }
    else if (BVCU_SUBDEV_INDEXMAJOR_MIN_GPS <= channelIndex && channelIndex <= BVCU_SUBDEV_INDEXMAJOR_MAX_GPS)
    {
        int index = channelIndex - BVCU_SUBDEV_INDEXMAJOR_MIN_GPS;
        if (index < MAX_GPS_CHANNEL_COUNT)
            pChannel = m_GPSChannels[index];
    }
    else if (BVCU_SUBDEV_INDEXMAJOR_MIN_TSP <= channelIndex && channelIndex <= BVCU_SUBDEV_INDEXMAJOR_MAX_TSP)
    {
        int index = channelIndex - BVCU_SUBDEV_INDEXMAJOR_MIN_TSP;
        if (index < MAX_TSP_CHANNEL_COUNT)
            pChannel = m_TSPChannels[index];
    }
    return pChannel;
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
void CPUSessionBase::OnDialogEvent(BVCSP_HDialog hDialog, int iEventCode, BVCSP_Event_DialogCmd* pParam)
{
    if (pParam)
    {
        BVCSP_DialogParam* pDialogParam = pParam->pDialogParam;
        CPUSessionBase* pSession = (CPUSessionBase*)pDialogParam->pUserData;
        if (pSession == 0 || pDialogParam->hSession != pSession->m_session)
            return;
        CChannelBase* pChannel = pSession->GetChannelBase(pDialogParam->stTarget.iIndexMajor);
        if (pChannel == 0 || pChannel->GetHDialog() != hDialog)
            return;
        if (iEventCode == BVCSP_EVENT_DIALOG_OPEN && BVCU_Result_SUCCEEDED(pParam->iResult))
            pChannel->OnOpen();
        else if (iEventCode != BVCSP_EVENT_DIALOG_UPDATE)
        {
            pChannel->SetHialog(0, 0);
            pChannel->OnClose();
        }
    }
}
BVCU_Result CPUSessionBase::afterDialogRecv(BVCSP_HDialog hDialog, BVCSP_Packet* pPacket)
{
    BVCSP_DialogInfo info;
    if (BVCU_Result_SUCCEEDED(BVCSP_GetDialogInfo(hDialog, &info)))
    {
        CPUSessionBase* pSession = (CPUSessionBase*)info.stParam.pUserData;
        if (pSession == 0 || info.stParam.hSession != pSession->m_session)
            return BVCU_RESULT_E_BADREQUEST;

        CChannelBase* pChannel = pSession->GetChannelBase(info.stParam.stTarget.iIndexMajor);
        if (pChannel == 0 || pChannel->GetHDialog() != hDialog)
            return BVCU_RESULT_E_BADREQUEST;
        return pChannel->OnRecvPacket(pPacket);
    }
    return BVCU_RESULT_S_OK;
}
BVCU_Result CPUSessionBase::OnNotify(BVCSP_HSession hSession, BVCSP_NotifyMsgContent* pData)
{
    printf("On Notify callback. submethod-%d \n", pData->iSubMethod);
    return BVCU_RESULT_S_OK;
}
BVCU_Result CPUSessionBase::OnCommand(BVCSP_HSession hSession, BVCSP_Command* pCommand)
{
    CPUSessionBase* pSession = 0;
    BVCSP_SessionInfo sesInfo;
    if (BVCU_Result_SUCCEEDED(BVCSP_GetSessionInfo(hSession, &sesInfo)))
    {
        pSession = (CPUSessionBase*)sesInfo.stParam.pUserData;
    }
    if (pSession == 0 || hSession != pSession->m_session)
        return BVCU_RESULT_E_BADREQUEST;
    BVCSP_Event_SessionCmd szResult;
    memset(&szResult, 0x00, sizeof(szResult));
    szResult.iPercent = 100;
    szResult.iResult = BVCU_RESULT_S_OK;
    if (pCommand->iMethod == BVCU_METHOD_QUERY)
    {
        if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_GPSDATA)
        {   // 获取PU的GPS通道数据。输入类型：无；输出类型: BVCU_PUCFG_GPSData
            int index = pCommand->iTargetIndex;
            if (index < MAX_GPS_CHANNEL_COUNT && pSession->m_GPSChannels[index] != 0)
            {
                const BVCU_PUCFG_GPSData* pParam = pSession->m_GPSChannels[index]->OnGetGPSData();
                if (pParam)
                {
                    szResult.stContent.iDataCount = 1;
                    szResult.stContent.pData = (void*)pParam;
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_SERIALPORT)
        {   // 串口属性。输入类型：BVCU_PUCFG_SerialPort;输出类型：无
            int index = pCommand->iTargetIndex;
            if (index < MAX_TSP_CHANNEL_COUNT && pSession->m_TSPChannels[index] != 0)
            {
                const BVCU_PUCFG_SerialPort* pParam = pSession->m_TSPChannels[index]->OnGetTSPParam();
                if (pParam)
                {
                    szResult.stContent.iDataCount = 1;
                    szResult.stContent.pData = (void*)pParam;
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_GPS)
        {   // GPS属性。输入类型：BVCU_PUCFG_GPSParam;输出类型：无
            int index = pCommand->iTargetIndex;
            if (index < MAX_GPS_CHANNEL_COUNT && pSession->m_GPSChannels[index] != 0)
            {
                const BVCU_PUCFG_GPSParam* pParam = pSession->m_GPSChannels[index]->OnGetGPSParam();
                if (pParam)
                {
                    szResult.stContent.iDataCount = 1;
                    szResult.stContent.pData = (void*)pParam;
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
    }
    else if (pCommand->iMethod == BVCU_METHOD_CONTROL)
    {
        if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_DEVICEINFO)
        {   // 设备信息。输入类型：BVCU_PUCFG_DeviceInfo;输出类型：无
                const BVCU_PUCFG_DeviceInfo* pParam = (BVCU_PUCFG_DeviceInfo*)pCommand->stMsgContent.pData;
                if (pParam)
                {
                    szResult.iResult = pSession->OnSetInfo(pParam->szName, pParam->iLatitude, pParam->iLongitude);
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_CHANNELINFO)
        {   // 某个PU的通道信息。输入类型：BVCU_PUCFG_PUChannelInfo；输出类型: 无；触发类型：同名Notify
            const BVCU_PUCFG_PUChannelInfo* pParam = (BVCU_PUCFG_PUChannelInfo*)pCommand->stMsgContent.pData;
            if (pParam)
            {
                for (int i = 0; i < pParam->iChannelCount; ++ i)
                {
                    CChannelBase* pChannel = pSession->GetChannelBase(pParam->pChannel[i].iChannelIndex);
                    szResult.iResult = pChannel->OnSetName(pParam->pChannel[i].szName);
                }
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_SERIALPORT)
        {   // 串口属性。输入类型：BVCU_PUCFG_SerialPort;输出类型：无
            int index = pCommand->iTargetIndex;
            if (index < MAX_TSP_CHANNEL_COUNT && pSession->m_TSPChannels[index] != 0)
            {
                const BVCU_PUCFG_SerialPort* pParam = (BVCU_PUCFG_SerialPort*)pCommand->stMsgContent.pData;
                if (pParam)
                {
                    szResult.iResult = pSession->m_TSPChannels[index]->OnSetTSPParam(pParam);
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_GPS)
        {   // GPS属性。输入类型：BVCU_PUCFG_GPSParam;输出类型：无
            int index = pCommand->iTargetIndex;
            if (index < MAX_GPS_CHANNEL_COUNT && pSession->m_GPSChannels[index] != 0)
            {
                const BVCU_PUCFG_GPSParam* pParam = (BVCU_PUCFG_GPSParam*)pCommand->stMsgContent.pData;
                if (pParam)
                {
                    szResult.iResult = pSession->m_GPSChannels[index]->OnSetGPSParam(pParam);
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_PTZCONTROL)
        {   // 操作云台。输入类型：BVCU_PUCFG_PTZControl；输出类型：无
            int index = pCommand->iTargetIndex;
            if (index < MAX_AV_CHANNEL_COUNT && pSession->m_avChannels[index] != 0)
            {
                const BVCU_PUCFG_PTZControl* pParam = (BVCU_PUCFG_PTZControl*)pCommand->stMsgContent.pData;
                if (pParam)
                {
                    szResult.iResult = pSession->m_avChannels[index]->OnPTZCtrl(pParam);
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
    }
    return BVCU_RESULT_E_FAILED;// 不接受这个命令处理
}
BVCU_Result CPUSessionBase::OnDialogCmd(BVCSP_HDialog hDialog, int iEventCode, BVCSP_DialogParam* pParam)
{
    BVCSP_SessionInfo sesInfo;
    if (BVCU_Result_SUCCEEDED(BVCSP_GetSessionInfo(pParam->hSession, &sesInfo)))
    {
        CPUSessionBase* pSession = (CPUSessionBase*)sesInfo.stParam.pUserData;
        if (pSession == 0 || pParam->hSession != pSession->m_session)
            return BVCU_RESULT_E_BADREQUEST;
        CChannelBase::g_bvcsp_onevent = pParam->OnEvent;
        CChannelBase* pChannel = pSession->GetChannelBase(pParam->stTarget.iIndexMajor);
        if (pChannel)
        {
            int iOldDir = pChannel->GetOpenDir();
            pChannel->SetHialog(hDialog, pParam->iAVStreamDir);
            BVCU_Result iReuslt = pChannel->OnOpenRequest();
            if (BVCU_Result_FAILED(iReuslt))
            {
                if (iEventCode == BVCSP_EVENT_DIALOG_OPEN)
                    pChannel->SetHialog(0, 0);
                else
                    pChannel->SetHialog(hDialog, iOldDir);
                return iReuslt;
            }
            pParam->pUserData = pSession;
            pParam->OnEvent = OnDialogEvent;
            pParam->afterRecv = afterDialogRecv;
            if (pParam->stTarget.iIndexMajor <= BVCU_SUBDEV_INDEXMAJOR_MAX_CHANNEL)
            {
                pChannel->SetBOpening(true);
                return BVCU_RESULT_S_PENDING;
            }
            else
                return BVCU_RESULT_S_OK;
        }
    }
    return BVCU_RESULT_E_FAILED;// 不接受这个命令处理
}