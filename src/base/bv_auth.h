#pragma once

#include <stdint.h>
#include <iostream>
#include <string>
#include "BVCSP.h"
#include "BVAuth.h"

typedef void (*OnAuthEvent)(int32_t authCode, BVRAuthResult result, struct BVRAuthParam* info);

struct AuthInitParam
{
    // 初始化硬件认证信息
    struct BVRAuthInfo termInfo;

    // 离线认证文件路径
    char authFilePath[512];

    // 开发者信息, 需要申请
    char appId[128];
    char appRSA_N[256];
    char appRSA_E[256];

    // 认证事件回调
    OnAuthEvent onEvent;
};

// 设置 认证相关参数和回调函数
int SetAuthParam(const AuthInitParam& param);

bool IsAuthSuccess();

int GetAuthCode();
