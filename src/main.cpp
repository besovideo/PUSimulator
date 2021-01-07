#include <stdio.h>
#include "BVCSP.h"

#ifdef _MSC_VER
    #ifdef _WIN64
        #pragma comment(lib, "BVCSP_x64.lib")
    #elif _WIN32
        #pragma comment(lib, "BVCSP.lib")
    #endif // _WIN32
#endif // _MSC_VER

int main()
{
	BVCSP_Initialize(1,0);

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