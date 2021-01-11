#include <stdio.h>
#include "BVCSP.h"
#include "main.h"


#ifdef _MSC_VER
    #ifdef _WIN64
        #pragma comment(lib, "BVCSP_x64.lib")
    #elif _WIN32
        #pragma comment(lib, "BVCSP.lib")
    #endif // _WIN32
#endif // _MSC_VER

// BVCSP日志回调
void Log_Callback(int level, const char* log)
{
    printf("[BVCSP LOG] %s\n", log);
}

int main()
{
    // 初始化库
    BVCSP_SetLogCallback(Log_Callback, BVCU_LOG_LEVEL_INFO);
    BVCSP_Initialize(1,0);

    // 开始认证
    Auth();

    while(1)
    {
        int iType;
        printf("0:exit  1:login  2:logout  3:setInfo  4:setServer \r\n");
        scanf("%d", &iType);
        if (iType == 0)
            break;
        if (iType == 1)
        {
            Login(false);
        }
        else if (iType == 2)
        {
            Logout();
        }
        else if (iType == 3)
        {
        }
    }

    BVCSP_Finish();
    return 0;
}