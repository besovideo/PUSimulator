#include <string>

#include "BVCSP.h"
#include "config.h"

#include "base/session.h"

class CPUSession : public CPUSessionBase
{
public:
    CPUSession(const char* ID);
    ~CPUSession();

private:
    virtual int OnSetInfo(const char* name, int lat, int lng);
    virtual void OnLoginEvent(BVCU_Result iResult);
    virtual void OnOfflineEvent(BVCU_Result iResult);
};

CPUSession::CPUSession (const char* ID)
    : CPUSessionBase(ID)
{
}

CPUSession ::~CPUSession()
{
}

// 重构设置设备信息接口，保存平台下发的配置信息，回复0：成功。
int CPUSession::OnSetInfo(const char* name, int lat, int lng)
{
    PUConfig puconfig;
    LoadConfig(&puconfig);
    strncpy_s(puconfig.Name, sizeof(puconfig.Name), name, _TRUNCATE);
    puconfig.lat = lat;
    puconfig.lng = lng;
    SetConfig(&puconfig);
    return 0;
}
// 在线状态变化通知，登录/退出 服务器
void CPUSession::OnLoginEvent(BVCU_Result iResult)
{
    if (BOnline())
    {
        printf("============================ pu online ==============================\n");
    }
    else
    {
        printf("======================== login faild: %d ==========================\n", iResult);
    }
}
void CPUSession::OnOfflineEvent(BVCU_Result iResult)
{
    printf("======================== server offline: %d ==========================\n", iResult);
}

static CPUSession* pSession = 0;  // 全局Session

// 登录服务器。从配置文件中读取设备信息，和服务器信息；请提前设置好。
int Login(bool autoOption)
{
    if (autoOption && pSession != 0)
    {  // 自动操作不能重复登录。
        return 0;
    }
    if (pSession == 0)
    {
        PUConfig puconfig;
        LoadConfig(&puconfig);
        pSession = new CPUSession(puconfig.ID);
        if (pSession)
        {
            // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
            // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
            // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
            pSession->SetName(puconfig.Name);
            pSession->SetServer(puconfig.serverIP, puconfig.serverPort, puconfig.protoType, 60 * 1000, 4 * 1000);
            pSession->SetDevicePosition(puconfig.lat, puconfig.lng);
        }
    }
    if (pSession == 0)
        return -1;
    // 上线服务器，lat,lng 应该改为当前设备定位位置。
    return pSession->Login(BVCU_PU_ONLINE_THROUGH_ETHERNET, 200*10000000, 200 * 10000000);
}

int Logout()
{
    if (pSession != 0)
    {
        return pSession->Logout();
    }
    return -1;
}
