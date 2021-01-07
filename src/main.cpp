#include <stdio.h>
#include "BVCSP.h"


#ifdef _MSC_VER
    #ifdef _WIN64
        #pragma comment(lib, "BVCSP_x64.lib")
    #elif _WIN32
        #pragma comment(lib, "BVCSP.lib")
    #endif // _WIN32
#endif // _MSC_VER

// 功能接口， 实现在各个功能模块cpp中。
int Auth(); // 认证。

// BVCSP日志回调
void Log_Callback(int level, const char* log)
{
    printf("[BVCSP LOG] %s\n", log);
}

int main()
{
    BVCSP_SetLogCallback(Log_Callback, BVCU_LOG_LEVEL_INFO);
	BVCSP_Initialize(1,0);

    Auth();

	while(1)
	{
		int iType;
        printf("0:exit 1:login/logout  2:setInfo  3:setServer \r\n");
        scanf("%d", &iType);
        if (iType == 0)
            break;
        if (iType == 1)
		{
            
		}
        else if (iType == 2)
		{
		}
        else if (iType == 3)
		{
		}
	}

	BVCSP_Finish();
	return 0;
}