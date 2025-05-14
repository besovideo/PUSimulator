#pragma once

#include <time.h>
#include "PUConfig.h"

// 模拟打开GPS定位功能
void OpenGPS(); // 开启GPS定位功能

// 模拟获取GPS数据
BVCU_PUCFG_GPSData* GetGPSData();