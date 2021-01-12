#pragma once

#include <time.h>
#include "base/dialog.h"

// GPS 通道
class CGPSChannel : public CGPSChannelBase
{
public:
    // ======= 需要实现的功能接口
    virtual BVCU_Result OnSetName(const char* name);
    virtual BVCU_Result OnOpenRequest();
    virtual void OnOpen();
    virtual void OnClose();
    virtual const BVCU_PUCFG_GPSData* OnGetGPSData();   // 收到查询定位
    virtual const BVCU_PUCFG_GPSParam* OnGetGPSParam(); // 收到查询配置
    virtual BVCU_Result OnSetGPSParam(const BVCU_PUCFG_GPSParam* pParam); // 收到修改配置

protected:
    int m_interval;   // 上报数据时间间隔，秒。
    time_t m_lasttime;   // 上次上报时间时间。秒。time();
    BVCU_PUCFG_GPSData m_position; // 当前位置。模拟的，您可以从GPS设备中获取。
    int m_lat;  // 中心位置，模拟位置以中心位置为圆点，画圆运动。
    int m_lng; 
    int m_chagedu;  // 半径

public:
    CGPSChannel();
    virtual ~CGPSChannel() {}
    void UpdateData();  // 检查上报时间是否到了，读取GPS数据，上报。
    bool ReadGPSData(); // 从设备中读取定位数据，需要实现。
};
