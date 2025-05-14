#include "utils.h"
#include <cstdlib>
#include <stdio.h>
#ifdef _MSC_VER
#include <winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>

#include <process.h>
#else
#include <iconv.h>
#endif // _MSC_VER

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#endif

// 获取MAC地址
#if _MSC_VER
int u_getMacAddress(MacAddressInfo& data)
{
    IPAddr add = 0x08080808; // 用8.8.8.8这个IP
    DWORD index = -1;
    DWORD ret = GetBestInterface(add, &index);

    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = -1;

    do
    {
        ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
        if (pAdapterInfo == NULL)
        {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            dwRetVal = -1;
            break;
        }
        // Make an initial call to GetAdaptersInfo to get
        // the necessary size into the ulOutBufLen variable
        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
        {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
            if (pAdapterInfo == NULL)
            {
                printf("Error allocating memory needed to call GetAdaptersinfo\n");
                dwRetVal = -2;
                break;
            }
        }

        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
        if (dwRetVal != NO_ERROR)
        {
            printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
            break;
        }

        pAdapter = pAdapterInfo;
        while (pAdapter)
        {
            if (pAdapter->Index != index)
            {
                pAdapter = pAdapter->Next;
                continue;
            }
            data.ComboIndex = pAdapter->ComboIndex;
            strncpy_s(data.AdapterName, sizeof(data.AdapterName), pAdapter->AdapterName, _TRUNCATE);
            strncpy_s(data.Description, sizeof(data.Description), pAdapter->Description, _TRUNCATE);
            memset(data.MacAddr, 0, sizeof(data.MacAddr));
            for (UINT i = 0; i < pAdapter->AddressLength; i++)
            {
                if (i == (pAdapter->AddressLength - 1))
                {
                    _snprintf(data.MacAddr + i * 3, 3, "%.2X", (int)pAdapter->Address[i]);
                }
                else
                {
                    _snprintf(data.MacAddr + i * 3, 3, "%.2X-", (int)pAdapter->Address[i]);
                }
            }
            data.Index = pAdapter->Index;
            memcpy(data.IpAddress, pAdapter->IpAddressList.IpAddress.String, sizeof(pAdapter->IpAddressList.IpAddress.String));
            memcpy(data.IpMask, pAdapter->IpAddressList.IpMask.String, sizeof(pAdapter->IpAddressList.IpMask.String));
            memcpy(data.Gateway, pAdapter->GatewayList.IpAddress.String, sizeof(pAdapter->GatewayList.IpAddress.String));

            // pAdapter = pAdapter->Next;

            dwRetVal = 0;
            break;
        }

    } while (false);

    if (pAdapterInfo)
    {
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
    if (NULL == wchar)
    {
        _dir[0] = 0;
        return FALSE;
    }

    MultiByteToWideChar(CP_UTF8, 0, _src, -1, wchar, len);
    WideCharToMultiByte(CP_ACP, 0, wchar, -1, _dir, _len, NULL, NULL);
    delete[] wchar;
#else
    size_t inlen = 0, outlen = _len;
    do
    {
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
    if (NULL == wchar)
    {
        _dir[0] = 0;
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, _src, -1, wchar, len);
    WideCharToMultiByte(CP_UTF8, 0, wchar, -1, _dir, _len, NULL, NULL);
    delete[] wchar;
#else
    size_t inlen = 0, outlen = _len;
    do
    {
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


int base64_encode(const char* src, int srcLen, char* pBuf, int bufLen)
{
    int len;
    char* res = pBuf;
    int y3 = srcLen % 3;
    //定义base64编码表  
    static const char* base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    //计算经过base64编码后的字符串长度  
    if (y3 == 0)
        len = (srcLen / 3) * 4;
    else
        len = (srcLen / 3 + 1) * 4;
    if (bufLen < len + 1)
        return 0;

    res[len] = '\0';

    int i, j, jc = srcLen - 3;
    //以3个8位字符为一组进行编码  
    for (i = 0, j = 0; j <= jc; j += 3, i += 4)
    {
        res[i] = base64_table[(src[j] >> 2) & 0x3f]; //取出第一个字符的前6位并找出对应的结果字符  
        res[i + 1] = base64_table[((src[j] & 0x3) << 4) | ((src[j + 1] >> 4) & 0xf)]; //将第一个字符的后位2位与第二个字符的前4位进行组合并找到对应的结果字符  
        res[i + 2] = base64_table[(src[j + 1] & 0xf) << 2 | ((src[j + 2] >> 6) & 0x3)]; //将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符  
        res[i + 3] = base64_table[src[j + 2] & 0x3f]; //取出第三个字符的后6位并找出结果字符  
    }

    switch (y3)
    {
    case 1:
        res[i] = base64_table[(src[j] >> 2) & 0x3f]; //取出第一个字符的前6位并找出对应的结果字符  
        res[i + 1] = base64_table[(src[j] & 0x3) << 4]; //将第一个字符的后位与第二个字符的前4位进行组合并找到对应的结果字符  
        res[i + 2] = '=';
        res[i + 3] = '=';
        break;
    case 2:
        res[i] = base64_table[(src[j] >> 2) & 0x3f]; //取出第一个字符的前6位并找出对应的结果字符  
        res[i + 1] = base64_table[((src[j] & 0x3) << 4) | ((src[j + 1] >> 4) & 0xf)]; //将第一个字符的后位与第二个字符的前4位进行组合并找到对应的结果字符  
        res[i + 2] = base64_table[(src[j + 1] & 0xf) << 2]; //将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符 
        res[i + 3] = '=';
        break;
    }

    return len;
}

int base64_decode(const char* str, char* pBuf, int bufLen)
{
    //根据base64表，以字符找到对应的十进制数据  
    int table[] = { 0,0,0,0,0,0,0,0,0,0,0,0,
             0,0,0,0,0,0,0,0,0,0,0,0,
             0,0,0,0,0,0,0,0,0,0,0,0,
             0,0,0,0,0,0,0,62,0,0,0,
             63,52,53,54,55,56,57,58,
             59,60,61,0,0,0,0,0,0,0,0,
             1,2,3,4,5,6,7,8,9,10,11,12,
             13,14,15,16,17,18,19,20,21,
             22,23,24,25,0,0,0,0,0,0,26,
             27,28,29,30,31,32,33,34,35,
             36,37,38,39,40,41,42,43,44,
             45,46,47,48,49,50,51
    };
    long len;
    long str_len;
    char* res = pBuf;

    //计算解码后的字符串长度  
    len = strlen(str);
    //判断编码后的字符串后是否有=  
    if (strstr(str, "=="))
        str_len = len / 4 * 3 - 2;
    else if (strstr(str, "="))
        str_len = len / 4 * 3 - 1;
    else
        str_len = len / 4 * 3;

    if (bufLen < str_len + 3)
        return 0;

    int i, j;
    //以4个字符为一位进行解码  
    for (i = 0, j = 0; i < len - 2; j += 3, i += 4)
    {
        res[j] = ((unsigned char)table[str[i]]) << 2 | (((unsigned char)table[str[i + 1]]) >> 4); //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的后2位进行组合  
        res[j + 1] = (((unsigned char)table[str[i + 1]]) << 4) | (((unsigned char)table[str[i + 2]]) >> 2); //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应bas464表的十进制数的后4位进行组合  
        res[j + 2] = (((unsigned char)table[str[i + 2]]) << 6) | ((unsigned char)table[str[i + 3]]); //取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合  
    }

    return str_len;
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
    static timespec g_GetTickCount_ts = { 0, 0 };

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

const char* extract_filename(const char* pathname)
{
    if (pathname == NULL)
        return NULL;

    // 查找最后一个路径分隔符（兼容 Windows 和 Unix 路径）
    const char* last_sep = strrchr(pathname, '/');
    const char* last_sep_win = strrchr(pathname, '\\');

    // 选择更靠后的分隔符位置（Windows 路径可能同时包含两种符号）
    const char* last_sep_final = (last_sep_win > last_sep) ? last_sep_win : last_sep;

    if (last_sep_final == NULL)
    {
        // 无分隔符，直接返回原路径的副本
        return pathname;
    }
    return last_sep_final;
}
