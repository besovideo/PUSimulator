#pragma once
#include "PUConfig.h"
#include "BVSearch.h"
#include "CMSConfig.h"
#include "BVEvent.h"
#include "BVCSP.h"

#include "dialog.h"
#include "filetransfer.h"

#define MAX_AV_CHANNEL_COUNT 8 // 最大支持的音视频通道数（可以改）

// GPS定位接口
struct bvGPSParam
{
    int iReportInterval; // 定位上报间隔，单位：秒。
    // 收到服务器(取消)订阅GPS, bStart: 1:开始订阅，0:取消订阅, iInterval: 上报间隔(秒)
    BVCU_Result(*OnSubscribeGPS)(int bStart, int iInterval);        // 开始订阅后，上层需要定时调用SendGPSData接口上报定位.
    BVCU_PUCFG_GPSData* (*OnGetGPSData)();                           // 获取当前位置, NULL: 没有位置.
    BVCU_PUCFG_GPSParam* (*OnGetGPSParam)();                         // 获取当前配置
    BVCU_Result(*OnSetGPSParam)(const BVCU_PUCFG_GPSParam* pParam); // 修改GPS配置
};

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
    // 其它命令
    virtual BVCU_PUCFG_RecordStatus* OnGetRecordStatus() = 0;                     // 获取录像状态
    virtual BVCU_Search_Response* OnGetRecordFiles(BVCU_Search_Request* req) = 0; // 获取录像文件列表
    virtual BVCU_Result OnManualRecord(bool bStart) = 0;                          // 开始/停止 手动录像.
    // virtual BVCU_Search_Response *OnSnapshotOne() = 0;                            // 抓拍并上传一张图片。

public:
    CPUSessionBase(const char* ID);
    virtual ~CPUSessionBase();

    // 上线控制,proto:0-UDP,1-TCP; timeout:毫秒; iKeepaliveTnterval:毫秒。
    void SetServer(const char* IP, int udp_port, int tcp_port, int proto, int timeout, int iKeepaliveInterval);
    // 上线服务器,through上线网络类型BVCU_PU_ONLINE_THROUGH_， lat,lng 当前设备WGS84坐标位置
    int Login(int through, int lat, int lng);
    int Logout();                                               // 下线
    bool BLogining() { return (!m_bOnline && m_session != 0); } // 是否正在上线。
    bool BOnline() { return m_bOnline; }                        // 是否已经上线。
    bool BLogouting() { return m_bBusying && m_session == 0; }
    bool BOffline() { return (!m_bOnline && m_session == 0); } // 是否离线状态

    // 给服务器发GPS实时位置数据.
    int SendGPSData(const BVCU_PUCFG_GPSData* gpsData) {
        BVCU_PUCFG_GPSDatas gpss;
        memset(&gpss, 0, sizeof(gpss));
        gpss.pGPSDatas = (BVCU_PUCFG_GPSData*)gpsData;
        gpss.iCount = 1;
        return SendNotify(BVCU_SUBMETHOD_SUBSCRIBE_GPS, &gpss);
    }
    // 给服务器发GPS历史位置数据.用于补录离线过程中产生的位置数据。gpsData是数组。
    int SendGPSHistory(const BVCU_PUCFG_GPSData* gpsData, int count) {
        BVCU_PUCFG_GPSDatas gpss;
        memset(&gpss, 0, sizeof(gpss));
        gpss.pGPSDatas = (BVCU_PUCFG_GPSData*)gpsData;
        gpss.iCount = count;
        gpss.bMakeup = 1;
        return SendNotify(BVCU_SUBMETHOD_SUBSCRIBE_GPS, &gpss);
    }
    // 给服务器发请求. 报警类型，子设备号，报警值，是否是结束报警，报警描述。
    int SendAlarm(int alarmType, int index, int value, int bEnd, const char* desc);
    // 给服务器发命令.
    int SendCommand(int iMethod, int iSubMethod, char* pTargetID, void* pData, void* pUserData);
    // 给服务器发通知.
    int SendNotify(int iSubMethod, void* pData);

    void SetUser(const char* id, const char* passwd);
    // 设置设备信息
    void SetName(const char* Name);                       // 设备名称
    void SetManufacturer(const char* Manufacturer);       // 制造商名字
    void SetProductName(const char* ProductName);         // 产品名
    void SetSoftwareVersion(const char* SoftwareVersion); // 软件版本
    void SetHardwareVersion(const char* HardwareVersion); // 硬件版本
    void SetWIFICount(int count);                         // WIFI数目
    void SetRadioCount(int count);                        // 无线模块数目
    void SetVideoInCount(int count);                      // 视频输入数
    void SetAudioInCount(int count);                      // 音频输入数
    void SetAudioOutCount(int count);                     // 音频输出数
    void SetAlertInCount(int count);                      // 报警输入数
    void SetAlertOutCount(int count);                     // 报警输出数
    void SetStorageCount(int count);                      // 存储设备数
    void SetPTZCount(int count);                          // 云台设备数
    void SetBootDuration(int duration);                   // 已开机时长，秒。
    void SetDevicePosition(int lat, int lng);             // 设备配置位置。
    // 注册通道信息
    int AddAVChannel(CAVChannelBase* pChannel);   // 添加音视频通道
    void AddGPSInterface(const bvGPSParam* pGPS); // 添加GPS接口
    // 注册文件接口类
    void SetFileManager(CFileTransManager* pFile)
    {
        m_fileManager = pFile;
        pFile->m_deviceInfo = &m_deviceInfo;
    } // 添加文件接口类

    // 文件操作
    int UploadFile(BVCU_File_HTransfer* phTransfer, const char* localFilePathName, const bvFileInfo* fileInfo);

    // get相关接口
    BVCU_PUCFG_DeviceInfo* GetDeviceInfo() { return &m_deviceInfo; }

protected:
    BVCU_PUCFG_DeviceInfo m_deviceInfo;

    CAVChannelBase* m_avChannels[MAX_AV_CHANNEL_COUNT];
    bvGPSParam m_GPSInterface;
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
    static BVCU_Result OnAudioRecv(BVCSP_HDialog hDialog, BVCSP_Packet* pPacket);
    static void OnCommandBack(BVCSP_HSession hSession, BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam);
};
