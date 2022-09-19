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

static bool bRun = true;
static int cmdType = 0;
// 用来定时 读取音视频、GPS、串口数据，并发送。
unsigned __stdcall Wall_App(void*)
{
    // 初始化库
    BVCSP_SetLogCallback(Log_Callback, BVCU_LOG_LEVEL_INFO);
    BVCSP_Initialize(0, 0);
    BVCSP_HandleEvent(); // 通知库内部使用当前线程作为事务线程。

    // 开始认证
    Auth();

    while (bRun)
    {
        HandleEvent();
        BVCSP_HandleEvent();

        if (cmdType == 1)
        {
            Login(false);
        }
        else if (cmdType == 2)
        {
            Logout();
        }
        else if (cmdType == 3)
        {
            SendAlarm();
        }
        else if (cmdType == 4)
        {
            SendCommand();
        }
        cmdType = 0;
    }

    Logout();
    BVCSP_Finish();
    return 0;
}

int main()
{
    _beginthreadex(NULL, 0, Wall_App, NULL, 0, NULL);


    while (1)
    {
        int iType;
        printf("0:exit  1:login  2:logout  3:alarm 4:command\r\n");
        if (scanf_s("%d", &iType) != 1)
            break;
        if (iType == 0)
            break;
        cmdType = iType;
    }
    bRun = false;
    Sleep(500); // 等待Wall_App线程退出。
    return 0;
}