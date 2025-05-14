﻿#pragma once
#include <stdio.h>
#include "BVCSP.h"

#define MAX_FILE_TRANSFER_COUNT 512
#define MODULE_FILE_TRANSFER_TIMEOUT (2 * 60 * 1000)

struct bvFileInfo
{
    char *user;        // 用户账户, 产生录像时登录账户，为空时后台使用上传录像者账户.
    int64_t starttime; // 开始时间(UTC)， （必须）
    int64_t endtime;   // 录像结束时间(UTC)
    int channel;       // 通道号,默认0
    char *filetype;    // "video"、"audio"、"image"
    int lat;           // 经度
    int lng;           // 维度
    char *desc1;       // 文件描述1
    char *desc2;       // 文件描述2
    bool mark;         // 是否是重点标记文件
};

typedef void *BVCU_File_HTransfer;
class CFileTransfer;
typedef struct _BVCU_File_TransferParam BVCU_File_TransferParam;
class CFileTransManager
{
public:
    /* 收到文件传输请求回调
    hTransfer：文件传输的句柄。
    pParam: 每个事件对应的参数，具体类型参考各个事件码的说明。如果pParam是NULL，表示无参数。
    返回：BVCU_RESULT_S_OK表示应用程序接受该传输请求，其他值表示拒绝。同情请求时注意填写pParam中需要填写的值。
    */
    virtual bvFileInfo *OnFileRequest(BVCU_File_HTransfer hTransfer, char *pLocalFilePathName) = 0;
    virtual void OnFileEvent(BVCU_File_HTransfer hTransfer, void *pUserData, int iEventCode, BVCU_Result iResult) = 0;

    void SetBandwidthLimit(int iBandwidthLimit) { m_iBandwidthLimit = iBandwidthLimit; } // 带宽限制。单位kbps，0表示无限制，建议限制文件传输的带宽。
public:
    CFileTransManager();
    virtual ~CFileTransManager();
    CFileTransfer **GetFileTransferList() { return m_fileList; }
    int IsFileTransferInList(CFileTransfer *pFileTransfer);
    CFileTransfer *AddFileTransfer();
    int RemoveFileTransfer(CFileTransfer *pFileTransfer);
    CFileTransfer *FindFileTransferByDlg(BVCSP_HDialog hdialog);
    CFileTransfer *FindFileTransfer(BVCU_File_HTransfer hfile);

    int GetFileTransferCount();
    int FileInfo2Json(const bvFileInfo *fileInfo, char *buf, int bufsize);

    int HandleEvent();

    int m_iSendDataCount; // 全局限速。1024pms ; (800*m_iSendDataCount) Bps
    BVCU_PUCFG_DeviceInfo *m_deviceInfo;

public:
    // BVCSP Event
    BVCU_Result OnDialogCmd_BVCSP(BVCSP_HDialog hDialog, int iEventCode, BVCSP_DialogParam *pParam);
    static void OnDialogEvent_BVCSP(BVCSP_HDialog hDialog, int iEventCode, BVCSP_Event_DialogCmd *pParam);
    static BVCU_Result OnAfterRecv_BVCSP(BVCSP_HDialog hDialog, BVCSP_Packet *pPacket);

private:
    CFileTransfer *m_fileList[MAX_FILE_TRANSFER_COUNT];
    int m_iBandwidthLimit; // 带宽限制。单位kbps，0表示无限制，建议限制文件传输的带宽。
};

enum BVFile_Dialog_Status
{
    BVFILE_DIALOG_STATUS_NONE = 0,  // 没有状态
    BVFILE_DIALOG_STATUS_INVITING,  // 在建立链接中
    BVFILE_DIALOG_STATUS_TRANSFER,  // 在传输中
    BVFILE_DIALOG_STATUS_SUCCEEDED, // 传输成功
    BVFILE_DIALOG_STATUS_FAILED,    // 传输失败
};
// 文件传输参数设置。收到传输请求并同意时：szLocalFileName，OnEvent需要填写。
typedef struct _BVCU_File_TransferParam
{
    int iSize;                            // 本结构体的大小，分配者应初始化为sizeof(BVCU_File_TransferParam)
    void *pUserData;                      // 用户自定义数据。BVCU_File_GlobalParam.OnFileRequest中可以填写。
    char szTargetID[BVCU_MAX_ID_LEN + 1]; // 文件传输目标对象ID。PU/NRU。"NRU_"时库内部会在回调前填写具体NRU ID。
    const bvFileInfo *fileInfo;           // 文件信息。
    char *pLocalFilePathName;             // 本地路径+文件名。BVCU_File_GlobalParam.OnFileRequest中要填写。
    unsigned int iFileStartOffset;        // 文件开始传输偏移。0：重新传输。-1(0xffffffff)：库内自动处理续传。
    int iTimeOut;                         // 连接超时间。单位 毫秒。
    int bUpload;                          // 0-下载，1-上传。
} BVCU_File_TransferParam;

// 文件传输信息
typedef struct _BVCU_File_TransferInfo
{
    // 传输参数
    BVCU_File_TransferParam stParam;

    // Transfer开始时刻，从1970-01-01 00:00:00 +0000 (UTC)开始的微秒数
    long long iCreateTime;

    // Transfer持续时间，单位微秒
    long long iOnlineTime;

    // 已经传输的字节数，包含BVCU_File_TransferParam.iFileStartOffset。和iTotalBytes一起计算出当前传输进度。
    unsigned int iTransferBytes;
    // 总字节数
    unsigned int iTotalBytes;

    int iSpeedKBpsLongTerm;  // 长时间传输速率，单位 KBytes/second
    int iSpeedKBpsShortTerm; // 短时间传输速率，单位 KBytes/second

} BVCU_File_TransferInfo;

class CFileTransfer
{
protected:
    BVCU_File_TransferInfo m_fileInfo;

    BVCSP_HDialog m_cspDialog;
    BVFile_Dialog_Status m_iStatus;
    FILE *m_fFile;
    unsigned int m_iFileSize;
    int m_iLastDataTime; // 最后成功发送/接收数据时间。几分钟无法发送/接收数据，关闭通道。
    int m_iCompleTime;   // 完成传输时间，用来判断通道超时。传输过程中用来控制速率。
    char m_localFilePathName[BVCU_MAX_FILE_NAME_LEN + 1];
    unsigned long m_iHandle;
    CFileTransManager *m_pOwn;
    unsigned short m_bClosing;
    unsigned short m_bCallOpen;

public:
    CFileTransfer();
    ~CFileTransfer() { Init(NULL); }
    void Init(CFileTransManager *pOwn);
    CFileTransManager *GetOwn() { return m_pOwn; }
    int bClosing() { return m_bClosing; }
    int SetInfo(BVCU_File_TransferParam *pParam);   // 主动
    int SetInfo_RecvReq(BVCSP_DialogParam *pParam); // 被动
    BVCU_File_TransferInfo *GetInfo() { return &m_fileInfo; }
    BVCU_File_TransferParam *GetParam() { return &m_fileInfo.stParam; }
    BVCU_File_TransferInfo *GetInfoNow(int bNetworkThread);
    int GetFileSize() { return m_iFileSize; }

    void SetHandle(unsigned long iHandle) { m_iHandle = iHandle; }
    unsigned long GetHandle() { return m_iHandle; }

    void SetCSPDialog(BVCSP_HDialog hDialog) { m_cspDialog = hDialog; }
    BVCSP_HDialog GetCSPDialog() { return m_cspDialog; }

    int HandleEvent(int iTickCount); // true:keep，false:destroy

    BVCU_Result OnRecvFrame(BVCSP_Packet *pPacket);
    void OnEvent(int iEvent, BVCU_Result iResult, int bOnbvcspEvent = 0);

    // 回复 平台下载文件请求
    BVCU_Result UpdateLocalFile(BVCSP_DialogParam *pParam, const bvFileInfo *fileInfo); // 被动
    int BuildFileData();
};
