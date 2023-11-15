#pragma once

#include <time.h>
#include "base/dialog.h"

// GPS 通道
class CGPSChannel : public CGPSChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnSetName(const char* name); // 收到配置通道名称请求
    virtual BVCU_Result OnOpenRequest();   // 收到打开请求，回复是否同意，0：同意
    virtual void OnOpen();   // 建立通道连接成功通知
    virtual void OnClose();  // 通道连接关闭通知
    virtual void OnPLI();    // 收到生成关键帧请求
    virtual const BVCU_PUCFG_GPSData* OnGetGPSData();   // 收到查询定位
    virtual const BVCU_PUCFG_GPSParam* OnGetGPSParam(); // 收到查询配置
    virtual BVCU_Result OnSetGPSParam(const BVCU_PUCFG_GPSParam* pParam); // 收到修改配置

protected:
    BVCU_PUCFG_GPSParam m_param;
    time_t m_lasttime;   // 上次上报时间时间。秒。time();
    BVCU_PUCFG_GPSData m_position; // 当前位置。模拟的，您可以从GPS设备中获取。
    int m_lat;  // 中心位置，模拟位置以中心位置为圆点，画圆运动。
    int m_lng;
    int m_chagedu;  // 半径

public:
    CGPSChannel();
    virtual ~CGPSChannel() {}
    const BVCU_PUCFG_GPSData* UpdateData();  // 检查上报时间是否到了，读取GPS数据，上报。
    bool ReadGPSData(); // 从设备中读取定位数据，需要实现。
};
