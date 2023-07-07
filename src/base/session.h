#pragma once
#include "PUConfig.h"
#include "CMSConfig.h"
#include "BVEvent.h"
#include "BVCSP.h"

#include "dialog.h"
#include "filetransfer.h"

#define MAX_AV_CHANNEL_COUNT 8 // 最大支持的音视频通道数（可以改）
#define MAX_GPS_CHANNEL_COUNT 4 // 最大支持的GPS通道数（可以改）
#define MAX_TSP_CHANNEL_COUNT 8 // 最大支持的串口通道数（可以改）

class CPUSessionBase
{
protected:
    // ============   下面的要注意了，需要实现 ===============
    // 收到配置设备信息命令。成功返回0。添加其它命令的支持，可以参考OnSetInfo的实现原理。
    // name:设备名称。lat,lng 设备WGS84坐标位置,1/10000000.
    virtual BVCU_Result OnSetInfo(const char* name, int lat, int lng) = 0;
    // 上线服务器结果回调通知.
    virtual void OnLoginEvent(BVCU_Result iResult) = 0;
    // 服务器掉线回调通知.
    virtual void OnOfflineEvent(BVCU_Result iResult) = 0;
    // 服务器命令回复回调. 给服务器发命令后，通过该接口处理回复。
    virtual void OnCommandReply(BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam) = 0;

public:
    CPUSessionBase(const char* ID);
    virtual ~CPUSessionBase();

    // 上线控制
    void SetServer(const char* IP, int port, int proto, int timeout, int iKeepaliveInterval);
    // 上线服务器,through上线网络类型BVCU_PU_ONLINE_THROUGH_， lat,lng 当前设备WGS84坐标位置
    int  Login(int through, int lat, int lng);
    int  Logout();    // 下线
    bool BLogining() { return (!m_bOnline && m_session != 0); }  // 是否正在上线。
    bool BOnline() { return m_bOnline; }  // 是否已经上线。
    bool BLogouting() { return m_bBusying && m_session == 0; }
    bool BOffline() { return (!m_bOnline && m_session == 0); } // 是否离线状态

    // 给服务器发请求. 报警类型，子设备号，报警值，是否是结束报警，报警描述。
    int  SendAlarm(int alarmType, int index, int value, int bEnd, const char* desc);
    // 给服务器发命令. 
    int  SendCommand(int iMethod, int iSubMethod, char* pTargetID, void* pData, void* pUserData);

    void SetUser(const char* id, const char* passwd);
    // 设置设备信息
    void SetName(const char* Name);  // 设备名称
    void SetManufacturer(const char* Manufacturer); // 制造商名字
    void SetProductName(const char* ProductName);   // 产品名
    void SetSoftwareVersion(const char* SoftwareVersion); // 软件版本
    void SetHardwareVersion(const char* HardwareVersion); // 硬件版本
    void SetWIFICount(int count);// WIFI数目
    void SetRadioCount(int count);// 无线模块数目
    void SetVideoInCount(int count);  // 视频输入数
    void SetAudioInCount(int count);  // 音频输入数
    void SetAudioOutCount(int count);  // 音频输出数
    void SetAlertInCount(int count);  // 报警输入数
    void SetAlertOutCount(int count); // 报警输出数
    void SetStorageCount(int count);  // 存储设备数
    void SetPTZCount(int count);  // 云台设备数
    void SetBootDuration(int duration); // 已开机时长，秒。
    void SetDevicePosition(int lat, int lng);  // 设备配置位置。
    // 注册通道信息
    int AddAVChannel(CAVChannelBase* pChannel); // 添加音视频通道
    int AddGPSChannel(CGPSChannelBase* pChannel); // 添加GPS通道, return 硬件号
    int AddTSPChannel(CTSPChannelBase* pChannel); // 添加串口通道
    // 注册文件接口类
    void SetFileManager(CFileTransManager* pFile) { m_fileManager = pFile; }  // 添加文件接口类

    // 文件操作
    int UploadFile(BVCU_File_HTransfer* phTransfer, const char* localFilePathName, const char* remoteFileName);

    // get相关接口
    BVCU_PUCFG_DeviceInfo* GetDeviceInfo() { return &m_deviceInfo; }

protected:
    BVCU_PUCFG_DeviceInfo  m_deviceInfo;

    CAVChannelBase* m_avChannels[MAX_AV_CHANNEL_COUNT];
    CGPSChannelBase* m_GPSChannels[MAX_GPS_CHANNEL_COUNT];
    CTSPChannelBase* m_TSPChannels[MAX_TSP_CHANNEL_COUNT];
    CFileTransManager* m_fileManager;

    BVCSP_SessionParam m_sesParam;
    BVCSP_HSession m_session;
    bool m_bOnline;
    bool m_bBusying; // 是否正在上线或下线。

public:
    // 获取
    CChannelBase* GetChannelBase(int channelIndex);
    CFileTransManager* GetFileManager() { return m_fileManager; }

    // BVCSP 的回调
    static void OnSessionEvent(BVCSP_HSession hSession, int iEventCode, void* pParam);
    static BVCU_Result OnCommand(BVCSP_HSession hSession, BVCSP_Command* pCommand);
    static BVCU_Result OnNotify(BVCSP_HSession hSession, BVCSP_NotifyMsgContent* pData);
    static BVCU_Result OnDialogCmd(BVCSP_HDialog hDialog, int iEventCode, BVCSP_DialogParam* pParam);
    static void OnDialogEvent(BVCSP_HDialog hDialog, int iEventCode, BVCSP_Event_DialogCmd* pParam);
    static BVCU_Result afterDialogRecv(BVCSP_HDialog hDialog, BVCSP_Packet* pPacket);
    static void OnCommandBack(BVCSP_HSession hSession, BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam);
};

