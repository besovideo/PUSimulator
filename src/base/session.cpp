#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../utils/utils.h"
#include "session.h"

#ifdef _MSC_VER
#include <windows.h>
#include <sysinfoapi.h>
#endif

CPUSessionBase::CPUSessionBase(const char* ID)
{
    memset(&m_deviceInfo, 0x00, sizeof(m_deviceInfo));
    strncpy_s(m_deviceInfo.szID, sizeof(m_deviceInfo.szID), ID, _TRUNCATE);
    strncpy_s(m_deviceInfo.szManufacturer, sizeof(m_deviceInfo.szManufacturer), "simulator", _TRUNCATE);
    strncpy_s(m_deviceInfo.szProductName, sizeof(m_deviceInfo.szProductName), "PUSimulator G1B2", _TRUNCATE);
    strncpy_s(m_deviceInfo.szSoftwareVersion, sizeof(m_deviceInfo.szSoftwareVersion), "0.0.1", _TRUNCATE);
    strncpy_s(m_deviceInfo.szHardwareVersion, sizeof(m_deviceInfo.szHardwareVersion), "0.0.1", _TRUNCATE);
    // 默认设置无效的经纬度地址。
    m_deviceInfo.iLatitude = 200 * 10000000;
    m_deviceInfo.iLongitude = 200 * 10000000;
    // 通道信息
    memset(m_avChannels, 0x00, sizeof(m_avChannels));
    memset(&m_GPSInterface, 0x00, sizeof(m_GPSInterface));
    m_session = 0;
    m_bOnline = false;
    m_bBusying = false;
    m_fileManager = NULL;
    // 登录参数
    memset(&m_sesParam, 0x00, sizeof(m_sesParam));
    m_sesParam.iSize = sizeof(m_sesParam);
    m_sesParam.iTimeOut = 8000;
    strncpy_s(m_sesParam.szClientID, sizeof(m_sesParam.szClientID), ID, _TRUNCATE);
    strncpy_s(m_sesParam.szUserAgent, sizeof(m_sesParam.szUserAgent), "PUSimulatorG1B2", _TRUNCATE);
    // strncpy_s(m_sesParam.szUKeyID, sizeof(m_sesParam.szUKeyID), "password_encrypted", _TRUNCATE);
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

void CPUSessionBase::SetUser(const char* id, const char* passwd)
{
    m_sesParam.iClientType = BVCSP_CLIENT_TYPE_UA;
    strncpy_s(m_sesParam.szUserName, sizeof(m_sesParam.szUserName), id, _TRUNCATE);
    strncpy_s(m_sesParam.szPassword, sizeof(m_sesParam.szPassword), passwd, _TRUNCATE);
}
void CPUSessionBase::SetName(const char* Name)
{
    strncpy_s(m_deviceInfo.szName, sizeof(m_deviceInfo.szName), Name, _TRUNCATE);
}
void CPUSessionBase::SetManufacturer(const char* Manufacturer)
{
    strncpy_s(m_deviceInfo.szManufacturer, sizeof(m_deviceInfo.szManufacturer), Manufacturer, _TRUNCATE);
}
void CPUSessionBase::SetProductName(const char* ProductName)
{
    strncpy_s(m_deviceInfo.szProductName, sizeof(m_deviceInfo.szProductName), ProductName, _TRUNCATE);
}
void CPUSessionBase::SetSoftwareVersion(const char* SoftwareVersion)
{
    strncpy_s(m_deviceInfo.szSoftwareVersion, sizeof(m_deviceInfo.szSoftwareVersion), SoftwareVersion, _TRUNCATE);
}
void CPUSessionBase::SetHardwareVersion(const char* HardwareVersion)
{
    strncpy_s(m_deviceInfo.szHardwareVersion, sizeof(m_deviceInfo.szHardwareVersion), HardwareVersion, _TRUNCATE);
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
void CPUSessionBase::SetPTZCount(int count)
{
    m_deviceInfo.iPTZCount = count;
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

void CPUSessionBase::AddGPSInterface(const bvGPSParam* pGPS)
{
    if (pGPS && pGPS->OnGetGPSData && pGPS->OnGetGPSParam && pGPS->OnSetGPSParam)
    {
        m_GPSInterface = *pGPS;
        m_deviceInfo.iGPSCount = 1;
    }
}

int BVCU_BVCSP_DialogDownload(CFileTransfer* pFile, const BVCU_File_TransferParam* pBVCUParam, BVCSP_DialogParam* pBVCSPParam, BVCSP_DialogControlParam* pBVCSPCtrl)
{
    memset(pBVCSPParam, 0x00, sizeof(*pBVCSPParam));
    pBVCSPParam->iSize = sizeof(*pBVCSPParam);
    pBVCSPParam->pUserData = pFile;
    strncpy_s(pBVCSPParam->stTarget.szID, sizeof(pBVCSPParam->stTarget.szID), pBVCUParam->szTargetID, _TRUNCATE);
    pBVCSPParam->stTarget.iIndexMajor = BVCU_SUBDEV_INDEXMAJOR_DOWNLOAD;
    pBVCSPParam->stTarget.iIndexMinor = -1;
    // pBVCSPParam->stFileTarget.pPathFileName = pBVCUParam->pRemoteFilePathName;
    // pBVCSPParam->stFileTarget.pFileInfoJson = pBVCUParam->pFileInfoJson;
    pBVCSPParam->stFileTarget.iStartTime_iOffset = pBVCUParam->iFileStartOffset;
    pBVCSPParam->stFileTarget.iEndTime_iFileSize = pFile->GetFileSize();
    if (pBVCUParam->bUpload)
        pBVCSPParam->iAVStreamDir = BVCU_MEDIADIR_DATASEND;
    else
        pBVCSPParam->iAVStreamDir = BVCU_MEDIADIR_DATARECV;
    pBVCSPParam->bOverTCP = 1;
    // control
    memset(pBVCSPCtrl, 0x00, sizeof(*pBVCSPCtrl));
    pBVCSPCtrl->iDelayMax = 5000;
    pBVCSPCtrl->iDelayMin = 500;
    pBVCSPCtrl->iDelayVsSmooth = 3;
    pBVCSPCtrl->iTimeOut = pBVCUParam->iTimeOut;
    return 1;
}

int CPUSessionBase::UploadFile(BVCU_File_HTransfer* phTransfer, const char* localFilePathName, const bvFileInfo* fileInfo)
{
    if (m_fileManager == NULL)
        return BVCU_RESULT_E_NOTINITILIZED;
    if (fileInfo == NULL || fileInfo->filetype == NULL || fileInfo->filetype[0] == 0)
    {
        printf("fileInfo->filetype is must\n");
        return BVCU_RESULT_E_BADREQUEST;
    }
    CFileTransfer* pFileTransfer = m_fileManager->AddFileTransfer();
    if (pFileTransfer == NULL)
        return BVCU_RESULT_E_ALLOCMEMFAILED;
    BVCU_Result iResult = BVCU_RESULT_E_INVALIDARG;
    BVCSP_HDialog cspDialog = NULL;
    BVCU_File_TransferParam param;
    memset(&param, 0x00, sizeof(param));
    param.iSize = sizeof(param);
    strcpy(param.szTargetID, "NRU_");
    param.bUpload = 1;
    param.pLocalFilePathName = (char*)localFilePathName;
    param.fileInfo = fileInfo;
    param.iFileStartOffset = -1;
    if (pFileTransfer->SetInfo(&param))
    {
        char fileInfoJson[512] = { 0 };
        char remotePath[512] = { 0 };
        BVCSP_DialogParam cspParam;
        BVCSP_DialogControlParam cspControl;
        BVCU_BVCSP_DialogDownload(pFileTransfer, pFileTransfer->GetParam(), &cspParam, &cspControl);
        m_fileManager->FileInfo2Json(fileInfo, fileInfoJson, sizeof(fileInfoJson));
        cspParam.stFileTarget.pFileInfoJson = fileInfoJson;

        const char* pFileName = extract_filename(pFileTransfer->GetParam()->pLocalFilePathName); // 这里的pLocalFilePathName是utf8编码
        struct tm* timeinfo = localtime((time_t *)&fileInfo->starttime);
        sprintf(remotePath, "/%s/%s/%d%02d%02d/%s", m_deviceInfo.szID, fileInfo->filetype,
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, pFileName);
        cspParam.stFileTarget.pPathFileName = remotePath;

        cspParam.hSession = m_session;
        cspParam.pUserData = m_fileManager;
        cspParam.afterRecv = CFileTransManager::OnAfterRecv_BVCSP;
        cspParam.OnEvent = CFileTransManager::OnDialogEvent_BVCSP;
        iResult = BVCSP_Dialog_Open(&cspDialog, &cspParam, &cspControl);
    }
    printf("%ld call OpenFile:%ld targetID:%s file:%s result:%d ",
        m_session, pFileTransfer->GetHandle(), param.szTargetID, param.pLocalFilePathName, iResult);
    if (BVCU_Result_FAILED(iResult))
    {
        m_fileManager->RemoveFileTransfer(pFileTransfer);
        if (phTransfer)
            *phTransfer = NULL;
    }
    else
    {
        pFileTransfer->SetCSPDialog(cspDialog);
        if (phTransfer)
            *phTransfer = (BVCU_File_HTransfer)pFileTransfer->GetHandle();
    }
    return iResult;
}

void CPUSessionBase::SetServer(const char* IP, int udp_port, int tcp_port, int proto, int timeout, int iKeepaliveInterval)
{
    strncpy_s(m_sesParam.szServerAddr, sizeof(m_sesParam.szServerAddr), IP, _TRUNCATE);
    m_sesParam.iCmdProtoType = proto == 0 ? BVCU_PROTOTYPE_UDP : BVCU_PROTOTYPE_TCP;
    if (m_sesParam.iCmdProtoType == BVCU_PROTOTYPE_TCP)
        m_sesParam.iServerPort = tcp_port;
    else
        m_sesParam.iServerPort = udp_port;
    m_sesParam.iTimeOut = timeout;
    m_sesParam.iKeepaliveInterval = iKeepaliveInterval;
}
int CPUSessionBase::Login(int through, int lat, int lng)
{
    m_bBusying = true;
    Logout();
    m_sesParam.pUserData = this;
    // 通道信息
    BVCU_PUCFG_ChannelInfo* pChannelList = NULL;
    int iChannelCount = 0;
    for (int i = 0; i < MAX_AV_CHANNEL_COUNT; ++i)
    {
        if (m_avChannels[i] != NULL)
            ++iChannelCount;
    }

    m_deviceInfo.iSerialPortCount = 0;

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
            m_sesParam.stEntityInfo.iChannelCount = channelIndex;
            m_sesParam.stEntityInfo.pChannelList = pChannelList;
        }
    }
    else
        m_sesParam.stEntityInfo.iChannelCount = 0;
    m_sesParam.stEntityInfo.iOnlineThrough = through;
    m_sesParam.stEntityInfo.iLatitude = lat;
    m_sesParam.stEntityInfo.iLongitude = lng;
    m_sesParam.iKeepaliveInterval = 25 * 1000;
    BVCU_Result bvResult = BVCSP_Login(&m_session, &m_sesParam);
    printf("Call BVCSP_login(%s:%d %s) code:%d session:%p\n", m_sesParam.szServerAddr, m_sesParam.iServerPort, m_deviceInfo.szID, bvResult, m_session);
    if (pChannelList)
        delete[] pChannelList;
    if (BVCU_Result_FAILED(bvResult))
        m_bBusying = false;
    return bvResult;
}
int CPUSessionBase::Logout()
{
    if (m_session)
    {
        m_bBusying = true;
        BVCU_Result bvResult = BVCSP_Logout(m_session);
        printf("Call BVCSP_Logout(%s:%d %s) code:%d session:%p\n", m_sesParam.szServerAddr, m_sesParam.iServerPort, m_deviceInfo.szID, bvResult, m_session);
        m_bOnline = false;
        m_session = 0;
        if (BVCU_Result_FAILED(bvResult))
            m_bBusying = false;
    }
    m_bOnline = false;
    return 0;
}

int CPUSessionBase::SendAlarm(int alarmType, int index, int value, int bEnd, const char* desc)
{
    if (m_session)
    {
        BVCU_Event_Source eventAlarm;
        memset(&eventAlarm, 0x00, sizeof(eventAlarm));
        eventAlarm.iEventType = alarmType; // BVCU_EVENT_TYPE_ALERTIN;
        eventAlarm.iSubDevIdx = index;
        eventAlarm.iValue = value;
        eventAlarm.bEnd = bEnd;
        sprintf_s(eventAlarm.szKey, "PUSimulatorE_%lld", time(NULL));
        if (desc)
            strncpy_s(eventAlarm.szEventDesc, sizeof(eventAlarm.szEventDesc), desc, _TRUNCATE);
        if (m_GPSInterface.OnGetGPSData)
        { // 如果有GPS模块，获取当前定位。
            BVCU_PUCFG_GPSData* gps = m_GPSInterface.OnGetGPSData();
            if (gps)
            {
                eventAlarm.iLongitude = gps->iLongitude;
                eventAlarm.iLatitude = gps->iLatitude;
            }
        }
        strncpy_s(eventAlarm.szDevID, sizeof(eventAlarm.szDevID), m_deviceInfo.szID, _TRUNCATE);
        {
            time_t now = time(NULL);      // 时间应该也是从GPS设备中读取
            tm* ptm = (tm*)gmtime(&now); // 初特殊说明，网传的时间都是UTC时间。
            eventAlarm.stTime.iYear = ptm->tm_year + 1900;
            eventAlarm.stTime.iMonth = ptm->tm_mon + 1;
            eventAlarm.stTime.iDay = ptm->tm_mday;
            eventAlarm.stTime.iHour = ptm->tm_hour;
            eventAlarm.stTime.iMinute = ptm->tm_min;
            eventAlarm.stTime.iSecond = ptm->tm_sec;
        }

        BVCSP_Notify notify;
        memset(&notify, 0x00, sizeof(notify));
        notify.iSize = sizeof(notify);
        notify.stMsgContent.iSize = sizeof(notify.stMsgContent);
        notify.stMsgContent.iSubMethod = BVCU_SUBMETHOD_EVENT_NOTIFY;
        notify.stMsgContent.stMsgContent.iDataCount = 1;
        notify.stMsgContent.stMsgContent.pData = &eventAlarm;
        BVCU_Result bvResult = BVCSP_SendNotify(m_session, &notify);
        printf("Call BVCSP_SendNotify code:%d session:%p\n", bvResult, m_session);
        return bvResult;
    }
    return BVCU_RESULT_E_BADREQUEST;
}

int CPUSessionBase::SendCommand(int iMethod, int iSubMethod, char* pTargetID, void* pData, void* pUserData)
{
    if (m_session)
    {
        BVCSP_Command cmd;
        memset(&cmd, 0x00, sizeof(cmd));
        cmd.iSize = sizeof(cmd);
        cmd.OnEvent = CPUSessionBase::OnCommandBack;
        cmd.iMethod = iMethod;
        cmd.iSubMethod = iSubMethod;
        cmd.pUserData = pUserData;
        if (pTargetID)
        {
            strncpy_s(cmd.szTargetID, sizeof(cmd.szTargetID), pTargetID, _TRUNCATE);
        }
        if (pData)
        {
            cmd.stMsgContent.iDataCount = 1;
            cmd.stMsgContent.pData = pData;
        }
        BVCU_Result bvResult = BVCSP_SendCmd(m_session, &cmd);
        printf("Call BVCSP_SendCmd code:%d session:%p\n", bvResult, m_session);
        return bvResult;
    }
    return BVCU_RESULT_E_BADREQUEST;
}

int CPUSessionBase::SendNotify(int iSubMethod, void* pData)
{
    BVCSP_Notify notify;
    memset(&notify, 0x00, sizeof(notify));
    notify.iSize = sizeof(notify);
    notify.stMsgContent.iSize = sizeof(notify.stMsgContent);
    notify.stMsgContent.iSubMethod = iSubMethod;
    if (pData != NULL)
    {
        notify.stMsgContent.stMsgContent.iDataCount = 1;
        notify.stMsgContent.stMsgContent.pData = pData;
    }
    BVCU_Result bvResult = BVCSP_SendNotify(m_session, &notify);
    printf("Call BVCSP_SendNotify code:%d session:%p\n", bvResult, m_session);
    return bvResult;
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
    return pChannel;
}

void CPUSessionBase::OnSessionEvent(BVCSP_HSession hSession, int iEventCode, void* pParam)
{
    BVCU_Result iResult = (BVCU_Result)((long long)(pParam));
    printf("session event hSession:%p iEventCode:%d result:%d \n", hSession, iEventCode, iResult);
    BVCSP_SessionInfo sesInfo;
    if (BVCU_Result_SUCCEEDED(BVCSP_GetSessionInfo(hSession, &sesInfo)))
    {
        CPUSessionBase* pSession = (CPUSessionBase*)sesInfo.stParam.pUserData;
        if (pSession == 0 || hSession != pSession->m_session)
            return;

        pSession->m_bBusying = false;
        if (iEventCode == BVCSP_EVENT_SESSION_OPEN)
        {
            if (BVCU_Result_SUCCEEDED(iResult))
            { // 登录成功
                printf("Login succeeded\n");
                pSession->m_bOnline = true;
            }
            else
            {
                printf("============================= Login failed: %d\n", iResult);
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
        if (iEventCode == BVCSP_EVENT_DIALOG_OPEN)
        {
            if (BVCU_Result_SUCCEEDED(pParam->iResult))
                pChannel->OnOpen();
            else
            {
                pChannel->SetHialog(0, 0);
                pChannel->OnClose();
            }
        }
        else if (iEventCode == BVCSP_EVENT_DIALOG_CLOSE)
        {
            pChannel->SetHialog(0, 0);
            pChannel->OnClose();
        }
        else if (iEventCode == BVCSP_EVENT_DIALOG_PLIKEY)
        { // 请求关键帧
            pChannel->OnPLI();
        }
    }
}
BVCU_Result CPUSessionBase::OnAudioRecv(BVCSP_HDialog hDialog, BVCSP_Packet* pPacket)
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
    printf("On Notify callback.  session = %p  submethod = %d \n", hSession, pData->iSubMethod);
    return BVCU_RESULT_S_OK;
}
BVCU_Result CPUSessionBase::OnCommand(BVCSP_HSession hSession, BVCSP_Command* pCommand)
{
    printf("On Notify callback.  session = %p  method = %d submethod = %d \n", hSession, pCommand->iMethod, pCommand->iSubMethod);
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
        { // 获取PU的GPS通道数据。输入类型：无；输出类型: BVCU_PUCFG_GPSData
            if (pSession->m_GPSInterface.OnGetGPSData)
            {
                const BVCU_PUCFG_GPSData* pParam = pSession->m_GPSInterface.OnGetGPSData();
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
        { // GPS属性。输入类型：BVCU_PUCFG_GPSParam;输出类型：无
            if (pSession->m_GPSInterface.OnGetGPSParam)
            {
                const BVCU_PUCFG_GPSParam* pParam = pSession->m_GPSInterface.OnGetGPSParam();
                if (pParam)
                {
                    szResult.stContent.iDataCount = 1;
                    szResult.stContent.pData = (void*)pParam;
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_PTZATTR)
        { // GPS属性。输入类型：BVCU_PUCFG_GPSParam;输出类型：无
            int index = pCommand->iTargetIndex;
            if (index < MAX_AV_CHANNEL_COUNT && pSession->m_avChannels[index] != 0)
            {
                const BVCU_PUCFG_PTZAttr* pParam = pSession->m_avChannels[index]->OnGetPTZParam();
                if (pParam)
                {
                    szResult.stContent.iDataCount = 1;
                    szResult.stContent.pData = (void*)pParam;
                    pCommand->OnEvent(hSession, pCommand, &szResult);
                    return BVCU_RESULT_S_OK;
                }
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_RECORDSTATUS)
        { // 获取PU的录像状态。输入类型：无；输出类型: BVCU_PUCFG_RecordStatus
            const BVCU_PUCFG_RecordStatus* pParam = pSession->OnGetRecordStatus();
            if (pParam)
            {
                szResult.stContent.iDataCount = 1;
                szResult.stContent.pData = (void*)pParam;
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_SEARCH_LIST)
        { // 获取 设备端存储文件列表
            BVCU_Search_Request* req = (BVCU_Search_Request*)pCommand->stMsgContent.pData;
            if (req->stSearchInfo.iType != BVCU_SEARCH_TYPE_FILE)
                return BVCU_RESULT_E_UNSUPPORTED;
            BVCU_Search_Response* pParam = pSession->OnGetRecordFiles((BVCU_Search_Request*)pCommand->stMsgContent.pData);
            if (pParam)
            {
                szResult.stContent.iDataCount = 1;
                szResult.stContent.pData = pParam;
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
    }
    else if (pCommand->iMethod == BVCU_METHOD_CONTROL)
    {
        if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_DEVICEINFO)
        { // 设备信息。输入类型：BVCU_PUCFG_DeviceInfo;输出类型：无
            const BVCU_PUCFG_DeviceInfo* pParam = (BVCU_PUCFG_DeviceInfo*)pCommand->stMsgContent.pData;
            if (pParam)
            {
                szResult.iResult = pSession->OnSetInfo(pParam->szName, pParam->iLatitude, pParam->iLongitude);
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_CHANNELINFO)
        { // 某个PU的通道信息。输入类型：BVCU_PUCFG_PUChannelInfo；输出类型: 无；触发类型：同名Notify
            const BVCU_PUCFG_PUChannelInfo* pParam = (BVCU_PUCFG_PUChannelInfo*)pCommand->stMsgContent.pData;
            if (pParam)
            {
                for (int i = 0; i < pParam->iChannelCount; ++i)
                {
                    CChannelBase* pChannel = pSession->GetChannelBase(pParam->pChannel[i].iChannelIndex);
                    szResult.iResult = pChannel->OnSetName(pParam->pChannel[i].szName);
                }
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_GPS)
        { // GPS属性。输入类型：BVCU_PUCFG_GPSParam;输出类型：无
            const BVCU_PUCFG_GPSParam* pParam = (BVCU_PUCFG_GPSParam*)pCommand->stMsgContent.pData;
            if (pParam && pSession->m_GPSInterface.OnSetGPSParam)
            {
                szResult.iResult = pSession->m_GPSInterface.OnSetGPSParam(pParam);
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_PTZCONTROL)
        { // 操作云台。输入类型：BVCU_PUCFG_PTZControl；输出类型：无
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
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_SUBSCRIBE)
        { // 订阅/取消订阅 GPS。输入类型：BVCU_PUCFG_Subscribe;输出类型：无
            const BVCU_PUCFG_Subscribe* pParam = (BVCU_PUCFG_Subscribe*)pCommand->stMsgContent.pData;
            if (pParam && strcmpi("GPS", pParam->szType) == 0 && pSession->m_GPSInterface.OnSubscribeGPS)
            { // 订阅GPS
                szResult.iResult = pSession->m_GPSInterface.OnSubscribeGPS(pParam->bStart, pParam->iInterval);
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_MANUALRECORD)
        { // 手工启动/停止PU录像。输入类型：BVCU_PUCFG_ManualRecord；输出类型：无
            const BVCU_PUCFG_ManualRecord* pParam = (BVCU_PUCFG_ManualRecord*)pCommand->stMsgContent.pData;
            if (pParam)
            {
                szResult.iResult = pSession->OnManualRecord(pParam->bStart);
                pCommand->OnEvent(hSession, pCommand, &szResult);
                return BVCU_RESULT_S_OK;
            }
        }
        // else if (pCommand->iSubMethod == BVCU_SUBMETHOD_PU_SNAPSHOT_ONE)
        // { // 手动抓拍一张图片并上传。输入类型：无；输出类型：BVCU_Search_FileInfo；
        //     BVCU_Search_Response *pParam = pSession->OnSnapshotOne();
        //     if (pParam)
        //     {
        //         szResult.stContent.iDataCount = 1;
        //         szResult.stContent.pData = pParam;
        //         pCommand->OnEvent(hSession, pCommand, &szResult);// 这个地方应该异步处理
        //         return BVCU_RESULT_S_OK;
        //     }
        // }
        else if (pCommand->iSubMethod == BVCU_SUBMETHOD_CONF_START ||
            pCommand->iSubMethod == BVCU_SUBMETHOD_CONF_PARTICIPATOR_ADD ||
            pCommand->iSubMethod == BVCU_SUBMETHOD_CONF_PARTICIPATOR_INVITE_SPEAK)
        { // 语音会议相关控制命令全部自动回OK
            pCommand->OnEvent(hSession, pCommand, &szResult);
            return BVCU_RESULT_S_OK;
        }
    }
    return BVCU_RESULT_E_FAILED; // 不接受这个命令处理
}
BVCU_Result CPUSessionBase::OnDialogCmd(BVCSP_HDialog hDialog, int iEventCode, BVCSP_DialogParam* pParam)
{
    BVCSP_SessionInfo sesInfo;
    if (BVCU_Result_SUCCEEDED(BVCSP_GetSessionInfo(pParam->hSession, &sesInfo)))
    {
        CPUSessionBase* pSession = (CPUSessionBase*)sesInfo.stParam.pUserData;
        if (pSession == 0 || pParam->hSession != pSession->m_session)
            return BVCU_RESULT_E_BADREQUEST;
        if (pParam->stTarget.iIndexMajor == BVCU_SUBDEV_INDEXMAJOR_DOWNLOAD)
        {
            if (pSession->m_fileManager != 0)
                return pSession->m_fileManager->OnDialogCmd_BVCSP(hDialog, iEventCode, pParam);
            else
                return BVCU_RESULT_E_BADREQUEST;
        }
        CChannelBase::g_bvcsp_onevent = pParam->OnEvent;
        CChannelBase* pChannel = pSession->GetChannelBase(pParam->stTarget.iIndexMajor);
        if (pChannel)
        {
            int iOldDir = pChannel->GetOpenDir();
            pChannel->SetHialog(hDialog, pParam->iAVStreamDir);
            BVCU_Result iReuslt = pChannel->OnOpenRequest(pParam);
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
            pParam->afterRecv = OnAudioRecv;
            if (iReuslt == BVCU_RESULT_S_PENDING)
                pChannel->SetBOpening(true);
            return iReuslt;
        }
    }
    return BVCU_RESULT_E_FAILED; // 不接受这个命令处理
}
void CPUSessionBase::OnCommandBack(BVCSP_HSession hSession, BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam)
{
    CPUSessionBase* pSession = 0;
    BVCSP_SessionInfo sesInfo;
    if (BVCU_Result_SUCCEEDED(BVCSP_GetSessionInfo(hSession, &sesInfo)))
    {
        pSession = (CPUSessionBase*)sesInfo.stParam.pUserData;
    }
    if (pSession == 0 || hSession != pSession->m_session)
        return;
    pSession->OnCommandReply(pCommand, pParam);
    return;
}
