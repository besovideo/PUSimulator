#pragma once
#include "BVCSP.h"

class CChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnSetName(const char* name) = 0; // 收到配置通道名称请求
    virtual void OnOpen() = 0;   // 建立通道连接成功通知
    virtual void OnClose() = 0;  // 通道连接关闭通知

protected:
    int m_index; // 硬件索引，从0开始编号，可以由session注册时分配（这时每个通道注册顺序不能变化）。
    int m_channelIndexBase;  // 通道号起始值。
    int m_supportMediaDir;   // 支持的媒体方向。
    char m_name[64];         // 通道名称。

    BVCSP_HDialog m_hDialog; // BVCSP传输句柄。
    int m_iOpenMediaDir;     // 当前打开媒体方向
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
    void SetHialog(BVCSP_HDialog hDialog, int mediaDir) { m_hDialog = hDialog; m_iOpenMediaDir = mediaDir; }
};

// 音视频通道
class CAVChannelBase : public CChannelBase
{
public:
    CAVChannelBase();
    virtual ~CAVChannelBase() {}
};

// GPS 通道
class CGPSChannelBase : public CChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnOpenRequest() = 0;   // 收到打开请求，回复是否同意，0：同意
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
    CTSPChannelBase() : CChannelBase(BVCU_SUBDEV_INDEXMAJOR_MIN_TSP) {};
    virtual ~CTSPChannelBase() {}
};
