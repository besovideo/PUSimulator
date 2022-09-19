#pragma once

#include <stdint.h>
#include <iostream>
#include <string>
#include "BVCSP.h"
#include "BVAuth.h"


typedef void (*OnAuthEvent)(int32_t authCode, BVRAuthResult result, struct BVRAuthParam* info);
typedef int32_t(*auth_info_init)(struct BVRAuthInfo* termInfo);

class AuthInitParam
{
public:
    AuthInitParam() :onEvent(NULL), onInitData(NULL) { }

    // authFilePath: "/tmp/bv_auth.dat"
    AuthInitParam(std::string filePath,
        std::string appDevType, std::string appId, std::string appN, std::string appE,
        OnAuthEvent cb, auth_info_init cb_init)
    {
        this->authFilePath = filePath;

        this->appDevType = appDevType;
        this->appId = appId;
        this->appRSA_N = appN;
        this->appRSA_E = appE;

        this->onEvent = cb;
        this->onInitData = cb_init;
    }

public:
    std::string authFilePath;

    // 开发者信息, 需要申请
    std::string appDevType;
    std::string appId;
    std::string appRSA_N;
    std::string appRSA_E;

    // 认证事件回调
    OnAuthEvent onEvent;
    // 初始化硬件认证信息
    auth_info_init onInitData;
};


// 初始化
int32_t initBVAuth(const AuthInitParam param);

// 认证是否成功
bool isAuthSuccess();


#ifdef _MSC_VER
#define u_snprintf _snprintf_s
#else
#define u_snprintf  snprintf
#endif
