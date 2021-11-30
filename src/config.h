#pragma once

// 默认配置
#define DEFAULT_ID "PU_55AA0000"
#define DEFAULT_NAME "PUSimulator G1B2"
#define DEFAULT_SERVERIP "192.168.6.57"
#define DEFAULT_SERVERPORT 9702
#define DEFAULT_PROTOTYPE 0
#define DEFAULT_GPS_INTERVAL 5
// 配置文件路径+名称
#define CONFIG_FILE_PATH_NAME "./pusimulator.ini"

struct PUConfig
{
    // info
    int  PUCount;  // 设备个数
    char ID[32];   // 设备ID号
    char Name[64]; // 设备名称
    int  lat;      // 维度，1/1000000
    int  lng;      // 经度，1/1000000
    // server
    char serverIP[64]; // 上线服务器地址
    int  serverPort;   // 上线服务器端口
    int  protoType;    // 协议类型：0 : UDP, 其它值：TCP。
    int  relogin;      // 是否断线重连：0：否，其他值：间隔时间，秒。
    // gps
    int  interval;     // 上报间隔：秒
    char gpsName[64];  // GPS 通道名称
    // 媒体通道
    char mediaName[64];  // GPS 通道名称
    char audioFile[64];  // 音频文件名称
};

// 加载配置信息，pConfig带回
void LoadConfig(PUConfig* pConfig);

// 保存配置信息。
int  SetConfig(const PUConfig* pConfig);

