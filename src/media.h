#pragma once

#include <time.h>
#include "base/dialog.h"

#define VIDEO_FILE_PATH_NAME "./h264_320x256.264"
#define AUDIO_FILE_PATH_NAME "./8000Hz_1Ch_16bit_32kbps.g726"
// 音视频 通道
class CMediaChannel : public CAVChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnSetName(const char* name); // 收到配置通道名称请求
    virtual BVCU_Result OnOpenRequest();   // 收到打开请求，回复是否同意，0：同意
    virtual void OnOpen();   // 建立通道连接成功通知
    virtual void OnClose();  // 通道连接关闭通知
    virtual void OnRecvAudio(long long iPTS, const void* pkt, int len);   // 收到平台发来的音频数据。编码信息同ReplySDP()。
    virtual BVCU_Result OnPTZCtrl(const BVCU_PUCFG_PTZControl* ptzCtrl);   // 收到平台发来的云台控制命令。

protected:
    int m_interval;   // 上报数据时间间隔，毫秒。// 用于模拟收到音视频输入数据
    int m_lasttime;   // 上次上报时间。毫秒。GetTickCount();
    int m_lastAdjtime;   // 上次调整码率时间。毫秒。GetTickCount();
    long long m_pts;  // 上次时间戳。
    FILE* m_audioFile;// 音频输入文件
    FILE* m_videoFile;// 音频输入文件

    char* ReadVideo(char* buf, int* len); // 从文件中读取h264数据，模拟收到编码器数据。
    char* ReadAudio(char* buf, int* len); // 从文件中读取g726数据，模拟收到编码器数据。
    void Reply();     // 回复请求，用写死的SDP信息，所以不轻易修改自带的音视频文件，除非您知道要修改什么。
public:
    CMediaChannel();
    virtual ~CMediaChannel() {}
    void SendData();  // 模拟收到音视频编码后数据，发送给平台。
};
