#pragma once
#include "BVCSP.h"

class CChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnSetName(const char* name) = 0; // 收到配置通道名称请求
    virtual BVCU_Result OnOpenRequest() = 0;   // 收到打开请求，回复是否同意，0：同意
    virtual void OnOpen() = 0;   // 建立通道连接成功通知
    virtual void OnClose() = 0;  // 通道连接关闭通知
    virtual void OnPLI() = 0;    // 收到生成关键帧请求

protected:
    int m_index; // 硬件索引，从0开始编号，可以由session注册时分配（这时每个通道注册顺序不能变化）。
    int m_channelIndexBase;  // 通道号起始值。
    int m_supportMediaDir;   // 支持的媒体方向。
    char m_name[64];         // 通道名称。

    BVCSP_HDialog m_hDialog; // BVCSP传输句柄。
    int m_openMediaDir;      // 当前打开媒体方向。
    bool m_bOpening;         // 是否正在打开中，等待回复。
public:
    CChannelBase(int IndexBase);
    virtual ~CChannelBase();
    void SetIndex(int index) { m_index = index; }  // 设置子硬件号
    void SetName(const char* name);
    bool BOpen() { return m_hDialog != 0; }

    int GetIndex() { return m_index; } // 获取子硬件号，不同硬件从0开始编号。
    int GetChannelIndex() { return m_channelIndexBase + m_index; } // 获取通道号。BVCU_SUBDEV_INDEXMAJOR_*
    const char* GetName() { return m_name; }
    int GetSupportMediaDir() { return m_supportMediaDir; }

    // 不要调用，底层交互接口
    BVCSP_HDialog GetHDialog() { return m_hDialog; }
    int  GetOpenDir() { return m_openMediaDir; }
    void SetHialog(BVCSP_HDialog hDialog, int mediaDir) { m_hDialog = hDialog; m_openMediaDir = mediaDir; }
    void SetBOpening(bool bOpening) { m_bOpening = bOpening; }
    BVCU_Result OnRecvPacket(const BVCSP_Packet* packet);
    typedef void (*bvcsp_OnDialogEvent)(BVCSP_HDialog hDialog, int iEventCode, BVCSP_Event_DialogCmd* pParam);
    static bvcsp_OnDialogEvent g_bvcsp_onevent;
};

// 音视频通道
class CAVChannelBase : public CChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual void OnRecvAudio(long long iPTS, const void* pkt, int len) = 0;   // 收到平台发来的音频数据。编码信息同ReplySDP()。
    virtual BVCU_Result OnPTZCtrl(const BVCU_PUCFG_PTZControl* ptzCtrl) = 0;   // 收到平台发来的云台控制命令。
    //virtual const BVCU_PUCFG_EncoderChannel* OnGetEncoder() = 0;  // 收到平台查询编码配置请求
    //virtual BVCU_Result OnSetEncoder(const BVCU_PUCFG_EncoderChannel* param) = 0;  // 收到平台设置编码配置请求
public:
    CAVChannelBase(bool bVideoIn, bool bAudioIn, bool bAudioOut, bool ptz);// 是否支持：视频采集，音频采集，音频播放，云台
    virtual ~CAVChannelBase() {}
    // 回复 打开请求, 不支持的可以为空。收到打开请求后，需要调用ReplySDP回复请求。
    BVCU_Result ReplySDP(BVCU_Result result, const BVCSP_VideoCodec* video, const BVCSP_AudioCodec* audio); // 是否成功、视频SDP、音频SDP
    BVCU_Result WriteVideo(long long iPTS, const char* pkt, int len);
    BVCU_Result WriteAudio(long long iPTS, const char* pkt, int len);
    // 查询
    bool BSupportVideoIn() { return (m_supportMediaDir & BVCU_MEDIADIR_VIDEOSEND) != 0; }
    bool BSupportAudioIn() { return (m_supportMediaDir & BVCU_MEDIADIR_AUDIOSEND) != 0; }
    bool BSupportAudioOut() { return (m_supportMediaDir & BVCU_MEDIADIR_AUDIORECV) != 0; }
    bool BSupportPTZ() { return m_bptz; }
    bool BNeedVideoIn() { return (m_openMediaDir & BVCU_MEDIADIR_VIDEOSEND) != 0; }
    bool BNeedAudioIn() { return (m_openMediaDir & BVCU_MEDIADIR_AUDIOSEND) != 0; }
    bool BNeedAudioOut() { return (m_openMediaDir & BVCU_MEDIADIR_AUDIORECV) != 0; }

protected:
    bool m_bptz; // 是否支持云台。
};

// GPS 通道
class CGPSChannelBase : public CChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual const BVCU_PUCFG_GPSData* OnGetGPSData() = 0;   // 收到查询定位
    virtual const BVCU_PUCFG_GPSParam* OnGetGPSParam() = 0; // 收到查询配置
    virtual BVCU_Result OnSetGPSParam(const BVCU_PUCFG_GPSParam* pParam) = 0; // 收到修改配置
public:
    CGPSChannelBase();
    virtual ~CGPSChannelBase() {}
    // 发送 GPS数据
    BVCU_Result WriteData(const BVCU_PUCFG_GPSData* pGPSData);
};

// 串口 通道
class CTSPChannelBase : public CChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual void OnRecvData(const void* pkt, int len) = 0;   // 收到平台发给串口的数据。
    virtual const BVCU_PUCFG_SerialPort* OnGetTSPParam() = 0; // 收到查询配置
    virtual BVCU_Result OnSetTSPParam(const BVCU_PUCFG_SerialPort* pParam) = 0; // 收到修改配置
    virtual void OnPLI() {}
public:
    CTSPChannelBase();
    virtual ~CTSPChannelBase() {}
    // 发送 串口数据 给平台
    BVCU_Result WriteData(const char* pkt, int len);
};
