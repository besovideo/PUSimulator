#pragma once

#include <time.h>
#include "base/dialog.h"

// 串口 通道
class CTSPChannel : public CTSPChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnSetName(const char* name); // 收到配置通道名称请求
    virtual BVCU_Result OnOpenRequest();   // 收到打开请求，回复是否同意，0：同意
    virtual void OnOpen();   // 建立通道连接成功通知
    virtual void OnClose();  // 通道连接关闭通知
    virtual void OnRecvData(const void* pkt, int len);   // 收到平台发给串口的数据。
    virtual const BVCU_PUCFG_SerialPort* OnGetTSPParam(); // 收到查询配置
    virtual BVCU_Result OnSetTSPParam(const BVCU_PUCFG_SerialPort* pParam); // 收到修改配置

protected:
    int m_interval;   // 上报数据时间间隔，秒。// 用于模拟收到串口数据
    time_t m_lasttime;   // 上次上报时间时间。秒。time();

public:
    CTSPChannel();
    virtual ~CTSPChannel() {}
    void SendData();  // 模拟收到串口数据，发送给平台。
};
