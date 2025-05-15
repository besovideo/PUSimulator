#pragma once

#include <stdint.h>
#include <iostream>
#include <string.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <process.h>
#include <Mmsystem.h>
#define bvcu_off_t _off_t
#define bvcu_stat _stat
#define bvcu_timeb _timeb
#define bvcu_ftime _ftime_s
#define bvcu_atoll _atoi64
#define snprintf _snprintf
#else
#include <unistd.h>
#include <string.h>
#include "sys/time.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define __STDCALL__ __attribute__((__stdcall__))

#if !defined(_TRUNCATE)
#define _TRUNCATE ((unsigned int)-1)
#endif

char* strfcpy(char* dst, size_t siz, const char* src, size_t max_count);

#define strncpy_s(dest, num, src, count) strfcpy(dest, num, src, count)
#define strcpy_s(dest, num, src) strfcpy(dest, num, src, _TRUNCATE)
#define sprintf_s sprintf
#define _snprintf snprintf
#define _snprintf_s(str, num, count, sFormat, ...) snprintf(str, num, sFormat, __VA_ARGS__)
#define _vsnprintf_s(str, num, count, sFormat, args) vsnprintf(str, num, sFormat, args)
#define _strdup strdup
#define _strcmpi strcasecmp
#define strcmpi strcasecmp
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define Sleep(ms) usleep(ms*1000);
#define sscanf_s sscanf
#define gets_s(str) fgets(str, sizeof(str), stdin);

int GetTickCount();
#endif

class MacAddressInfo
{
public:
    int32_t ComboIndex;
    int32_t Index;
    char AdapterName[256];
    char Description[132];
    char MacAddr[128];
    char IpAddress[16];
    char IpMask[16];
    char Gateway[16];
};
// 获取网卡地址
int u_getMacAddress(MacAddressInfo& data);

int Utf8ToAnsi(char* _dir, int _len, const char* _src);
int AnsiToUtf8(char* _dir, int _len, const char* _src);


int base64_encode(const char* src, int srcLen, char* pBuf, int bufLen); // 返回编码后字符串长度（不包含\0）。
int base64_decode(const char* str, char* pBuf, int bufLen); // 返回解码后数据长度。


const char* extract_filename(const char* pathname);

/************************************************************************/
/* 互斥锁                                                               */
/************************************************************************/
#ifdef _MSC_VER
typedef CRITICAL_SECTION    bvcu_mutex_t;
#define bvcu_mutex_create   InitializeCriticalSection
#define bvcu_mutex_destroy  DeleteCriticalSection
#define bvcu_mutex_lock     EnterCriticalSection
#define bvcu_mutex_trylock  TryEnterCriticalSection
#define bvcu_mutex_unlock   LeaveCriticalSection
#else /*POSIX - Linux*/
typedef pthread_mutex_t        bvcu_mutex_t;
#define bvcu_mutex_create(mutex) { pthread_mutexattr_t attr; pthread_mutexattr_init(&attr); pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); pthread_mutex_init(mutex, &attr); pthread_mutexattr_destroy(&attr); }
#define bvcu_mutex_destroy  pthread_mutex_destroy
#define bvcu_mutex_lock     pthread_mutex_lock
#define bvcu_mutex_trylock  !pthread_mutex_trylock
#define bvcu_mutex_unlock   pthread_mutex_unlock
#define BOOL bool
typedef struct
{
    BOOL state;
    BOOL manual_reset;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}event_t;
#define HANDLE event_t*
#endif

class CCritSec {
private:
    // make copy constructor and assignment operator inaccessible
    CCritSec(const CCritSec& refCritSec);

    bvcu_mutex_t m_CritSec;
public:
    CCritSec() { bvcu_mutex_create(&m_CritSec); }
    ~CCritSec() { bvcu_mutex_destroy(&m_CritSec); }
    void Lock() { bvcu_mutex_lock(&m_CritSec); }
    BOOL Trylock() { return bvcu_mutex_trylock(&m_CritSec); }
    void Unlock() { bvcu_mutex_unlock(&m_CritSec); }
};
