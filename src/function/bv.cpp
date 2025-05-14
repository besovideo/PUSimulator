
#include <stdint.h>
#include <iostream>
#include <string>
#include "../utils/utils.h"
#include "BVCSP.h"
#include "BVAuth.h"
#include "../base/cfile.h"
#include "../base/session.h"
#include "../base/dialog.h"
#include "../base/filetransfer.h"
#include "bv.h"

bvSessionParam g_sessionParam; // 登录相关参数
bvChannelParam g_channelParam; // 音视频通道参数
bvFileParam g_fileParam;       // 文件接口

// 音视频 通道
class CMediaChannel : public CAVChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnSetName(const char* name) { return BVCU_RESULT_S_OK; } // 收到配置通道名称请求
    virtual BVCU_Result OnOpenRequest(BVCSP_DialogParam* pParam)                                          // 收到打开请求，回复是否同意，0：同意
    {
        BVCSP_VideoCodec* videocodec = &pParam->szMyselfVideo;
        memset(videocodec, 0x00, sizeof(*videocodec));
        videocodec->codec = SAVCODEC_ID_H264;
        if (strcmpi(g_channelParam.szVideoCodec, "H265") == 0)
            videocodec->codec = SAVCODEC_ID_H265;
        BVCSP_AudioCodec* audiocodec = &pParam->szMyselfAudio;
        memset(audiocodec, 0x00, sizeof(*audiocodec));
        audiocodec->codec = SAVCODEC_ID_G711A;
        if (strcmpi(g_channelParam.szAudioCodec, "G726") == 0)
            audiocodec->codec = SAVCODEC_ID_G726;
        else if (strcmpi(g_channelParam.szAudioCodec, "AAC") == 0)
            audiocodec->codec = SAVCODEC_ID_AAC;
        audiocodec->iSampleRate = g_channelParam.iSampleRate;
        audiocodec->iBitrate = g_channelParam.iBitrate;
        audiocodec->iChannelCount = g_channelParam.iChannelCount;
        audiocodec->iBitrate = g_channelParam.iBitrate;
        pParam->szTargetAudio = pParam->szMyselfAudio;
        if (bOpened)
            g_channelParam.OnOpen(BNeedVideoIn(), BNeedAudioIn(), BNeedAudioOut());

        return BVCU_RESULT_S_OK;
    }
    virtual void OnOpen() // 建立通道连接成功通知
    {
        g_channelParam.OnOpen(BNeedVideoIn(), BNeedAudioIn(), BNeedAudioOut());
        bOpened = true;
    }
    virtual void OnClose() // 通道连接关闭通知
    {
        g_channelParam.OnClose();
        bOpened = false;
    }
    virtual void OnPLI() { g_channelParam.OnPLI(); }                                                                   // 收到生成关键帧请求
    virtual void OnRecvAudio(long long iPTS, const void* pkt, int len) { g_channelParam.OnRecvAudio(iPTS, pkt, len); } // 收到平台发来的音频数据。编码信息同ReplySDP()。
    virtual BVCU_Result OnPTZCtrl(const BVCU_PUCFG_PTZControl* ptzCtrl) { return g_channelParam.OnPTZCtrl(ptzCtrl); }  // 收到平台发来的云台控制命令。
    virtual BVCU_PUCFG_PTZAttr* OnGetPTZParam() { return g_channelParam.OnGetPTZParam(); }                             // 收到平台发来的云台查询命令。

public:
    bool bOpened; // 是否已经建立通道
    CMediaChannel(bool bVideoIn, bool bAudioIn, bool bAudioOut, bool ptz) : CAVChannelBase(bVideoIn, bAudioIn, bAudioOut, ptz) { bOpened = false; }
    virtual ~CMediaChannel() {}
};

class CMyFileTrans : public CFileTransManager
{
public:
    virtual bvFileInfo* OnFileRequest(BVCU_File_HTransfer hTransfer, char* pLocalFilePathName)
    {
        if (g_fileParam.OnFileRequest)
            return g_fileParam.OnFileRequest(hTransfer, pLocalFilePathName);
    };
    virtual void OnFileEvent(BVCU_File_HTransfer hTransfer, void* pUserData, int iEventCode, BVCU_Result iResult)
    {
        if (g_fileParam.OnFileEvent)
            g_fileParam.OnFileEvent(hTransfer, pUserData, iEventCode, iResult);
    };

public:
    CMyFileTrans() {};
    virtual ~CMyFileTrans() {};
};

class CPUSession : public CPUSessionBase
{ // 设备对象
public:
    CPUSession(const char* ID) : CPUSessionBase(ID) {};
    ~CPUSession() {};

private:
    virtual BVCU_Result OnSetInfo(const char* name, int lat, int lng) { return BVCU_RESULT_E_UNSUPPORTED; }
    virtual void OnLoginEvent(BVCU_Result iResult) { g_sessionParam.OnLoginEvent(iResult); }
    virtual void OnOfflineEvent(BVCU_Result iResult) { g_sessionParam.OnOfflineEvent(iResult); }
    virtual void OnCommandReply(BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam) { g_sessionParam.OnCommandReply(pCommand, pParam); };
    virtual BVCU_PUCFG_RecordStatus* OnGetRecordStatus()
    { // 获取录像状态
        if (g_fileParam.OnGetRecordStatus)
            return g_fileParam.OnGetRecordStatus();
    }
    virtual BVCU_Search_Response* OnGetRecordFiles(BVCU_Search_Request* req)
    { // 获取录像文件列表
        if (g_fileParam.OnGetRecordFiles)
            return g_fileParam.OnGetRecordFiles(req);
    };
    virtual BVCU_Result OnManualRecord(bool bStart)
    { // 开始/停止 手动录像.
        if (g_fileParam.OnManualRecord)
            return g_fileParam.OnManualRecord(bStart);
    };
};

// ================  实现部分 ==================
CMediaChannel* g_mediaChannel = 0;
CMyFileTrans* g_fileManager = 0;
CPUSession* g_session = 0;
bool g_bRun = true;

// BVCSP日志回调
void Log_Callback(int level, const char* log)
{
    printf("[BVCSP LOG] %s\n", log);
}

// 为库提供线程，库是单线程程序，一些和库可能冲突的操作可以放这里，不用加锁。
#ifdef _MSC_VER
unsigned __stdcall bvcsp_thread(void*)
#else
void* bvcsp_thread(void*)
#endif
{
    do {
        BVCSP_HandleEvent();
        if (g_fileManager)
            g_fileManager->HandleEvent();
    } while (g_bRun);
#ifdef _MSC_VER
    return 0;
#endif
}


int InitBVLib(const AuthInitParam authParam, bvSessionParam sessionParam,
    const bvChannelParam* channelParam, const bvGPSParam* gpsParam,
    const bvFileParam* fileParam)
{
    BVCSP_SetLogCallback(Log_Callback, BVCU_LOG_LEVEL_INFO);
    BVCSP_Initialize(0, 0);
    if (g_mediaChannel != 0)
    {
        printf("has inited!\n");
        return -1;
    }
#ifdef _MSC_VER
    _beginthreadex(NULL, 0, bvcsp_thread, NULL, 0, NULL);
#else
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, bvcsp_thread, NULL);
#endif
    // 初始化文件读写等操作实现，根据文件系统实现，通用文件系统使用默认实现(CFILE)
    SetFileFunc(NULL); // 使用默认实现
    // 初始化认证参数
    BVCSP_HandleEvent();
    if (SetAuthParam(authParam) < 0)
    {
        printf("SetAuthParam failed!\n");
        return -1;
    }

    // session 参数
    g_sessionParam = sessionParam;
    g_session = new CPUSession(authParam.termInfo.ID);
    if (g_session == NULL)
    {
        printf("new session failed.");
        return -1;
    }
    g_session->SetServer(sessionParam.IP, sessionParam.port, sessionParam.port, sessionParam.proto, sessionParam.timeout, sessionParam.iKeepaliveInterval);
    // 初始化音视频参数
    if (channelParam)
    {
        if (channelParam->OnOpen == 0 || channelParam->OnClose == 0 || channelParam->OnPLI == 0 ||
            channelParam->OnRecvAudio == 0 || channelParam->OnPTZCtrl == 0)
        {
            printf("channel param error!\n");
            return -1;
        }
        bool bVideoIn = false;
        bool bAudioIn = false;
        if (_stricmp(channelParam->szVideoCodec, "H264") == 0 ||
            _stricmp(channelParam->szVideoCodec, "H265") == 0)
        {
            bVideoIn = true;
        }
        if (_stricmp(channelParam->szAudioCodec, "PCMA") == 0 ||
            _stricmp(channelParam->szAudioCodec, "G726") == 0 ||
            _stricmp(channelParam->szAudioCodec, "AAC") == 0)
        {
            bAudioIn = true;
        }
        g_channelParam = *channelParam;
        g_mediaChannel = new CMediaChannel(bVideoIn, bAudioIn, channelParam->bAudioRecv, channelParam->bPtzCtrl);
        if (g_mediaChannel == NULL)
        {
            printf("new channel failed.");
            return -1;
        }
        g_session->AddAVChannel(g_mediaChannel);
        g_session->SetPTZCount(1);// 假设支持云台操作
    }
    // GPS
    if (gpsParam)
        g_session->AddGPSInterface(gpsParam);
    // file
    if (fileParam)
    {
        g_fileParam = *fileParam;
        g_fileManager = new CMyFileTrans();
        g_session->SetFileManager(g_fileManager);
    }
    else
        memset(&g_fileParam, 0, sizeof(g_fileParam));
}

// 退出程序时，通过FreeBVLib()退出登录，并释放资源。
void FreeBVLib()
{
    Logout();
    Sleep(500); // 等待下线命令发送出去
    g_bRun = false;
    BVCSP_Finish();
}


int Login(int through, int lat, int lng)
{
    if (g_session)
        return g_session->Login(through, lat, lng);
    return -1;
}
int Logout()
{ // 下线
    if (g_session)
        return g_session->Logout();
    return -1;
}
bool BLogining()
{ // 是否正在上线。
    if (g_session)
        return g_session->BLogining();
    return false;
}
bool BOnline()
{ // 是否已经上线。
    if (g_session)
        return g_session->BOnline();
    return false;
}
bool BLogouting()
{ // 是否正在下线.
    if (g_session)
        return g_session->BLogouting();
    return false;
}
bool BOffline()
{ // 是否离线状态
    if (g_session)
        return g_session->BOffline();
    return true;
}

// 发送音视频数据
int SendAudioData(long long iPTS, const void* pkt, int len)
{
    if (g_mediaChannel == 0)
    {
        printf("g_mediaChannel is null!\n");
        return -1;
    }
    if (g_mediaChannel->BOpen() == false)
    {
        printf("media channel is not open!\n");
        return -1;
    }
    return g_mediaChannel->WriteAudio(iPTS, (char*)pkt, len);
}
// 发送音视频数据
int SendVideoData(long long iPTS, const void* pkt, int len)
{
    if (g_mediaChannel == 0)
    {
        printf("g_mediaChannel is null!\n");
        return -1;
    }
    if (g_mediaChannel->BOpen() == false)
    {
        printf("media channel is not open!\n");
        return -1;
    }
    return g_mediaChannel->WriteVideo(iPTS, (char*)pkt, len);
}

// 获取推荐的码率
int GetGuessBandwidth()
{
    return g_mediaChannel->GetGuessBandwidth();
}

// ================================ GPS 定位接口 ==============================
// 发送GPS数据
int SendGPSData(const BVCU_PUCFG_GPSData* gpsData)
{
    if (g_session == 0)
    {
        printf("initBVLib before this\n");
        return -1;
    }
    return g_session->SendGPSData(gpsData);
}

// 给服务器发GPS历史位置数据.用于补录离线过程中产生的位置数据。gpsData是数组。
int SendGPSHistory(const BVCU_PUCFG_GPSData* gpsData, int count) {
    if (g_session == 0)
    {
        printf("initBVLib before this\n");
        return -1;
    }
    return g_session->SendGPSHistory(gpsData, count);
}
// =============================== 文件传输接口 =============================
// 上传文件
int UploadFile(BVCU_File_HTransfer* phTransfer, const char* localFilePathName, const bvFileInfo* fileInfo)
{
    if (g_session == 0)
    {
        printf("initBVLib before this\n");
        return -1;
    }
    return g_session->UploadFile(phTransfer, localFilePathName, fileInfo);
}