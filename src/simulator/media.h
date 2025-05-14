#pragma once

#include <time.h>
#include "../function/bv.h"

// 获取音视频通道参数
void GetMediaInfo(bvChannelParam* param);

// 开启采集和编码器,这里是打开音视频文件模拟音视频流。
void OpenMedia(bool video, bool audio, bool audioOut);

// 关闭采集和编码器,这里是关闭音视频文件。
void CloseMedia();

// 收到生成关键帧请求
void ReqPLI();
