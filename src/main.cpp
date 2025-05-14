

#include <stdio.h>
#include <iostream>
#include <string>

#include "./utils/utils.h"
#include "base/bv_auth.h"
#include "function/bv.h"

#include "./simulator/config.h"
#include "./simulator/media.h"
#include "./simulator/gps.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

#ifdef _WIN64
#pragma comment(lib, "BVCSP_x64.lib")
#elif _WIN32
#pragma comment(lib, "BVCSP.lib")
#endif // _WIN32

// 请修改为您申请的开发者密码，下面的密钥是测试开发者密钥（会不定时被注销）。
// 请修改为您申请的开发者密码，下面的密钥是测试开发者密钥（会不定时被注销）。
// 请修改为您申请的开发者密码，下面的密钥是测试开发者密钥（会不定时被注销）。
const char* APP_ID = "app_abc814f85c75374b";
const char* APP_N = "f2248a26e34cdfe8297704affc7f127b";
const char* APP_E = "9572f125ccd791f1";
// 保存认证离线文件的路径+文件名
const char* Auth_file = "bv_auth.dat";
// 设备ID号，这里写死测试用，实际使用时需要根据设备的硬件信息(如MAC地址等)生成唯一的ID号。不超过32个字符。格式("PU_%08X",int32_t)。
const char* PU_ID = "PU_63AA0000";

// ======================================= 认证相关实现 =======================================

// 认证事件回调
void my_OnAuthEvent(int32_t authCode, BVRAuthResult result, struct BVRAuthParam* info)
{
    if (result == AUTH_Result_OK)
        printf("======================= auth OK ==========================\n");
    else
        printf("======================= auth code: %d result: %d\n", authCode, result);
}

// 认证 开发者信息 + 硬件信息
AuthInitParam my_auth_inf()
{
    // 获取MAC地址(仅支持windows)
    MacAddressInfo macInfo;
    u_getMacAddress(macInfo);
    printf("[auth] mac addaress: %s\n", macInfo.MacAddr);
    const char* macAddress = macInfo.MacAddr;
    printf("[auth] puid : %s\n", PU_ID);

    AuthInitParam param;
    memset(&param, 0, sizeof(param));
    strncpy_s(param.authFilePath, sizeof(param.authFilePath), Auth_file, _TRUNCATE); // 离线认证文件路径
    strncpy_s(param.appId, sizeof(param.appId), APP_ID, _TRUNCATE);           // 开发者ID
    strncpy_s(param.appRSA_N, sizeof(param.appRSA_N), APP_N, _TRUNCATE);         // 开发者公钥N
    strncpy_s(param.appRSA_E, sizeof(param.appRSA_E), APP_E, _TRUNCATE);         // 开发者公钥E
    param.onEvent = my_OnAuthEvent; // 认证事件回调

    // 以下内容需要如实填写, 没有数据的可以留空，填写的有效数据越多越不会和其它设备冲突（冲突会被踢下线），但要保持不变（变了需要重新认证）########################
    // 以下内容需要如实填写, 没有数据的可以留空，填写的有效数据越多越不会和其它设备冲突（冲突会被踢下线），但要保持不变（变了需要重新认证）########################
    // 以下内容需要如实填写, 没有数据的可以留空，填写的有效数据越多越不会和其它设备冲突（冲突会被踢下线），但要保持不变（变了需要重新认证）########################
    _snprintf(param.termInfo.ID, sizeof(param.termInfo.ID), PU_ID);
    _snprintf(param.termInfo.MAC, sizeof(param.termInfo.MAC) - 1, macAddress);
    _snprintf(param.termInfo.ModelNumber, sizeof(param.termInfo.ModelNumber) - 1, "");
    _snprintf(param.termInfo.IMEI, sizeof(param.termInfo.IMEI) - 1, ""); // 尽量填写真实，防止和别人冲突
    _snprintf(param.termInfo.HardwareProvider, sizeof(param.termInfo.HardwareProvider) - 1, "BesoVideo");
    _snprintf(param.termInfo.HardwareSN, sizeof(param.termInfo.HardwareSN) - 1, "");
    _snprintf(param.termInfo.HardwareVersion, sizeof(param.termInfo.HardwareVersion) - 1, "");
    _snprintf(param.termInfo.SoftwareProvider, sizeof(param.termInfo.SoftwareProvider) - 1, "");
    _snprintf(param.termInfo.SoftwareVersion, sizeof(param.termInfo.SoftwareVersion) - 1, "");
    _snprintf(param.termInfo.OSType, sizeof(param.termInfo.OSType) - 1, ""); // 尽量填写
    _snprintf(param.termInfo.OSVersion, sizeof(param.termInfo.OSVersion) - 1, ""); // 系统版本号
    _snprintf(param.termInfo.OSID, sizeof(param.termInfo.OSID) - 1, ""); // 获取系统ID
    _snprintf(param.termInfo.CPU, sizeof(param.termInfo.CPU) - 1, ""); // 尽量填写
    _snprintf(param.termInfo.Desc, sizeof(param.termInfo.Desc) - 1, "");
    _snprintf(param.termInfo.UserLabel, sizeof(param.termInfo.UserLabel) - 1, ""); // 用户标签, 指定设备对用户可见

    return param;
}

// ======================================= 登录相关实现 =======================================

// 登录事件回调
void OnLoginEvent(BVCU_Result iResult)
{
    if (iResult == BVCU_RESULT_S_OK)
    {
        printf("login ok, online\n");
    }
    else
    {
        printf("login faild: %d\n", iResult);
    }
}
// 断线事件回调
void OnOfflineEvent(BVCU_Result iResult)
{
    if (iResult == BVCU_RESULT_S_OK)
    {
        printf("logout ok\n");
    }
    else
    {
        printf("offline, service disconnect: %d\n", iResult);
    }
}
// 收到命令回复事件回调
void OnCommandReply(BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam)
{
    printf("recv command, ID: %d, resoult: %d\n", pCommand->iSubMethod, pParam->iResult);
}

// ======================================= 实时音视频相关实现 =======================================

// 建立通道连接成功通知, 上层开始发送音视频数据(可能会收到多次，根据最后一次最新的请求打开需求的音视频。
void OnOpen(bool video, bool audio, bool audioOut)
{
    OpenMedia(video, audio, audioOut); // 模拟打开采集和编码器
    printf("================  media opened. video: %d audio: %d audioOut: %d\n", video, audio, audioOut);
}
// 通道连接关闭通知, 上层停止发送音视频数据。
void OnClose()
{
    CloseMedia(); // 模拟关闭采集和编码器
    printf("================  media closed \n");
}
// 收到生成关键帧请求, 上层通知编码器生成关键帧。
void OnPLI()
{
    ReqPLI(); // 模拟通知编码器生成关键帧
    printf("================  media pli \n");
}
// 收到平台发来的音频数据。编码信息同szAudioCodec。
void OnRecvAudio(long long iPTS, const void* pkt, int len)
{
    printf("recv talk audio data: pts:%lld len: %d", iPTS, len);
}
BVCU_Result OnPTZCtrl(const BVCU_PUCFG_PTZControl* ptzCtrl)
{ // 收到平台发来的云台控制命令。
    printf("recv ptz control command: %d, %d, %d\n", ptzCtrl->iPTZCommand, ptzCtrl->iParam1, ptzCtrl->iParam2);
    return BVCU_RESULT_S_OK;
}
BVCU_PUCFG_PTZAttr* OnGetPTZParam()
{ // 收到平台发来的云台查询命令。返回云台参数。
    printf("recv ptz get param\n");
    return NULL; // 这里应该根据真实的云台参数返回。
}

// ======================================= GPS定位相关实现 =======================================
static int GPSInterval = 5;            // 定位上报间隔，单位秒, 应该从设备本地配置中来，对实时位置不敏感的设备建议30以上。
static int bStartGPS = 0;              // 是否开始上报定位数据, 服务器端通过命令订阅开启。
static BVCU_PUCFG_GPSParam g_gpsParam; // GPS参数, 应该从设备本地配置中来
// 开始订阅后，上层需要定时调用SendGPSData接口上报定位.
BVCU_Result OnSubscribeGPS(int bStart, int iInterval)
{
    printf("recv GPS subscribe, bStart: %d, iInterval: %d\n", bStart, iInterval);
    if (bStart == 0)
    {
        bStartGPS = 0;
        return BVCU_RESULT_S_OK;
    }
    if (iInterval < 5 || iInterval > 60 * 60)
        iInterval = 5;
    GPSInterval = iInterval;
    bStartGPS = 1;
    return BVCU_RESULT_S_OK;
}
// 获取当前位置, NULL: 没有位置.
BVCU_PUCFG_GPSData* OnGetGPSData() { return GetGPSData(); }
// 获取当前配置
BVCU_PUCFG_GPSParam* OnGetGPSParam()
{
    printf("recv GPS get param\n");
    g_gpsParam.iReportInterval = GPSInterval;
    return &g_gpsParam;
}
// 修改GPS配置
BVCU_Result OnSetGPSParam(const BVCU_PUCFG_GPSParam* pParam)
{
    g_gpsParam = *pParam;
    GPSInterval = pParam->iReportInterval;
    if (0 >= GPSInterval || GPSInterval <= 10 * 60)
        GPSInterval = 5;
    printf("recv GPS set param, report interval: %d\n", GPSInterval);
    // 这里应该将GPSInterval保存到文件中。
    return BVCU_RESULT_S_OK;
}

// ======================================= 文件传输相关实现 =======================================

// 获取录像文件列表
static BVCU_Search_FileInfo g_fileList[10];    // 录像文件列表,写死的,真实实现中需要开发者自己管理本地录像文件列表。
static BVCU_Search_Response g_fileResponse;    // 录像文件列表响应
static BVCU_PUCFG_RecordStatus g_recordStatus; // 录像状态，真实开发中需要开发者自己管理当前录像状态。
const  char testFile[] = "uploadfile.mp4"; // 测试文件
BVCU_Search_Response* OnGetRecordFiles(BVCU_Search_Request* req)
{
    printf("recv get record file list, index:%d pagesize:%d \n", req->stSearchInfo.iPostition, req->stSearchInfo.iCount);
    printf("time begin-end : %lld - %lld\n", req->stFilter.stFileFilter.iTimeBegin, req->stFilter.stFileFilter.iTimeEnd);
    printf("file type: %d\n", req->stFilter.stFileFilter.iFileType);
    memset(&g_fileResponse, 0, sizeof(g_fileResponse));
    g_fileResponse.iCount = 1;
    g_fileResponse.pData.pFileInfo = g_fileList;
    g_fileResponse.stSearchInfo = req->stSearchInfo;
    g_fileResponse.stSearchInfo.iCount = 1;
    memset(g_fileList, 0, sizeof(g_fileList));
    g_fileList[0].iRecordType = BVCU_STORAGE_RECORDTYPE_ONTIME; // 录像类型
    g_fileList[0].iFileType = BVCU_STORAGE_FILE_TYPE_RECORD;    // 文件类型
    g_fileList[0].iFileSize = 1024 * 1024 * 10;                 // 文件大小，单位字节
    g_fileList[0].iTimeBegin = time(NULL) - 60 * 60;            // 文件开始时间，单位秒
    g_fileList[0].iTimeEnd = time(NULL);                        // 文件结束时间，单位秒
    g_fileList[0].iTimeRecord = time(NULL);                     // 文件索引入库时间，单位秒
    // 文件路径+文件名, 这个很重要, base中的默认实现会根据这个参数读取文件。
    strncpy_s(g_fileList[0].szFilePath, sizeof(g_fileList[0].szFilePath), testFile, _TRUNCATE);

    return &g_fileResponse; // 这里返回录像文件列表。
}
// 收到下载文件请求,返回NULL: 拒绝请求.
bvFileInfo* OnFileRequest(BVCU_File_HTransfer hTransfer, char* pLocalFilePathName)
{
    printf("recv file download, file: %s\n", pLocalFilePathName);
    // 这里可以拒绝下载请求，返回NULL。
    // pLocalFilePathName是OnGetRecordFiles()接口返回文件列表中的szFilePath字段。
    //
    // 假设根据pLocalFilePathName可以找到文件，返回文件信息。
    static bvFileInfo fileInfo;
    static char* bvFileType = "video";
    memset(&fileInfo, 0, sizeof(fileInfo));
    // fileInfo.user;        // 用户账户, 产生录像时登录账户，为空时后台使用上传录像者账户.
    fileInfo.starttime = time(NULL) - 60 * 5; // 开始时间(UTC)， （必须）
    fileInfo.endtime = time(NULL);            // 录像结束时间(UTC)
    // fileInfo.channel;                         // 通道号,默认0
    fileInfo.filetype = bvFileType; // "video"、"audio"、"image"
    fileInfo.lat = 30.0 * 10000000;                                              // 经度
    fileInfo.lng = 120.0 * 1000000;                                              // 维度
    fileInfo.desc1;                                                              // 文件描述1
    fileInfo.desc2;                                                              // 文件描述2
    fileInfo.mark = false;                                                       // 是否是重点标记文件
    return &fileInfo;                                                            // 返回文件信息。
}
// 文件传输事件
void OnFileEvent(BVCU_File_HTransfer hTransfer, void* pUserData, int iEventCode, BVCU_Result iResult)
{
    printf("file event, event: %d, result: %d\n", iEventCode, iResult);
}
// 获取录像状态
BVCU_PUCFG_RecordStatus* OnGetRecordStatus()
{
    printf("recv record get status\n");
    return &g_recordStatus;
}
// 开始/停止 手动录像.
BVCU_Result OnManualRecord(bool bStart)
{
    g_recordStatus.bRecordAudio = bStart;
    g_recordStatus.bRecordVideo = bStart;
    g_recordStatus.pFileNames = g_fileList[0].szFilePath;
    return BVCU_RESULT_S_OK;
}

// ======================================= 主函数 =======================================
static bool brun = true;

// 安全退出
#ifdef _MSC_VER
BOOL ConsoleCtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
    case CTRL_C_EVENT:          // Ctrl+C
    case CTRL_BREAK_EVENT:      // Ctrl+Break
    case CTRL_CLOSE_EVENT:      // 关闭控制台窗口
    case CTRL_LOGOFF_EVENT:     // 用户注销
    case CTRL_SHUTDOWN_EVENT:   // 系统关机
        printf("recv exit event: %d\n", dwCtrlType);
        brun = false;
        return TRUE;  // 处理该事件

    default:
        return FALSE; // 忽略其他事件
    }
}
#else
void signal_handler(int signal) {
    printf("recv exit event: %d\n", signal);
    brun = false;
}
#endif // 


int main()
{
    // 注册控制台事件处理函数
#ifdef _MSC_VER
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
        printf("SetConsoleCtrlHandler failed!");
        return 1;
    }
#else
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // 系统终止信号
    signal(SIGHUP, signal_handler);   // 终端断开
#endif
    // 模拟硬件初始化，生成ID等，这里的实现是从配置文件中读取
    PUConfig pConfig;
    LoadConfig(&pConfig);
    PU_ID = pConfig.ID;

    // 初始化库，参数，回调
    // 登录服务器相关参数
    bvSessionParam sessionParam;
    memset(&sessionParam, 0, sizeof(sessionParam));
    sessionParam.IP = pConfig.serverIP;
    sessionParam.port = pConfig.serverTCPPort;
    sessionParam.proto = pConfig.protoType;
    if (pConfig.protoType == 0)
        sessionParam.port = pConfig.serverUDPPort;
    sessionParam.timeout = 30 * 1000;                    // 命令超时时间，毫秒
    sessionParam.iKeepaliveInterval = 5 * 1000;          // 保活命令间隔，毫秒
    sessionParam.OnLoginEvent = OnLoginEvent;     // 登录结果通知
    sessionParam.OnOfflineEvent = OnOfflineEvent; // 登出结果通知
    sessionParam.OnCommandReply = OnCommandReply; // 给服务器发命令收到回复
    // 实时音视频相关参数
    bvChannelParam channelParam;
    memset(&channelParam, 0, sizeof(channelParam));
    // 填写音视频编码信息，不支持视频或音频时，请将对应的编码信息置空。
    GetMediaInfo(&channelParam);
    channelParam.bAudioRecv = true;             // 是否支持音频接收。true：支持，false：不支持。
    channelParam.bPtzCtrl = true;               // 是否支持云台控制。true：支持，false：不支持。
    channelParam.OnOpen = OnOpen;               // 建立通道连接成功通知, 上层开始发送音视频数据。
    channelParam.OnClose = OnClose;             // 通道连接关闭通知, 上层停止发送音视频数据。
    channelParam.OnPLI = OnPLI;                 // 收到生成关键帧请求, 上层通知编码器生成关键帧。
    channelParam.OnRecvAudio = OnRecvAudio;     // 收到平台发来的音频数据。编码信息同szAudioCodec。
    channelParam.OnPTZCtrl = OnPTZCtrl;         // 收到平台发来的云台控制命令。
    channelParam.OnGetPTZParam = OnGetPTZParam; // 收到平台发来的云台查询命令。返回云台参数。
    // GPS 定位相关参数
    OpenGPS(); // 打开GPS设备
    bvGPSParam gpsParam;
    memset(&gpsParam, 0, sizeof(gpsParam));
    gpsParam.OnSubscribeGPS = OnSubscribeGPS; // 收到订阅GPS命令
    gpsParam.OnGetGPSData = OnGetGPSData;     // 获取当前位置, NULL: 没有位置.
    gpsParam.OnGetGPSParam = OnGetGPSParam;   // 获取当前配置
    gpsParam.OnSetGPSParam = OnSetGPSParam;   // 修改GPS配置
    g_gpsParam.bEnable = 1;                   // 是否启用GPS定位
    g_gpsParam.iSupportSatelliteSignal = BVCU_PUCFG_SATELLITE_GPS | BVCU_PUCFG_SATELLITE_BDS;
    g_gpsParam.iSetupSatelliteSignal = BVCU_PUCFG_SATELLITE_GPS | BVCU_PUCFG_SATELLITE_BDS;
    g_gpsParam.iReportInterval = 5;                                            // 上报间隔，单位秒
    strncpy_s(g_gpsParam.szName, sizeof(g_gpsParam.szName), "GPS", _TRUNCATE); // GPS名称
    // 文件传输相关参数
    memset(g_fileList, 0, sizeof(g_fileList));
    bvFileParam fileParam;
    memset(&fileParam, 0, sizeof(fileParam));
    fileParam.iBandwidthLimit = pConfig.bandwidth;   // 带宽限制。单位kbps，0表示无限制，建议限制文件传输的带宽。
    fileParam.OnGetRecordFiles = OnGetRecordFiles;   // 获取录像文件列表
    fileParam.OnFileRequest = OnFileRequest;         // 收到下载文件请求
    fileParam.OnFileEvent = OnFileEvent;             // 文件传输事件
    fileParam.OnGetRecordStatus = OnGetRecordStatus; // 获取录像状态
    fileParam.OnManualRecord = OnManualRecord;       // 开始/停止 手动录像.

    // 初始化库，设置设备参数.
    // 如果不支持音视频：将&channelParam改成NULL。
    // 如果不支持GPS定位：将&gpsParam改成NULL。
    // 如果不支持文件传输：将&fileParam改成NULL。
    InitBVLib(my_auth_inf(), sessionParam, &channelParam, &gpsParam, &fileParam);

    int m_lastReloginTime = 0;
    int lastReportGPSTime = 0;
    int lastUploadFile = 0;
    // 主线程
    while (brun)
    {
        Sleep(100);
        // 等待认证通过
        if (!IsAuthSuccess()) {
            printf("device not auth.contact sales. auth code: %d\n", GetAuthCode());
            Sleep(10 * 1000);
            continue;
        }
        // 自动上线，掉线后重新上线
        if (!BOnline()) {
            if (!BLogining()) {
                int relogin = rand() % 60000 + 10000; // 至少10秒，随机1分钟，防止批量设备并发上线。
                int nowtime = GetTickCount();
                if (nowtime - m_lastReloginTime > relogin) {
                    Login(BVCU_PU_ONLINE_THROUGH_ETHERNET, 0, 0);
                    m_lastReloginTime = nowtime;
                }
            }
            continue;
        }
        // 自动上报GPS订阅
        if (bStartGPS) {
            int nowtime = GetTickCount();
            if (nowtime - lastReportGPSTime > GPSInterval * 1000) {
                const BVCU_PUCFG_GPSData* newGps = GetGPSData();
                SendGPSData(newGps);
                lastReportGPSTime = nowtime;
            }
        }
        // 测试主动上传音视频文件
        if (lastUploadFile == 0) {
            lastUploadFile = 1;
            BVCU_File_HTransfer hFile;
            bvFileInfo fileInfo;
            char* bvFileType = "video";
            memset(&fileInfo, 0, sizeof(fileInfo));
            // fileInfo.user;        // 用户账户, 产生录像时登录账户，为空时后台使用上传录像者账户.
            fileInfo.starttime = time(NULL) - 60 * 5; // 开始时间(UTC)， （必须）
            fileInfo.endtime = time(NULL);            // 录像结束时间(UTC)
            // fileInfo.channel;                         // 通道号,默认0
            fileInfo.filetype = bvFileType; // "video"、"audio"、"image"
            fileInfo.lat = 30.0 * 10000000;                                              // 经度
            fileInfo.lng = 120.0 * 1000000;                                              // 维度
            fileInfo.desc1;                                                              // 文件描述1
            fileInfo.desc2;                                                              // 文件描述2
            fileInfo.mark = false;                                                       // 是否是重点标记文件
            UploadFile(&hFile, testFile, &fileInfo);
        }
    }
    FreeBVLib();
    return 0;
}