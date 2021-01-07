#ifndef __AUTH_HPP__
#define __AUTH_HPP__


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include "utils.h"
#include "auth/bv_auth.h"
#include "BVCSP.h"


// 认证事件回调
void my_OnAuthEvent(int32_t authCode, BVRAuthResult result, struct BVRAuthParam* info)
{
    printf("======================= auth code: %d result: %d\n", authCode, result);
}


// 认证请求硬件信息回调
int32_t my_auth_inf(struct BVRAuthInfo* termInfo)
{
    MacAddressInfo macInfo;

    // 获取MAC地址(仅支持windows)
    u_getMacAddress(macInfo);
    printf("[auth] mac addaress: %s\n", macInfo.MacAddr.data());

    const char* macAddress = macInfo.MacAddr.data();

    // 以下内容需要如实填写, 没有数据的可以留空########################
    u_snprintf(termInfo->MAC, sizeof(termInfo->MAC) - 1, macAddress);
    u_snprintf(termInfo->ID, sizeof(termInfo->ID), "PU_55AAAA");
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
    std::string authFilePath = "bv_auth.dat";
    std::string appDevType = "PU";
    std::string appId = "app_abc814f85c75374b";
    std::string appN = "f2248a26e34cdfe8297704affc7f127b";
    std::string appE = "9572f125ccd791f1";

    AuthInitParam initParam(
        authFilePath,
        appDevType, appId, appN, appE,
        my_OnAuthEvent, my_auth_inf);

    // 初始化认证
    if (!initBVAuth(initParam)) {
        printf("initBVAuth fail\n");
        return -1;
    }
    return 0;
}

#endif
