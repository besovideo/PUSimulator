#pragma once

#include <stdint.h>
#include <iostream>
#include <string.h>

class MacAddressInfo
{
public:
    int32_t ComboIndex;
    std::string AdapterName;
    std::string Description;
    std::string MacAddr;
    int32_t Index;
    std::string IpAddress;
    std::string IpMask;
    std::string Gateway;
};
// 获取网卡地址
int u_getMacAddress(MacAddressInfo& data);

