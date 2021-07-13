#include <stdio.h>
#include <process.h>

#include "BVCSP.h"
#include "main.h"


#ifdef _MSC_VER
#include <windows.h>
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

// 用来定时 读取音视频、GPS、串口数据，并发送。
unsigned __stdcall Wall_App(void*)
{
    while (true)
    {
        HandleEvent();
        Sleep(5);
    }
    return 0;
}

int main()
{
    // 初始化库
    BVCSP_SetLogCallback(Log_Callback, BVCU_LOG_LEVEL_INFO);
    BVCSP_Initialize(1,0);

    // 开始认证
    Auth();

    _beginthreadex(NULL, 0, Wall_App, NULL, 0, NULL);

    while(1)
    {
        int iType;
        printf("0:exit  1:login  2:logout  3:alarm  4:setServer \r\n");
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
            SendAlarm();
        }
    }
    Logout();
    BVCSP_Finish();
    return 0;
}