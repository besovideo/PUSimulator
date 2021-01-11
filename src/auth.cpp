#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "BVCSP.h"

#include "utils.h"
#include "auth/bv_auth.h"
#include "config.h"
#include "main.h"


// 认证事件回调
void my_OnAuthEvent(int32_t authCode, BVRAuthResult result, struct BVRAuthParam* info)
{
    if (result == AUTH_Result_OK)
    {
        printf("======================= auth OK ==========================\n");
        Login(true);
    }
    else {
        printf("======================= auth code: %d result: %d\n", authCode, result);
    }
}


// 认证请求硬件信息回调
int32_t my_auth_inf(struct BVRAuthInfo* termInfo)
{
    // 获取MAC地址(仅支持windows)
    MacAddressInfo macInfo;
    u_getMacAddress(macInfo);
    printf("[auth] mac addaress: %s\n", macInfo.MacAddr.data());
    const char* macAddress = macInfo.MacAddr.data();
    // 获取配置信息
    PUConfig config;
    LoadConfig(&config);
    printf("[auth] puid : %s\n", config.ID);

    // 以下内容需要如实填写, 没有数据的可以留空########################
    // 以下内容需要如实填写, 没有数据的可以留空########################
    // 以下内容需要如实填写, 没有数据的可以留空########################
    u_snprintf(termInfo->MAC, sizeof(termInfo->MAC) - 1, macAddress);
    u_snprintf(termInfo->ID, sizeof(termInfo->ID), config.ID);
    u_snprintf(termInfo->ModelNumber, sizeof(termInfo->ModelNumber) - 1, "");
    u_snprintf(termInfo->IMEI, sizeof(termInfo->IMEI) - 1, "");
    u_snprintf(termInfo->HardwareProvider, sizeof(termInfo->HardwareProvider) - 1, "");
    u_snprintf(termInfo->HardwareSN, sizeof(termInfo->HardwareSN) - 1, "");
    u_snprintf(termInfo->HardwareVersion, sizeof(termInfo->HardwareVersion) - 1, "");
    u_snprintf(termInfo->SoftwareProvider, sizeof(termInfo->SoftwareProvider) - 1, "");
    u_snprintf(termInfo->SoftwareVersion, sizeof(termInfo->SoftwareVersion) - 1, "");
    u_snprintf(termInfo->OSType, sizeof(termInfo->OSType) - 1, "");
    u_snprintf(termInfo->OSVersion, sizeof(termInfo->OSVersion) - 1, "");
    u_snprintf(termInfo->OSID, sizeof(termInfo->OSID) - 1, "");
    u_snprintf(termInfo->CPU, sizeof(termInfo->CPU) - 1, "");
    u_snprintf(termInfo->Desc, sizeof(termInfo->Desc) - 1, "");
    u_snprintf(termInfo->UserLabel, sizeof(termInfo->UserLabel) - 1, ""); // 用户标签, 指定设备对用户可见

    return 0;
}


int Auth()
{
    // =============  请根据您的情况修改下面参数  ================
    // 保存认证离线文件的路径+文件名
    std::string authFilePath = "bv_auth.dat";
    // 认证终端类型："PU"/"MPU"/"MCP"等，不同终端类型认证证书不同，可能费用不同。
    std::string appDevType = "PU";
    // 请修改为您申请的开发者密码，下面的密钥是测试开发者密钥（会不定时被注销）。
    // 请修改为您申请的开发者密码，下面的密钥是测试开发者密钥（会不定时被注销）。
    // 请修改为您申请的开发者密码，下面的密钥是测试开发者密钥（会不定时被注销）。
    std::string appId = "app_abc814f85c75374b";
    std::string appN = "f2248a26e34cdfe8297704affc7f127b";
    std::string appE = "9572f125ccd791f1";

    AuthInitParam initParam(
        authFilePath,
        appDevType, appId, appN, appE,
        my_OnAuthEvent, my_auth_inf);

    // 初始化认证
    if (initBVAuth(initParam) != 0) {
        printf("initBVAuth fail\n");
        return -1;
    }
    return 0;
}

