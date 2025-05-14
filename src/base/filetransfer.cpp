
#include <string>
#include "../utils/utils.h"
#include "session.h"
#include "filetransfer.h"
#include "cfile.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

extern bv_file* g_file; // cfile.cpp中定义的文件读写接口

CFileTransfer::CFileTransfer()
    : m_cspDialog(NULL), m_iStatus(BVFILE_DIALOG_STATUS_NONE), m_fFile(NULL), m_iFileSize(0), m_iCompleTime(0), m_iHandle(0), m_pOwn(NULL), m_bClosing(0), m_bCallOpen(0)
{
    memset(&m_fileInfo, 0x00, sizeof(m_fileInfo));
    m_localFilePathName[0] = '\0';
}

void CFileTransfer::Init(CFileTransManager* pOwn)
{
    if (m_cspDialog)
    {
        // BVCSP_Dialog_Close(m_cspDialog); // BVCU_Finsh()会死锁。
        m_cspDialog = NULL;
    }
    if (m_fFile)
    {
        g_file->fclose(m_fFile);
        m_fFile = NULL;
    }
    m_iStatus = BVFILE_DIALOG_STATUS_NONE;
    m_iFileSize = 0;
    m_iCompleTime = 0;
    m_iLastDataTime = GetTickCount();
    m_iHandle = 0;
    m_pOwn = pOwn;
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
    strncpy_s(m_localFilePathName, sizeof(m_localFilePathName), szLocalFileAnsi, _TRUNCATE);
    if (m_fileInfo.stParam.bUpload == 0)
    { // 下载文件
        m_fFile = g_file->fsopen(szLocalFileAnsi, "ab+", 1);
        if (m_fFile)
        { // 处理续传
            g_file->fseek(m_fFile, 0L, SEEK_END);
            m_iFileSize = g_file->ftell(m_fFile);
            if ((int)m_fileInfo.stParam.iFileStartOffset == -1) // == -1
            {
                m_fileInfo.stParam.iFileStartOffset = m_iFileSize;
                m_fileInfo.iTransferBytes = m_iFileSize;
            }
            else
            {
                m_fileInfo.stParam.iFileStartOffset = 0;
                if (m_iFileSize > 0) // 重新传输，删除老文件
                {
                    fclose(m_fFile);
                    m_fFile = g_file->fsopen(szLocalFileAnsi, "wb+", TRUE); // _SH_DENYRW);;
                }
            }
        }
    }
    else
    { // 上传文件，获取文件大小
        m_fFile = g_file->fsopen(szLocalFileAnsi, "rb", 0);
        if (m_fFile)
        {
            g_file->fseek(m_fFile, 0L, SEEK_END);
            m_iFileSize = g_file->ftell(m_fFile);
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
    m_fileInfo.stParam.pLocalFilePathName = pParam->stFileTarget.pPathFileName;
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
            m_fileInfo.stParam.pLocalFilePathName = cspDialogInfo.stParam.stFileTarget.pPathFileName;
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
        else
        {
            m_fileInfo.stParam.pLocalFilePathName = NULL;
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
                        iDely = (iDely * m_pOwn->m_iSendDataCount) >> 10;
                    else
                        iDely = 1;
                    do
                    {
                        char sendBuf[800];
                        int iSendLen = g_file->fread(sendBuf, 1, 800, m_fFile);
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
                                g_file->fseek(m_fFile, 0 - iSendLen, SEEK_CUR);
                                break;
                            }
                            else
                            {
                                m_fileInfo.iTransferBytes += iSendLen;
                                m_iLastDataTime = GetTickCount();
                            }
                        }
                        else
                        {
                            m_iStatus = BVFILE_DIALOG_STATUS_SUCCEEDED;
                            break;
                        }
                        ++isendcount;
                    } while (isendcount < iDely);
                    m_iCompleTime = iTickCount;
                }
            }
            else
            {
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
            m_iCompleTime = iTickCount - (MODULE_FILE_TRANSFER_TIMEOUT - 10 * 1000); // 延迟10秒关闭
            m_iStatus = BVFILE_DIALOG_STATUS_SUCCEEDED;
        }
    }
    if (m_iStatus >= BVFILE_DIALOG_STATUS_SUCCEEDED)
    {
        int iDely = iTickCount - m_iCompleTime;
        if (iDely < 0 || iDely > MODULE_FILE_TRANSFER_TIMEOUT)
            return 0; // 关闭通道
    }
    else if (iTickCount - m_iLastDataTime > MODULE_FILE_TRANSFER_TIMEOUT)
    {                                                                           // 3分钟没有数据发送/接收，关闭通道
        m_iCompleTime = iTickCount - (MODULE_FILE_TRANSFER_TIMEOUT - 2 * 1000); // 延迟2秒关闭;
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
            int iWriteLen = g_file->fwrite(pPacket->pData, 1, pPacket->iDataSize, m_fFile);
            m_fileInfo.iTransferBytes += pPacket->iDataSize;
            if (iWriteLen >= pPacket->iDataSize)
                return BVCU_RESULT_S_OK;
        }
        else if (pPacket->iDataSize == 0) // 数据已经和发送方确认传输完毕
        {
            m_iCompleTime = GetTickCount() - (MODULE_FILE_TRANSFER_TIMEOUT - 2 * 1000); // 延迟2秒关闭
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
        if (m_fFile)
            g_file->fclose(m_fFile);
        m_fFile = NULL;
    }
    else
        m_bCallOpen = 1;

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
            else if (bOnbvcspEvent == 0 && m_fileInfo.stParam.bUpload) // 自己主动关闭的，接收者没有发送eof过来，失败（不再兼容2020年5月之前版本服务器）。
                iResult = BVCU_RESULT_E_MAXRETRIES;
        }
    }
    m_pOwn->OnFileEvent((BVCU_File_HTransfer)m_iHandle, m_fileInfo.stParam.pUserData, iEvent, iResult);
    if (m_bClosing == TRUE)
    {
        if (m_cspDialog) // onevent 之后才能关闭，不然 pRemoteFilePathName 无效。
            BVCSP_Dialog_Close(m_cspDialog);
        m_cspDialog = NULL;
    }
}

BVCU_Result CFileTransfer::UpdateLocalFile(BVCSP_DialogParam* pParam, const bvFileInfo* fileInfo)
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
    { // 下载请求
        m_fFile = g_file->fsopen(szLocalFileAnsi, "rb", 0);
        if (!m_fFile)
        {
            printf("fopen file=%s error\n", szLocalFileAnsi);
            return BVCU_RESULT_E_NOTFOUND;
        }

        g_file->fseek(m_fFile, 0L, SEEK_END);
        m_iFileSize = g_file->ftell(m_fFile);
        if (m_fileInfo.stParam.iFileStartOffset != -1)
        {
            if (m_fileInfo.stParam.iFileStartOffset > m_iFileSize)
                return BVCU_RESULT_E_BADREQUEST;
            g_file->fseek(m_fFile, m_fileInfo.stParam.iFileStartOffset, SEEK_SET);
        }
        else
        {
            m_fileInfo.stParam.iFileStartOffset = 0;
            g_file->fseek(m_fFile, 0L, SEEK_SET);
        }
    }
    else
    {                                                        // 上传请求
        m_fFile = g_file->fsopen(szLocalFileAnsi, "ab+", 1); // _SH_DENYRW);;
        if (m_fFile)
        { // 处理续传
            g_file->fseek(m_fFile, 0L, SEEK_END);
            m_iFileSize = g_file->ftell(m_fFile);
            if (m_fileInfo.stParam.iFileStartOffset == -1) // 续传
                m_fileInfo.stParam.iFileStartOffset = m_iFileSize;
            else
            {
                m_fileInfo.stParam.iFileStartOffset = 0;
                if (m_iFileSize > 0) // 重新传输，删除老文件
                {
                    fclose(m_fFile);
                    m_fFile = g_file->fsopen(szLocalFileAnsi, "wb+", TRUE); // _SH_DENYRW);;
                }
            }
        }
        if (!m_fFile)
        {
            if (m_fileInfo.stParam.iFileStartOffset == -1)
            { // 如果是续传。判断是否正在被下载，是否已经上传完成。
                m_fFile = g_file->fsopen(szLocalFileAnsi, "rb", 0);
                if (!m_fFile)
                    return BVCU_RESULT_E_BUSY;
                g_file->fseek(m_fFile, 0L, SEEK_END);
                m_fileInfo.stParam.iFileStartOffset = g_file->ftell(m_fFile);
                if (m_fileInfo.stParam.iFileStartOffset < pParam->stFileTarget.iEndTime_iFileSize)
                    return BVCU_RESULT_E_BUSY;
            }
            else
                return BVCU_RESULT_E_BUSY;
        }
        m_iFileSize = pParam->stFileTarget.iEndTime_iFileSize;
        if (m_fileInfo.stParam.iFileStartOffset > m_iFileSize)
            return BVCU_RESULT_E_MAXRETRIES; // 续传请求，且本地文件大小大于要传输的文件大小。
    }
    m_fileInfo.iTransferBytes = m_fileInfo.stParam.iFileStartOffset;
    m_fileInfo.iTotalBytes = m_iFileSize;
    // 设置回复参数
    pParam->stFileTarget.iEndTime_iFileSize = m_iFileSize;
    pParam->stFileTarget.iStartTime_iOffset = m_fileInfo.stParam.iFileStartOffset;
    if (fileInfo)
    {
        m_pOwn->FileInfo2Json(fileInfo, szLocalFileAnsi, sizeof(szLocalFileAnsi));
        pParam->stFileTarget.pFileInfoJson = szLocalFileAnsi;
    }
    m_iStatus = BVFILE_DIALOG_STATUS_INVITING;
    if (m_fileInfo.iTransferBytes >= m_fileInfo.iTotalBytes)
    {
        if (m_fFile)
            g_file->fclose(m_fFile);
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
        m_fileInfo.stParam.pLocalFilePathName = cspDialogInfo.stParam.stFileTarget.pPathFileName;
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
                g_file->fclose(m_fFile);
                m_fFile = NULL;
            }
            else
                g_file->fseek(m_fFile, m_fileInfo.iTransferBytes, SEEK_SET);
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
            const bvFileInfo* fileInfo = OnFileRequest((BVCU_File_HTransfer)pFileTransfer->GetHandle(), pFileTransfer->GetParam()->pLocalFilePathName);
            if (fileInfo)
            {
                iResult = pFileTransfer->UpdateLocalFile(pParam, fileInfo);
            }
        }

        pParam->pUserData = this;
        pParam->afterRecv = CFileTransManager::OnAfterRecv_BVCSP;
        pParam->OnEvent = CFileTransManager::OnDialogEvent_BVCSP;
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
    else
        return BVCU_RESULT_E_BADREQUEST;
    return BVCU_RESULT_E_SEVERINTERNAL;
}
void CFileTransManager::OnDialogEvent_BVCSP(BVCSP_HDialog hDialog, int iEventCode, BVCSP_Event_DialogCmd* pParam)
{
    if (pParam == NULL)
        return;
    BVCSP_DialogParam* pDialogParam = pParam->pDialogParam;
    CFileTransManager* pManager = (CFileTransManager*)pDialogParam->pUserData;
    if (pManager == 0)
        return;
    CFileTransfer* pFileTransfer = pManager->FindFileTransferByDlg(hDialog);
    if (pFileTransfer && !pFileTransfer->bClosing())
    {
        printf("OnFileEvent:%d eventCode:%d result:%d \n",
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

    CFileTransManager* pManager = (CFileTransManager*)info.stParam.pUserData;
    if (pManager == 0)
        return BVCU_RESULT_E_BADREQUEST;
    CFileTransfer* pFileTransfer = pManager->FindFileTransferByDlg(hDialog);
    if (pFileTransfer && !pFileTransfer->bClosing())
    {
        return pFileTransfer->OnRecvFrame(pPacket);
    }
    return BVCU_RESULT_E_BADREQUEST;
}

CFileTransManager::CFileTransManager()
{
    m_iSendDataCount = 51200;       // 40000 KBps
    m_iBandwidthLimit = 100 * 1024; // 100 Mbps
    memset(m_fileList, 0x00, sizeof(m_fileList));
}
CFileTransManager::~CFileTransManager()
{
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
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
        pFileTransfer->SetHandle(index + 1);

        if (m_iBandwidthLimit > 0)
        { // 文件传输带宽限制，计算每路带宽
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

        int iCount = GetFileTransferCount();
        if (m_iBandwidthLimit > 0 && iCount > 0)
        {                                                                // 文件传输带宽限制，计算每路带宽
            m_iSendDataCount = (m_iBandwidthLimit << 2) / (25 * iCount); // kbps*1024/8/800/count
        }
        return 1;
    }
    return 0;
}
CFileTransfer* CFileTransManager::FindFileTransferByDlg(BVCSP_HDialog hdialog)
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
CFileTransfer* CFileTransManager::FindFileTransfer(BVCU_File_HTransfer hfile)
{
    for (int i = 0; i < MAX_FILE_TRANSFER_COUNT; ++i)
    {
        if (m_fileList[i] && (BVCU_File_HTransfer)m_fileList[i]->GetHandle() == hfile)
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
int CFileTransManager::FileInfo2Json(const bvFileInfo* fileInfo, char* buf, int bufsize)
{
    if (fileInfo)
    {
        char temp[512];
        int lsize = sprintf(temp, "{\n\"id\":\"%s\",\"starttime\":%lld,\"endtime\":%lld,\"channel\":%d,\"lat\":%d,\"lng\":%d",
            m_deviceInfo->szID, fileInfo->starttime, fileInfo->endtime, fileInfo->channel, fileInfo->lat, fileInfo->lng);
        if (fileInfo->user)
            lsize += sprintf(temp + lsize, ",\"user\":\"%s\"", fileInfo->user);
        if (fileInfo->filetype)
            lsize += sprintf(temp + lsize, ",\"filetype\":\"%s\"", fileInfo->filetype);
        if (fileInfo->desc1)
            lsize += sprintf(temp + lsize, ",\"desc1\":\"%s\"", fileInfo->desc1);
        if (fileInfo->desc2)
            lsize += sprintf(temp + lsize, ",\"desc2\":\"%s\"", fileInfo->desc2);
        if (fileInfo->mark)
            lsize += sprintf(temp + lsize, ",\"mark\":true");
        lsize += sprintf(temp + lsize, "}");
        lsize = base64_encode(temp, lsize, buf, bufsize);
        return lsize;
    }
    return 0;
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
