#include "utils.h"
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
