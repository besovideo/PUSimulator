#include "utils.h"
#include <cstdlib>
#include <stdio.h>
#include <iconv.h>
#ifdef _MSC_VER
#include <winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>

#include <process.h>
#endif // _MSC_VER


#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#endif

// 获取MAC地址
#if _MSC_VER
int u_getMacAddress(MacAddressInfo& data)
{
    IPAddr add = 0x08080808;//用8.8.8.8这个IP
    DWORD index = -1;
    DWORD ret = GetBestInterface(add, &index);

    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = -1;

    do
    {
        ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            dwRetVal = -1;
            break;
        }
        // Make an initial call to GetAdaptersInfo to get
        // the necessary size into the ulOutBufLen variable
        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
            if (pAdapterInfo == NULL) {
                printf("Error allocating memory needed to call GetAdaptersinfo\n");
                dwRetVal = -2;
                break;
            }
        }

        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
        if (dwRetVal != NO_ERROR) {
            printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
            break;
        }

        pAdapter = pAdapterInfo;
        while (pAdapter)
        {
            if (pAdapter->Index != index) {
                pAdapter = pAdapter->Next;
                continue;
            }
            data.ComboIndex = pAdapter->ComboIndex;
            data.AdapterName.assign(pAdapter->AdapterName);
            data.Description.assign(pAdapter->Description);
            char* macBuffer = (char*)malloc(pAdapter->AddressLength * 3);
            memset(macBuffer, 0, pAdapter->AddressLength * 3);
            for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                if (i == (pAdapter->AddressLength - 1)) {
                    _snprintf(macBuffer + i * 3, 3, "%.2X", (int)pAdapter->Address[i]);
                }
                else {
                    _snprintf(macBuffer + i * 3, 3, "%.2X-", (int)pAdapter->Address[i]);
                }
            }
            data.MacAddr.assign(macBuffer);
            data.Index = pAdapter->Index;
            data.IpAddress.assign(pAdapter->IpAddressList.IpAddress.String);
            data.IpMask.assign(pAdapter->IpAddressList.IpMask.String);
            data.Gateway.assign(pAdapter->GatewayList.IpAddress.String);

            //pAdapter = pAdapter->Next;

            dwRetVal = 0;
            break;
        }

    } while (false);

    if (pAdapterInfo) {
        free(pAdapterInfo);
    }

    return dwRetVal;
}
#else
int u_getMacAddress(MacAddressInfo& data)
{
    return 0;
}
#endif

int Utf8ToAnsi(char* _dir, int _len, const char* _src)
{
#ifdef _MSC_VER
    int len = MultiByteToWideChar(CP_UTF8, 0, _src, -1, 0, 0);
    wchar_t* wchar = new wchar_t[len + 1];
    if (NULL == wchar) {
        _dir[0] = 0;
        return FALSE;
    }

    MultiByteToWideChar(CP_UTF8, 0, _src, -1, wchar, len);
    WideCharToMultiByte(CP_ACP, 0, wchar, -1, _dir, _len, NULL, NULL);
    delete []wchar;
#else
    size_t inlen = 0, outlen = _len;
    do {
        iconv_t cd = 0;
        cd = iconv_open("UTF-8", "GBK18030");
        if (cd == 0 || cd == (iconv_t)-1)
            break;
        iconv_close(cd);
        cd = iconv_open("GBK18030", "UTF-8");
        if (cd == 0 || cd == (iconv_t)-1)
            break;
        inlen = strlen(_src) + 1;
        if (iconv(cd, (char**)(&_src), &inlen, &_dir, &outlen) == -1)
        {
            iconv_close(cd);
            break;
        }
        iconv_close(cd);
        return TRUE;
    } while (0);
    strncpy_s(_dir, _len, _src, _TRUNCATE);
    printf("iconv(utf82ansi) failed! %d src=%s\r\n", errno, _src);
    return FALSE;
#endif
    return TRUE;
}
int AnsiToUtf8(char* _dir, int _len, const char* _src)
{
#ifdef _MSC_VER
    int len = MultiByteToWideChar(CP_ACP, 0, _src, -1, 0, 0);
    wchar_t* wchar = new wchar_t[len + 1];
    if (NULL == wchar) {
        _dir[0] = 0;
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, _src, -1, wchar, len);
    WideCharToMultiByte(CP_UTF8, 0, wchar, -1, _dir, _len, NULL, NULL);
    delete []wchar;
#else
    size_t inlen = 0, outlen = _len;
    do {
        iconv_t cd = 0;
        cd = iconv_open("GBK18030", "UTF-8");
        if (cd == 0 || cd == (iconv_t)-1)
            break;
        iconv_close(cd);
        cd = iconv_open("UTF-8", "GBK18030");
        if (cd == 0 || cd == (iconv_t)-1)
            break;
        inlen = strlen(_src) + 1;
        if (iconv(cd, (char**)(&_src), &inlen, &_dir, &outlen) == -1)
        {
            iconv_close(cd);
            break;
        }
        iconv_close(cd);
        return TRUE;
    } while (0);
    strncpy_s(_dir, _len, _src, _TRUNCATE);
    printf("iconv(ansi2utf8) failed! %d src=%s\r\n", errno, _src);
    return FALSE;
#endif
    return TRUE;
}

#ifdef __linux__
char* strfcpy(char* dst, size_t siz, const char* src, size_t max_count)
{
    char* d = dst;
    const char* s = src;
    size_t count = 0;

    while (count < (siz - 1) && count < max_count && *s)
    {
        *d++ = *s++;
        ++count;
    }

    if (siz > 0)
        *d = '\0';
    return dst;
}

int GetTickCount()
{
    static int g_GetTickCount_t = 0;
    static timespec g_GetTickCount_ts = { 0,0 };

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if ((ts.tv_nsec - g_GetTickCount_ts.tv_nsec) > 10 * 1000000 || !g_GetTickCount_t || (ts.tv_sec > g_GetTickCount_ts.tv_sec))
    {
        int a, b;
        a = ts.tv_sec << 3;
        a *= 125;
        b = ts.tv_nsec >> 6;
        b /= 15625;
        g_GetTickCount_t = a + b;
        g_GetTickCount_ts = ts;
    }
    return g_GetTickCount_t;
}
#endif

