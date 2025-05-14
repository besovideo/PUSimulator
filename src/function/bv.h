#pragma once

#include <stdint.h>
#include <iostream>
#include <string>
#include "BVCSP.h"
#include "BVCUConst.h"
#include "../base/bv_auth.h"
#include "../base/dialog.h"
#include "../base/filetransfer.h"
#include "../base/session.h"

// 登录实例相关 参数
struct bvSessionParam
{
    const char* IP;                                                                    // 服务器IP地址
    int port;                                                                          // 服务器端口
    int proto;                                                                         // 协议: 0:UDP, 1: TCP
    int timeout;                                                                       // 命令超时时间，毫秒
    int iKeepaliveInterval;                                                            // 保活命令间隔，毫秒
    void (*OnLoginEvent)(BVCU_Result iResult) {};                                       // 登录结果通知
    void (*OnOfflineEvent)(BVCU_Result iResult) {};                                     // 登出结果通知
    void (*OnCommandReply)(BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam) {}; // 给服务器发命令收到回复
};

//  实时音视频相关 参数
struct bvChannelParam
{
    char szVideoCodec[32]; // 视频编码格式, 如"H265"、"H264", 空：表示不支持视频。
    char szAudioCodec[32]; // 音频编码格式, 如"PCMA"、"G726"、"AAC", 空：表示不支持音频。
    int iSampleRate;       // 音频采样率, 8000、16000、32000、44100、48000。
    int iBitrate;          // 音频比特率, 32、64。G726编码时必须填.
    int iChannelCount;     // 音频通道数
    bool bAudioRecv;       // 是否支持音频接收。true：支持，false：不支持。
    bool bPtzCtrl;         // 是否支持云台控制。true：支持，false：不支持。

    void (*OnOpen)(bool video, bool audio, bool audioOut);          // 建立通道连接成功通知, 上层开始发送音视频数据。
    void (*OnClose)();                                              // 通道连接关闭通知, 上层停止发送音视频数据。
    void (*OnPLI)();                                                // 收到生成关键帧请求, 上层通知编码器生成关键帧。
    void (*OnRecvAudio)(long long iPTS, const void* pkt, int len);  // 收到平台发来的音频数据。编码信息同szAudioCodec。
    BVCU_Result(*OnPTZCtrl)(const BVCU_PUCFG_PTZControl* ptzCtrl); // 收到平台发来的云台控制命令。
    BVCU_PUCFG_PTZAttr* (*OnGetPTZParam)();                         // 收到平台发来的云台查询命令。返回云台参数。
};

// 文件传输相关参数
struct bvFileParam
{
    int iBandwidthLimit; // 带宽限制。单位kbps，0表示无限制，建议限制文件传输的带宽。

    BVCU_Search_Response* (*OnGetRecordFiles)(BVCU_Search_Request* req);                                      // 获取录像文件列表
    bvFileInfo* (*OnFileRequest)(BVCU_File_HTransfer hTransfer, char* pLocalFilePathName);                    // 收到下载文件请求,返回NULL: 拒绝请求.
    void (*OnFileEvent)(BVCU_File_HTransfer hTransfer, void* pUserData, int iEventCode, BVCU_Result iResult); // 文件传输事件
    BVCU_PUCFG_RecordStatus* (*OnGetRecordStatus)();                                                          // 获取录像状态
    BVCU_Result(*OnManualRecord)(bool bStart);                                                               // 开始/停止 手动录像.
};

// ================================= 初始化库 ==================================

// 初始化库，设置设备参数.
/*
 * authParam: 认证参数
 * channelParam: 音视频通道参数, NULL: 不支持音视频。
 * gpsParam: GPS定位接口参数, NULL: 不支持GPS。
 * fileParam: 文件相关接口, NULL: 不支持文件接口.
 * return: 0: 成功，-1: 失败。
 */
int InitBVLib(const AuthInitParam authParam, bvSessionParam sessionParam,
    const bvChannelParam* channelParam, const bvGPSParam* gpsParam,
    const bvFileParam* fileParam);

// 退出程序时，通过FreeBVLib()退出登录，并释放资源。
void FreeBVLib();

// 认证是否成功
bool IsAuthSuccess();
// 获取认证号
int  GetAuthCode();

// ================================= 登录session接口 ============================

// 上线服务器,
/*
 * through: 上线网络类型BVCU_PU_ONLINE_THROUGH_
 * lat,lng 当前设备WGS84坐标位置
 */
int Login(int through, int lat, int lng);
int Logout();      // 下线
bool BLogining();  // 是否正在上线。
bool BOnline();    // 是否已经上线。
bool BLogouting(); // 是否正在下线.
bool BOffline();   // 是否离线状态

// ================================= 实时音视频接口 ==============================
// 回复打开成功

// 发送音视频数据
int SendAudioData(long long iPTS, const void* pkt, int len);
// 发送音视频数据
int SendVideoData(long long iPTS, const void* pkt, int len);
// 获取推荐的码率
int GetGuessBandwidth();

// ================================ GPS 定位接口 ==============================
// 发送GPS数据
int SendGPSData(const BVCU_PUCFG_GPSData* gpsData);
// 给服务器发GPS历史位置数据.用于补录离线过程中产生的位置数据。gpsData是数组。
int SendGPSHistory(const BVCU_PUCFG_GPSData* gpsData, int count);

// =============================== 文件传输接口 =============================
// 上传音视频/图片 文件
int UploadFile(BVCU_File_HTransfer* phTransfer, const char* localFilePathName, const bvFileInfo* fileInfo);