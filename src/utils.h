#pragma once

#include <stdint.h>
#include <iostream>
#include <string.h>

#ifdef __linux__
#include <unistd.h>
#include <string.h>
#include "sys/time.h"

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#define __STDCALL__ __attribute__((__stdcall__))

#if !defined(_TRUNCATE)
#define _TRUNCATE ((unsigned int)-1)
#endif

char* strfcpy(char* dst, size_t siz, const char* src, size_t max_count);

#define strncpy_s(dest,num,src,count) strfcpy(dest,num, src, count)
#define strcpy_s(dest,num,src) strfcpy(dest,num, src, _TRUNCATE)
#define sprintf_s sprintf
#define _snprintf snprintf
#define _snprintf_s(str,num,count,sFormat,...) snprintf(str,num,sFormat, __VA_ARGS__)
#define _vsnprintf_s(str,num,count,sFormat,args) vsnprintf(str,num,sFormat, args)
#define _strdup strdup
#define _strcmpi strcasecmp
#define strcmpi strcasecmp
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define Sleep usleep
#define sscanf_s sscanf
#define gets_s(str) fgets(str, sizeof(str), stdin);

int GetTickCount();
#endif

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

int Utf8ToAnsi(char* _dir, int _len, const char* _src);
int AnsiToUtf8(char* _dir, int _len, const char* _src);

