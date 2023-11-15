#include <string>

#include "BVCSP.h"
#include "config.h"

#include "base/session.h"
#include "gps.h"
#include "tsp.h"
#include "media.h"

class CPUSession : public CPUSessionBase
{  // 设备对象
protected:
    CGPSChannel* m_pGPS;     // GPS通道对象。
    CTSPChannel* m_pTSP;     // 串口通道对象。
    CMediaChannel* m_pMedia; // 媒体通道对象。
    time_t m_lastReloginTime;// 最后一次尝试登录时间
    bool m_bNeedOnline; // 是否需要上线。
    int m_relogintime; // 是否断线重连，0：否，其他值：间隔时间，秒

    bool m_bSubGPS;     // 是否被订阅了GPS数据
    int  m_iInterval;   // 订阅GPS间隔，秒
    int  m_lastgpstime; // 最后上报GPS时间
public:
    CPUSession(const char* ID, int relogintime);
    ~CPUSession();
    void RegisterChannel();
    void HandleEvent(time_t nowTime);
    bool BFirst();
    int  Login(int through, int lat, int lng);
    int  Logout();

private:
    virtual BVCU_Result OnSetInfo(const char* name, int lat, int lng);
    virtual BVCU_Result OnSubscribeGPS(int bStart, int iInterval);
    virtual void OnLoginEvent(BVCU_Result iResult);
    virtual void OnOfflineEvent(BVCU_Result iResult);
    virtual void OnCommandReply(BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam);
};

