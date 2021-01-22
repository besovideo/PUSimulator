
#include <string>
#include "../utils.h"
#include "session.h"
#include "filetransfer.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

CFileTransfer::CFileTransfer()
    : m_cspDialog(NULL)
    , m_iStatus(BVFILE_DIALOG_STATUS_NONE)
    , m_fFile(NULL)
    , m_iFileSize(0)
    , m_iCompleTime(0)
    , m_iHandle(0)
    , m_pSession(NULL)
    , m_bClosing(0)
    , m_bCallOpen(0)
{
    memset(&m_fileInfo, 0x00, sizeof(m_fileInfo));
    m_localFilePathName[0] = '\0';
}

void CFileTransfer::Init(CFileTransManager* pSession)
{
    if (m_cspDialog)
    {
        //BVCSP_Dialog_Close(m_cspDialog); // BVCU_Finsh()会死锁。
        m_cspDialog = NULL;
    }
    if (m_fFile)
    {
        m_pSession->bv_fclose(m_fFile);
        m_fFile = NULL;
    }
    m_iStatus = BVFILE_DIALOG_STATUS_NONE;
    m_iFileSize = 0;
    m_iCompleTime = 0;
    m_iLastDataTime = GetTickCount();
    m_iHandle = 0;
    m_pSession = pSession;
    m_bClosing = 0;
    m_bCallOpen = 0;
    m_localFilePathName[0] = '\0';
    memset(&m_fileInfo, 0x00, sizeof(m_fileInfo));
}

int CFileTransfer::SetInfo(BVCU_File_TransferParam* pParam)
{
    char szLocalFileAnsi[512] = { 0 };
    m_fileInfo.stParam = *pParam;
    if (pParam->pLocalFilePathName)
        strncpy_s(m_localFilePathName, sizeof(m_localFilePathName), pParam->pLocalFilePathName, _TRUNCATE);
    m_fileInfo.stParam.pLocalFilePathName = m_localFilePathName;
    if (m_fileInfo.stParam.iTimeOut <= 0)
        m_fileInfo.stParam.iTimeOut = 30 * 1000;
    // 打开文件
    Utf8ToAnsi(szLocalFileAnsi, sizeof(szLocalFileAnsi), m_localFilePathName);
    if (m_fileInfo.stParam.bUpload == 0)
    { // 下载文件
        m_fFile = m_pSession->bv_fsopen(szLocalFileAnsi, "ab+", 1);
        if (m_fFile)
        { // 处理续传
            m_pSession->bv_fseek(m_fFile, 0L, SEEK_END);
            m_iFileSize = m_pSession->bv_ftell(m_fFile);
            if (m_fileInfo.stParam.iFileStartOffset == -1) // == -1
            {
                m_fileInfo.stParam.iFileStartOffset = m_iFileSize;
                m_fileInfo.iTransferBytes = m_iFileSize;
            }
            else {
                m_fileInfo.stParam.iFileStartOffset = 0;
                m_pSession->bv_fseek(m_fFile, 0L, SEEK_SET);
            }
        }
    }
    else
    { // 上传文件，获取文件大小
        m_fFile = m_pSession->bv_fsopen(szLocalFileAnsi, "rb", 0);
        if (m_fFile)
        {
            m_pSession->bv_fseek(m_fFile, 0L, SEEK_END);
            m_iFileSize = m_pSession->bv_ftell(m_fFile);
            m_fileInfo.iTotalBytes = m_iFileSize;
        }
    }
    if (!m_fFile)
    {
        printf("SetInfo fopen failed: file:%s\n", szLocalFileAnsi);
        return 0;
    }
    m_iStatus = BVFILE_DIALOG_STATUS_INVITING;
    m_iLastDataTime = GetTickCount();
    return 1;
}

int CFileTransfer::SetInfo_RecvReq(BVCSP_DialogParam* pParam)
{
    m_fileInfo.stParam.iSize = sizeof(m_fileInfo.stParam);
    m_fileInfo.stParam.iTimeOut = 30 * 1000;
    strncpy_s(m_fileInfo.stParam.szTargetID, sizeof(m_fileInfo.stParam.szTargetID), pParam->szSourceID, _TRUNCATE);
    m_fileInfo.stParam.pRemoteFilePathName = pParam->stFileTarget.pPathFileName;
    m_fileInfo.stParam.iFileStartOffset = pParam->stFileTarget.iStartTime_iOffset;
    m_fileInfo.iTotalBytes = pParam->stFileTarget.iEndTime_iFileSize;
    if (pParam->iAVStreamDir == BVCU_MEDIADIR_DATASEND)
        m_fileInfo.stParam.bUpload = 1;
    return 1;
}

BVCU_File_TransferInfo* CFileTransfer::GetInfoNow(int bNetworkThread)
{
    if (bNetworkThread)
    {
        BVCSP_DialogInfo cspDialogInfo;
        BVCU_Result iResult = BVCSP_GetDialogInfo(m_cspDialog, &cspDialogInfo);
        if (BVCU_Result_SUCCEEDED(iResult))
        {
            m_fileInfo.iCreateTime = cspDialogInfo.iCreateTime;
            m_fileInfo.iOnlineTime = cspDialogInfo.iOnlineTime;
            m_fileInfo.stParam.pRemoteFilePathName = cspDialogInfo.stParam.stFileTarget.pPathFileName;
            if (m_fileInfo.stParam.bUpload)
            {
                m_fileInfo.iSpeedKBpsLongTerm = cspDialogInfo.iVideoKbpsLongTerm;
                m_fileInfo.iSpeedKBpsShortTerm = cspDialogInfo.iVideoKbpsShortTerm_Send;
            }
            else
            {
                m_fileInfo.iSpeedKBpsLongTerm = cspDialogInfo.iVideoKbpsLongTerm;
                m_fileInfo.iSpeedKBpsShortTerm = cspDialogInfo.iVideoKbpsShortTerm;
            }
        }
    }
    return &m_fileInfo;
}

int CFileTransfer::HandleEvent(int iTickCount)
{
    if (m_fileInfo.stParam.bUpload)
    {
        if (m_iStatus == BVFILE_DIALOG_STATUS_TRANSFER)
        {
            if (m_fFile)
            {
                int iDely = iTickCount - m_iCompleTime;
                if (iDely > 1 || m_iCompleTime == 0)
                {
                    int isendcount = 0;
                    if (m_iCompleTime)
                        iDely = (iDely * m_pSession->m_iSendDataCount) >> 10;
                    else iDely = 1;
                    do {
                        char sendBuf[800];
                        int iSendLen = m_pSession->bv_fread(sendBuf, 1, 800, m_fFile);
                        if (iSendLen > 0)
                        {
                            BVCSP_Packet bvcspPacket;
                            memset(&bvcspPacket, 0x00, sizeof(bvcspPacket));
                            bvcspPacket.bKeyFrame = 1;
                            bvcspPacket.iDataType = BVCSP_DATA_TYPE_TSP;
                            bvcspPacket.iDataSize = iSendLen;
                            bvcspPacket.pData = sendBuf;
                            if (BVCU_Result_FAILED(BVCSP_Dialog_Write(m_cspDialog, &bvcspPacket)))
                            {
                                m_pSession->bv_fseek(m_fFile, 0 - iSendLen, SEEK_CUR);
                                break;
                            }
                            else
                            {
                                m_fileInfo.iTransferBytes += iSendLen;
                                m_iLastDataTime = GetTickCount();
                            }
                        }
                        else {
                            m_iStatus = BVFILE_DIALOG_STATUS_SUCCEEDED;
                            break;
                        }
                        ++isendcount;
                    } while (isendcount < iDely);
                    m_iCompleTime = iTickCount;
                }
            }
            else {
                m_iCompleTime = iTickCount;
                if (m_fileInfo.iTransferBytes >= m_fileInfo.iTotalBytes)
                    m_iStatus = BVFILE_DIALOG_STATUS_SUCCEEDED;
                else
                    m_iStatus = BVFILE_DIALOG_STATUS_FAILED;
            }
        }
    }
    else
    {
        if (m_iStatus == BVFILE_DIALOG_STATUS_TRANSFER && m_fileInfo.iTransferBytes >= m_fileInfo.iTotalBytes)
        {
            m_iCompleTime = iTickCount - (MODULE_FILE_TRANSFER_TIMEOUT - 10 * 1000);// 延迟10秒关闭
            m_iStatus = BVFILE_DIALOG_STATUS_SUCCEEDED;
        }
    }
    if (m_iStatus >= BVFILE_DIALOG_STATUS_SUCCEEDED)
    {
        int iDely = iTickCount - m_iCompleTime;
        if (iDely < 0 || iDely > MODULE_FILE_TRANSFER_TIMEOUT)
            return 0;  // 关闭通道
    }
    else if (iTickCount - m_iLastDataTime > MODULE_FILE_TRANSFER_TIMEOUT)
    { // 3分钟没有数据发送/接收，关闭通道
        m_iCompleTime = iTickCount - (MODULE_FILE_TRANSFER_TIMEOUT - 2 * 1000);// 延迟2秒关闭;
        if (m_fileInfo.iTransferBytes >= m_fileInfo.iTotalBytes)
            m_iStatus = BVFILE_DIALOG_STATUS_SUCCEEDED;
        else
            m_iStatus = BVFILE_DIALOG_STATUS_FAILED;
    }
    return 1;
}

BVCU_Result CFileTransfer::OnRecvFrame(BVCSP_Packet* pPacket)
{
    if (m_fileInfo.stParam.bUpload == 0 && m_fFile /*&& m_fileInfo.iTransferBytes < m_fileInfo.iTotalBytes*/)
    {
        m_iLastDataTime = GetTickCount();
        if (pPacket->iDataSize > 0)
        {
            int iWriteLen = m_pSession->bv_fwrite(pPacket->pData, 1, pPacket->iDataSize, m_fFile);
            m_fileInfo.iTransferBytes += pPacket->iDataSize;
            if (iWriteLen >= pPacket->iDataSize)
                return BVCU_RESULT_S_OK;
        }
        else if (pPacket->iDataSize == 0)  // 数据已经和发送方确认传输完毕
        {
            m_iCompleTime = GetTickCount() - (MODULE_FILE_TRANSFER_TIMEOUT - 2 * 1000);// 延迟2秒关闭
            m_iStatus = BVFILE_DIALOG_STATUS_SUCCEEDED;
            return BVCU_RESULT_S_OK;
        }
    }
    return BVCU_RESULT_E_FAILED;
}

void CFileTransfer::OnEvent(int iEvent, BVCU_Result iResult, int bOnbvcspEvent)
{
    if (iEvent == BVCSP_EVENT_DIALOG_CLOSE ||
        (iEvent == BVCSP_EVENT_DIALOG_OPEN && BVCU_Result_FAILED(iResult)))
    {
        m_bClosing = 1;
        if (m_cspDialog)
            BVCSP_Dialog_Close(m_cspDialog);
        m_cspDialog = NULL;
        if (m_fFile)
            m_pSession->bv_fclose(m_fFile);
        m_fFile = NULL;
    }
    else
        m_bCallOpen = 1;
    if (m_fileInfo.stParam.OnEvent) {
        if (iEvent == BVCSP_EVENT_DIALOG_CLOSE)
        {
            if (!m_bCallOpen)
            {
                iEvent = BVCSP_EVENT_DIALOG_OPEN;
                iResult = BVCU_RESULT_E_BADSTATE;
            }
            else if (BVCU_Result_SUCCEEDED(iResult))
            { // 文件没有传输完成，或对方没有接收完成
                if (m_fileInfo.iTransferBytes != m_fileInfo.iTotalBytes)
                    iResult = BVCU_RESULT_E_BADSTATE;
                else if (bOnbvcspEvent == 0 && m_fileInfo.stParam.bUpload)  // 不是接收者主动bye的，很可能没有接收完成。
                    iResult = BVCU_RESULT_S_IGNORE;
            }
        }
        m_fileInfo.stParam.OnEvent((BVCU_File_HTransfer)m_iHandle, m_fileInfo.stParam.pUserData, iEvent, iResult);
    }
}

BVCU_Result CFileTransfer::UpdateLocalFile(BVCSP_DialogParam* pParam)
{
    char szLocalFileAnsi[512] = { 0 };
    if (m_fileInfo.stParam.pLocalFilePathName)
        strncpy_s(m_localFilePathName, sizeof(m_localFilePathName), m_fileInfo.stParam.pLocalFilePathName, _TRUNCATE);
    else
    {
        printf("local file not found\n");
        return BVCU_RESULT_E_NOTFOUND;
    }

    Utf8ToAnsi(szLocalFileAnsi, sizeof(szLocalFileAnsi), m_localFilePathName);
    m_fileInfo.stParam.pLocalFilePathName = m_localFilePathName;
    if (m_fileInfo.stParam.bUpload)
    {  // 下载请求
        m_fFile = m_pSession->bv_fsopen(szLocalFileAnsi, "rb", 0);
        if (!m_fFile)
        {
            printf("fopen file=%s error\n", szLocalFileAnsi);
            return BVCU_RESULT_E_NOTFOUND;
        }

        m_pSession->bv_fseek(m_fFile, 0L, SEEK_END);
        m_iFileSize = m_pSession->bv_ftell(m_fFile);
        if (m_fileInfo.stParam.iFileStartOffset != -1)
        {
            if (m_fileInfo.stParam.iFileStartOffset > m_iFileSize)
                return BVCU_RESULT_E_BADREQUEST;
            m_pSession->bv_fseek(m_fFile, m_fileInfo.stParam.iFileStartOffset, SEEK_SET);
        }
        else {
            m_fileInfo.stParam.iFileStartOffset = 0;
            m_pSession->bv_fseek(m_fFile, 0L, SEEK_SET);
        }
    }
    else
    { // 上传请求
        m_fFile = m_pSession->bv_fsopen(szLocalFileAnsi, "ab+", 1);// _SH_DENYRW);;
        if (m_fFile)
        { // 处理续传
            m_pSession->bv_fseek(m_fFile, 0L, SEEK_END);
            m_iFileSize = m_pSession->bv_ftell(m_fFile);
            if (m_fileInfo.stParam.iFileStartOffset == -1)
                m_fileInfo.stParam.iFileStartOffset = m_iFileSize;
            else {
                m_fileInfo.stParam.iFileStartOffset = 0;
                m_pSession->bv_fseek(m_fFile, 0L, SEEK_SET);
            }
        }
        if (!m_fFile)
        {
            if (m_fileInfo.stParam.iFileStartOffset == -1)
            {// 如果是续传。判断是否正在被下载，是否已经上传完成。
                m_fFile = m_pSession->bv_fsopen(szLocalFileAnsi, "rb", 0);
                if (!m_fFile)
                    return BVCU_RESULT_E_BUSY;
                m_pSession->bv_fseek(m_fFile, 0L, SEEK_END);
                m_fileInfo.stParam.iFileStartOffset = m_pSession->bv_ftell(m_fFile);
                if (m_fileInfo.stParam.iFileStartOffset < pParam->stFileTarget.iEndTime_iFileSize)
                    return BVCU_RESULT_E_BUSY;
            }
            else
                return BVCU_RESULT_E_BUSY;
        }
        m_iFileSize = pParam->stFileTarget.iEndTime_iFileSize;
        if (m_fileInfo.stParam.iFileStartOffset > m_iFileSize)
            return BVCU_RESULT_E_MAXRETRIES; // 续传请求，且本地文件大小大约要传输的文件大小。
    }
    m_fileInfo.iTransferBytes = m_fileInfo.stParam.iFileStartOffset;
    m_fileInfo.iTotalBytes = m_iFileSize;
    // 设置回复参数
    pParam->stFileTarget.iEndTime_iFileSize = m_iFileSize;
    pParam->stFileTarget.iStartTime_iOffset = m_fileInfo.stParam.iFileStartOffset;
    pParam->afterRecv = CFileTransManager::OnAfterRecv_BVCSP;
    pParam->OnEvent = CFileTransManager::OnDialogEvent_BVCSP;
    pParam->pUserData = this;
    m_iStatus = BVFILE_DIALOG_STATUS_INVITING;
    if (m_fileInfo.iTransferBytes >= m_fileInfo.iTotalBytes)
    {
        if (m_fFile)
            m_pSession->bv_fclose(m_fFile);
        m_fFile = NULL;
    }
    m_iLastDataTime = GetTickCount();
    return BVCU_RESULT_S_OK;
}

int CFileTransfer::BuildFileData()
{
    // 更新当前 媒体方向、编码参数、等信息
    BVCSP_DialogInfo cspDialogInfo;
    BVCU_Result iResult2 = BVCSP_GetDialogInfo(m_cspDialog, &cspDialogInfo);
    if (BVCU_Result_SUCCEEDED(iResult2))
    {
        m_fileInfo.iCreateTime = cspDialogInfo.iCreateTime;
        m_fileInfo.iOnlineTime = cspDialogInfo.iOnlineTime;
        m_fileInfo.stParam.pRemoteFilePathName = cspDialogInfo.stParam.stFileTarget.pPathFileName;
        if (cspDialogInfo.stParam.stFileTarget.iStartTime_iOffset != -1 && m_fileInfo.stParam.iFileStartOffset == -1)
            m_fileInfo.stParam.iFileStartOffset = m_fileInfo.iTransferBytes = cspDialogInfo.stParam.stFileTarget.iStartTime_iOffset;
        else if (cspDialogInfo.stParam.stFileTarget.iStartTime_iOffset != m_fileInfo.stParam.iFileStartOffset)
            m_fileInfo.stParam.iFileStartOffset = m_fileInfo.iTransferBytes = 0;
        if (m_fileInfo.stParam.bUpload == 0)
            m_iFileSize = cspDialogInfo.stParam.stFileTarget.iEndTime_iFileSize;
        m_fileInfo.iTotalBytes = m_iFileSize;
        if (!m_fileInfo.stParam.szTargetID[0] || strchr(m_fileInfo.stParam.szTargetID, '_')) // 被动的用户ID不能替换掉
            strncpy_s(m_fileInfo.stParam.szTargetID, sizeof(m_fileInfo.stParam.szTargetID), cspDialogInfo.stParam.stTarget.szID, _TRUNCATE);
        if (m_fFile)
        {
            if (m_fileInfo.iTransferBytes >= m_fileInfo.iTotalBytes)
            {
                m_pSession->bv_fclose(m_fFile);
                m_fFile = NULL;
            }
            else
                m_pSession->bv_fseek(m_fFile, m_fileInfo.iTransferBytes, SEEK_SET);
        }
        m_iStatus = BVFILE_DIALOG_STATUS_TRANSFER;
        m_iLastDataTime = GetTickCount();
    }
    return 1;
}

BVCU_Result CFileTransManager::OnDialogCmd_BVCSP(BVCSP_HDialog hDialog, int iEventCode, BVCSP_DialogParam* pParam)
{
    if (iEventCode == BVCSP_EVENT_DIALOG_OPEN)
    {
        if (!pParam->stFileTarget.pPathFileName)
            return BVCU_RESULT_E_BADREQUEST;
        CFileTransfer* pFileTransfer = AddFileTransfer();
        if (pFileTransfer == NULL)
            return BVCU_RESULT_E_BUSY;
        BVCU_Result iResult = BVCU_RESULT_E_INVALIDARG;
        if (pFileTransfer->SetInfo_RecvReq(pParam))
        {
            iResult = OnFileRequest((BVCU_File_HTransfer)pFileTransfer->GetHandle(), pFileTransfer->GetParam());
            if (BVCU_Result_SUCCEEDED(iResult))
            {
                iResult = pFileTransfer->UpdateLocalFile(pParam);
            }
        }
        if (BVCU_Result_SUCCEEDED(iResult))
        {
            pFileTransfer->SetCSPDialog(hDialog);
            return BVCU_RESULT_S_OK;
        }
        else
            RemoveFileTransfer(pFileTransfer);
        return iResult;
    }
    else if (iEventCode == BVCSP_EVENT_DIALOG_CLOSE)
    {
        BVCSP_Event_DialogCmd stParam;
        memset(&stParam, 0x00, sizeof(stParam));
        OnDialogEvent_BVCSP(hDialog, BVCSP_EVENT_DIALOG_CLOSE, &stParam);
        return BVCU_RESULT_S_OK;
    }
    else  return BVCU_RESULT_E_BADREQUEST;
    return BVCU_RESULT_E_SEVERINTERNAL;
}
void CFileTransManager::OnDialogEvent_BVCSP(BVCSP_HDialog hDialog, int iEventCode, BVCSP_Event_DialogCmd* pParam)
{
    if (pParam == NULL)
        return;
    BVCSP_DialogParam* pDialogParam = pParam->pDialogParam;
    CPUSessionBase* pSession = (CPUSessionBase*)pDialogParam->pUserData;
    if (pSession == 0)
        return;
    CFileTransManager* pManager = pSession->GetFileManager();
    if (pManager == 0)
        return;
    CFileTransfer* pFileTransfer = pManager->FindFileTransfer(hDialog);
    if (pFileTransfer && !pFileTransfer->bClosing())
    {
        printf("OnFileEvent:%X eventCode:%d result:%d \n",
             pFileTransfer->GetHandle(), iEventCode, pParam->iResult);
        if (iEventCode != BVCSP_EVENT_DIALOG_CLOSE && BVCU_Result_SUCCEEDED(pParam->iResult))
            pFileTransfer->BuildFileData();
        pFileTransfer->OnEvent(iEventCode, pParam->iResult, 1);
        if (pFileTransfer->bClosing())
            pManager->RemoveFileTransfer(pFileTransfer);
    }
}
BVCU_Result CFileTransManager::OnAfterRecv_BVCSP(BVCSP_HDialog hDialog, BVCSP_Packet* pPacket)
{
    BVCSP_DialogInfo info;
    if (BVCU_Result_FAILED(BVCSP_GetDialogInfo(hDialog, &info)))
        return BVCU_RESULT_E_BADREQUEST;

    CPUSessionBase* pSession = (CPUSessionBase*)info.stParam.pUserData;
    if (pSession == 0)
        return BVCU_RESULT_E_BADREQUEST;
    CFileTransManager* pManager = pSession->GetFileManager();
    if (pManager == 0)
        return BVCU_RESULT_E_BADREQUEST;
    CFileTransfer* pFileTransfer = pManager->FindFileTransfer(hDialog);
    if (pFileTransfer && !pFileTransfer->bClosing())
    {
        return pFileTransfer->OnRecvFrame(pPacket);
    }
    return BVCU_RESULT_E_BADREQUEST;
}

CFileTransManager::CFileTransManager()
{
    m_iSendDataCount = 5120; // 4000 KBps
    m_iBandwidthLimit = 100*1024;  // 100 Mbps
    memset(m_fileList, 0x00, sizeof(m_fileList));
}
CFileTransManager::~CFileTransManager()
{
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++ i)
    {
        if (m_fileList[i])
        {
            delete m_fileList[i];
            m_fileList[i] = 0;
        }
    }
}
int CFileTransManager::IsFileTransferInList(CFileTransfer* pFileTransfer)
{
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
    {
        if (m_fileList[i] && m_fileList[i] == pFileTransfer)
        {
            return 1;
        }
    }
    return 0;
}
CFileTransfer* CFileTransManager::AddFileTransfer()
{
    CFileTransfer* pFileTransfer = NULL;
    int index = -1;
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
    {
        if (m_fileList[i] == 0)
        {
            index = i;
            break;
        }
    }
    if (index >= 0)
    {
        pFileTransfer = new CFileTransfer();
        if (pFileTransfer == 0)
            return 0;
        m_fileList[index] = pFileTransfer;
        pFileTransfer->Init(this);
        pFileTransfer->SetHandle(index+1);

        if (m_iBandwidthLimit > 0)
        {  // 文件传输带宽限制，计算每路带宽
            int iCount = GetFileTransferCount();
            m_iSendDataCount = (m_iBandwidthLimit << 2) / (25 * iCount); // kbps*1024/8/800/count
        }
    }
    return pFileTransfer;
}
int CFileTransManager::RemoveFileTransfer(CFileTransfer* pFileTransfer)
{
    int index = -1;
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
    {
        if (m_fileList[i] == pFileTransfer)
        {
            index = i;
            break;
        }
    }
    if (index >= 0)
    {
        m_fileList[index] = NULL;
        pFileTransfer->Init(NULL);
        delete pFileTransfer;

        if (m_iBandwidthLimit > 0)
        {  // 文件传输带宽限制，计算每路带宽
            int iCount = GetFileTransferCount();
            m_iSendDataCount = (m_iBandwidthLimit << 2) / (25 * iCount); // kbps*1024/8/800/count
        }
        return 1;
    }
    return 0;
}
CFileTransfer* CFileTransManager::FindFileTransfer(BVCSP_HDialog hdialog)
{
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
    {
        if (m_fileList[i] && m_fileList[i]->GetCSPDialog() == hdialog)
        {
            return m_fileList[i];
        }
    }
    return 0;
}
int CFileTransManager::GetFileTransferCount()
{
    int count = 0;
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
    {
        if (m_fileList[i])
        {
            count++;
        }
    }
    return count;
}
int CFileTransManager::HandleEvent()
{
    int iLastTick = GetTickCount();
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
    {
        if (m_fileList[i])
        {
            m_fileList[i]->HandleEvent(iLastTick);
        }
    }
    return 0;
}
