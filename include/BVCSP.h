#ifndef __BV_CSP_H__
#define __BV_CSP_H__

#ifdef  __cplusplus
extern "C" {
#endif
#include "SAVCodec.h"
#include "BVCUConst.h"
#include "BVCUCommon.h"
#include "PUConfig.h"

#ifdef _MSC_VER
#ifdef LIBBVCSP_EXPORTS
#define LIBBVCSP_API __declspec(dllexport)
#else
#define LIBBVCSP_API __declspec(dllimport)
#endif
#else
#define LIBBVCSP_API __attribute__ ((visibility("default")))
#endif//_MSC_VER

    // 注意：库内所有回调中不能包含任何阻塞操作（例如：互斥量），防止出现死锁。

    // Session事件
    typedef enum _BVCSP_SESSION_EVENT
    {
        BVCSP_EVENT_SESSION_OPEN = 1,  // 创建Session。事件参数：BVCU_Result
        BVCSP_EVENT_SESSION_CLOSE,     // 关闭Session。事件参数：BVCU_Result
    }BVCSP_SESSION_EVENT;
    // Dialog事件
    typedef enum _BVCSP_DIALOG_EVENT
    {
        BVCSP_EVENT_DIALOG_OPEN = 1,    // 创建Dialog。
        BVCSP_EVENT_DIALOG_UPDATE,      // 更新Dialog。
        BVCSP_EVENT_DIALOG_CLOSE,       // 关闭Dialog。
        BVCSP_EVENT_DIALOG_PLIKEY,      // PLI请求关键帧。
    }BVCSP_DIALOG_EVENT;

    // Client Type
    typedef enum _BVCSP_CLIENT_TYPE
    {
        BVCSP_CLIENT_TYPE_UNKNOWN = 0,     // 未知终端 
        BVCSP_CLIENT_TYPE_CU = (1 << 0),     // CU 客户端 
        BVCSP_CLIENT_TYPE_PU = (1 << 1),     // PU 终端 
        BVCSP_CLIENT_TYPE_NRU = (1 << 2),    // NRU终端 
        BVCSP_CLIENT_TYPE_VTDU = (1 << 3), // VTDU终端 
        BVCSP_CLIENT_TYPE_DEC = (1 << 4),  // DEC终端 
        BVCSP_CLIENT_TYPE_UA = (1 << 5),

        BVCSP_CLIENT_TYPE_CUSTOM = (1 << 8), // 之后为自定义类型 
    }BVCSP_CLIENT_TYPE;

    // Data Type
    typedef enum _BVCSP_DATA_TYPE
    {
        BVCSP_DATA_TYPE_UNKNOWN = 0, // 未知数据类型 
        BVCSP_DATA_TYPE_AUDIO, // 音频帧数据 
        BVCSP_DATA_TYPE_VIDEO, // 视频帧数据 
        BVCSP_DATA_TYPE_GPS,   // GPS数据  BVCU_PUCFG_GPSData
        BVCSP_DATA_TYPE_TSP,   // 串口数据  二进制数据流
    }BVCSP_DATA_TYPE;

    typedef enum _BVCSP_SESSION_STATUS
    {
        BVCSP_SESSION_STATUS_OFFLINE = 0,
        BVCSP_SESSION_STATUS_CONNECTED,
        BVCSP_SESSION_STATUS_UNAUTHORIZED,
        BVCSP_SESSION_STATUS_ONLINE
    }BVCSP_SESSION_STATUS;

    // 会话通道控制属性
    typedef enum _BVCSP_DIALOG_OPTIONS
    {
        BVCSP_DIALOG_OPTIONS_NOFMTP = 1 << 0, // SDP中不添加fmtp。
    }BVCSP_DIALOG_OPTIONS;

    typedef  void* BVCSP_HSession;
    typedef  void* BVCSP_HDialog;

    //=================================command=======================================
    // 发送的通知包和收到的命令回响内容 
    typedef struct _BVCSP_CmdMsgContent BVCSP_CmdMsgContent;
    struct _BVCSP_CmdMsgContent {
        /* 一个通知/回响可能包含多条信息。pNext指向下一条信息。最后一条信息的pNext应指向NULL
        每个通知/回响的信息类型和顺序是固定的。大多数通知/回响只支持一种数据类型(pNext是NULL) */
        BVCSP_CmdMsgContent* pNext;

        // 信息数目
        int iDataCount;

        /* 信息数组，数组元素个数等于iDataCount，pData[0]表示第一个成员，pData[1]表示第2个成员。
        类型由具体命令决定 */
        void* pData;
    };

    typedef struct _BVCSP_NotifyMsgContent BVCSP_NotifyMsgContent;
    struct _BVCSP_NotifyMsgContent {
        // 本结构体的大小，分配者应初始化为sizeof(BVCSP_NotifyMsgContent)
        int iSize;

        // 一个通知可能包含多条信息。pNext指向下一条信息。最后一条信息的pNext应指向NULL。
        BVCSP_NotifyMsgContent* pNext;

        // 通知内容的类型，BVCU_SUBMETHOD_
        int iSubMethod;

        // 信息源（系统中的网络实体)ID。为空表示是当前登录的Server
        char szSourceID[BVCU_MAX_ID_LEN + 1];

        // 信息源的附属设备的索引，从0开始，例如PU的云台/通道/音视频IO等。设为-1表示无意义
        int iSourceIndex;

        // 目标ID。为空表示命令目标是当前登录的Server
        char szTargetID[BVCU_MAX_ID_LEN + 1];

        // 从0开始的目标附属设备的索引，例如PU的云台/通道/音视频IO等。设为-1表示无意义
        int iTargetIndex;

        // 通知负载
        BVCSP_CmdMsgContent stMsgContent;
    };

    //    发出的通知 
    typedef struct _BVCSP_Notify BVCSP_Notify;
    struct _BVCSP_Notify
    {
        // 本结构体的大小，分配者应初始化为sizeof(BVCSP_Notify)
        int iSize;

        // 通知超过iTimeOut未收到回响则认为失败，单位毫秒。如果设置为0，则采用BVCSP_ServerParam.iTimeout
        int iTimeOut;

        // 通知内容
        BVCSP_NotifyMsgContent stMsgContent;

        // 填写的用户数据，用来标识每个通知。OnEvent会返回该值。
        void* pUserData;

        /* 通知送到结果回调。
        * pUserData：填写的用户数据，用来标识每个通知。OnEvent会返回该值。
        * iResult：通知送达结果。常见返回值BVCU_RESULT_S_OK: 服务器已收到通知。BVCU_RESULT_E_TIMEOUT: 发送超时，服务器未收到通知。
        */
        void(*OnEvent)(void* pUserData, BVCU_Result iResult);
    };

    typedef struct _BVCSP_Event_SessionCmd
    {
        BVCU_Result iResult;  // 错误码
        int iPercent;  // 命令完成百分比，取值范围0~100。一个命令的返回可能很长，BVCSP通过多次调用OnEvent来通知应用程序，
        // 每次OnEvent的iPercent成员会递增，100表示彻底完成，只有最后一个OnEvent才能设置为100。
        // 如果出错，iResult会被设置成错误码，iPercent设置成出错时完成的百分比。
        BVCSP_CmdMsgContent stContent; // 命令回响的数据
    }BVCSP_Event_SessionCmd;

    //    发出的命令 
    typedef struct _BVCSP_Command BVCSP_Command;
    struct _BVCSP_Command {
        // 本结构体的大小，分配者应初始化为sizeof(BVCSP_Command)
        int iSize;

        // 用户自定义数据。通常用于回调通知。应用程序/库可以用该成员来区分不同的命令
        void* pUserData;

        // 命令类型，BVCU_METHOD_* 
        int iMethod;

        // 子命令类型，BVCU_SUBMETHOD_*，决定了BVCSP_CmdMsgContent.pData类型
        int iSubMethod;

        // 命令源（系统中的网络实体)ID。SendCmd()时可以不填，库内部自动填写。
        char szSourceID[BVCU_MAX_ID_LEN + 1];

        // 命令源的登录标识。 0表示无意义。SendCmd()时不填，库内部自动填写。
        int iSourceID;

        // 系统中的网络实体目标ID。设置为空表示命令目标是当前登录的Server
        char szTargetID[BVCU_MAX_ID_LEN + 1];

        // 从0开始的目标附属设备的索引，例如PU的云台/通道/音视频IO等。设为-1表示无意义
        int iTargetIndex;

        // 命令超过iTimeOut未收到回响则认为失败，单位毫秒。如果设置为0，则采用BVCSP_ServerParam.iTimeout
        int iTimeOut;

        // 命令负载
        BVCSP_CmdMsgContent stMsgContent;

        /* 命令结果回调。
        pCommand:本命令指针。注意该指针指向的是SDK内部维护的一个BVCSP_Command浅拷贝。
        pParam: 命令结果 */
        void(*OnEvent)(BVCSP_HSession hSession, BVCSP_Command* pCommand, BVCSP_Event_SessionCmd* pParam);

        // 保留
        int iReserved[2];
    };

    /*=================================dialog========================================*/
    typedef struct _BVCSP_Event_DialogCmd BVCSP_Event_DialogCmd;
    typedef struct _BVCSP_VideoCodec
    {
        SAVCodec_ID codec; // 编码方式 SAVCODEC_ID_H264、SAVCODEC_ID_H265
        int iWidth;        // 视频图像宽度。单位像素
        int iHeight;       // 视频图像高度。单位像素
        char* pExtraData;  // sps/pps/vps等打包的h264/h265帧数据（含0x00000001头）
        int   iExtraDataSize;   // pExtraData数据长度
    } BVCSP_VideoCodec;

    typedef struct _BVCSP_AudioCodec
    {
        SAVCodec_ID codec;   // 编码方式 SAVCODEC_ID_G726、SAVCODEC_ID_AAC
        int  iBitrate;       // 波特率 32000、16000
        int  iChannelCount;  // 声道数 1
        int  iSampleRate;    // 采样率 8000
        SAV_SampleFormat eSampleFormat; // 采样精度 SAV_SAMPLE_FMT_S16
        char* pExtraData; // config字段解出的二进制数据(AAC音频编码时config有意义)。
        int   iExtraDataSize;// pExtraData数据长度
    } BVCSP_AudioCodec;

    typedef struct _BVCSP_Packet {
        // 数据类型 见 BVCSP_DATA_TYPE_*  
        unsigned short iDataType;

        // 数据包序列号 (库外不需要填写)  
        unsigned short iSeq;

        // 此帧数据前是否有丢帧， 0-不是  其它值-是  
        unsigned short bLostFrame;

        // 是否是关键帧， 0-不是  其它值-是，>= 128 表示扩展数据长度有变化  
        unsigned short bKeyFrame;

        // 数据包大小unit:byte  
        int iDataSize;

        // 数据包数据  
        void* pData;

        // 时间戳  timestamp, unit: us, 相对1970-01-01 00:00:00的微妙。
        long long iPTS;

    }BVCSP_Packet;

    typedef struct _BVCSP_DialogTarget {
        // 目标ID，例如PU ID,CU ID  
        char szID[BVCU_MAX_ID_LEN + 1];

        // 目标下属的子设备的主要号，BVCU_SUBDEV_INDEXMAJOR_*  
        int iIndexMajor;

        /* 目标下属的子设备的次要号，例如PU通道下属的流号。设置为-1表示由Server决定传输哪个流。
        bit 0～5：BVCU_ENCODERSTREAMTYPE_*
        bit 6~31：由BVCU_ENCODERSTREAMTYPE_决定的参数，默认为0
        对BVCU_ENCODERSTREAMTYPE_STORAGE/BVCU_ENCODERSTREAMTYPE_PREVIEW，设置为0，表示未使用
        对BVCU_ENCODERSTREAMTYPE_PICTURE，bit 6~9：连拍张数-1（即设置为0表示连拍1张）；bit 10~15: 抓拍间隔，单位秒，最大允许值为60秒
        */
        int iIndexMinor;

    }BVCSP_DialogTarget;

    typedef struct _BVCSP_FileTarget {
        // 目标文件相对路径+文件名, 有意义时库内部会深拷贝 
        char* pPathFileName;

        /* 回放时：回放开始时间， 相对文件开始的秒数
            文件传输时： 开始下载或上传文件起始偏移，单位：Byte。
            0表示重新上传（下载），需要删除老文件。上传请求时：-1(0xffffffff)表示续传，由接收端填写偏移值 */
        unsigned int iStartTime_iOffset;

        /* 回放时：回放结束时间， 相对文件开始的秒数， 需要大于iStartTime
            文件传输时： 文件总大小，单位：Byte，需要大于 iOffset */
        unsigned int iEndTime_iFileSize;

        // 目标文件信息描述，base64(json字符串)。见：https://mdwiki.smarteye.com/zh/smarteye/filename-format
        char* pFileInfoJson;
    }BVCSP_FileTarget;

    // Dialog。会话参数 
    typedef struct _BVCSP_DialogParam
    {
        // 本结构体的大小，分配者应初始化为sizeof(BVCSP_DialogParam) 
        int iSize;

        // 命令源 登录标识。Dialog_Open()时不填，库内部自动填写。 
        int iSourceApplierID;
        // 命令源（系统中的网络实体)ID。Dialog_Open()时可以不填，库内部自动填写。 
        char szSourceID[BVCU_MAX_ID_LEN + 1];

        // 用户自定义数据。通常用于回调通知 
        void* pUserData;

        // 登录Session 
        BVCSP_HSession hSession;

        //  会话目标。 
        BVCSP_DialogTarget stTarget;

        // 会话文件目标，回放和文件传输通道时有意义，实时通道时设置为零 
        BVCSP_FileTarget stFileTarget;

        // 会话的数据流方向, BVCU_MEDIADIR_*，文件传输通道只支持DATASEND或DATARECV其中一个  
        int iAVStreamDir;

        // 会话的流传输途径, BVCU_STREAM_PATHWAY_*  
        int iAVStreamWay;

        // 会话数据通道是否走TCP连接，0：否，1：主动TCP，2：被动TCP。over TCP时iAVStreamWay无意义  
        int bOverTCP;

        // 会话控制属性，见 BVCSP_DIALOG_OPTIONS*
        int iOptions;

        //视频通话请求返回的CallID
        char szCallID[BVCU_MAX_ID_LEN + 1];

        //打开会话密码
        char szPassword[BVCU_MAX_ID_LEN + 1];

        //  编码相关参数，当通道方向是实时流或回放流时有意义。  
        BVCSP_VideoCodec szTargetVideo;  // 目标视频编码参数
        BVCSP_AudioCodec szTargetAudio;  // 目标音频编码参数
        BVCSP_VideoCodec szMyselfVideo;  // 自己视频编码参数
        BVCSP_AudioCodec szMyselfAudio;  // 自己音频编码参数

        /*
        事件回调。函数BVCSP_GetDialogInfo可以用来获得BVCSP_DialogParam
        iEventCode:事件码，参见Dialog事件,BVCSP_EVENT_DIALOG_*
        pParam: 每个事件对应的参数，具体类型参考各个事件码的说明。如果pParam是NULL，表示无参数。
        */
        void(*OnEvent)(BVCSP_HDialog hDialog, int iEventCode, BVCSP_Event_DialogCmd* pParam);

        /* 调用者不可以在回调数据中对收到的数据进行处理，可以拷贝到自己内存中处理。
        pSource: 数据包来源信息。
        pPacket：音视频数据：收到的原始媒体数据；GPS数据：BVCU_PUCFG_GPSData；纯数据：组好帧后的数据
        返回：对纯数据无意义。对音视频数据：
        BVCU_RESULT_S_OK：pPacket被解码显示/播放。
        BVCU_RESULT_E_FAILED：pPacket不被解码显示/播放。
        */
        BVCU_Result(*afterRecv)(BVCSP_HDialog hDialog, BVCSP_Packet* pPacket);

    }BVCSP_DialogParam;

#define BVCSP_NETWORK_DVSS_MIN 1
#define BVCSP_NETWORK_DVSS_MAX 7
#define BVCSP_NETWORK_DELAY_MAX 10000
    typedef struct _BVCSP_DialogControlParam {
        // 会话命令超过iTimeOut未收到回响则认为失败，单位毫秒。如果设置为0，则采用BVCSP_SessionParam.iTimeout 
        int iTimeOut;

        // A/V数据从接收到输出，最大允许的延迟，不能超过BVCSP_NETWORK_DELAY_MAX。单位：毫秒（参考值：5000） 
        int iDelayMax;

        // A/V数据从接收到输出，最小允许的延迟，不能超过BVCSP_NETWORK_DELAY_MAX。单位：毫秒（参考值：500） 
        int iDelayMin;

        // 延迟与平滑选择。取值范围 BVCSP_NETWORK_DVSS_MIN～BVCSP_NETWORK_DVSS_MAX，越小则输出数据延迟越小，但平滑变差，越大则输出数据越平滑，但延迟变大（参考值：3）。 
        int iDelayVsSmooth;
    }BVCSP_DialogControlParam;

    //  会话信息  
    typedef struct _BVCSP_DialogInfo {
        // Dialog参数 
        BVCSP_DialogParam stParam;
        BVCSP_DialogControlParam stControlParam;

        // 会话开始时刻，从1970-01-01 00:00:00 +0000 (UTC)开始的微秒数 
        SAV_TYPE_INT64 iCreateTime;

        // 会话持续时间，单位微秒 
        SAV_TYPE_INT64 iOnlineTime;

        // 统计信息  分为音频、视频，当没有音视频时，两个值相同，表示DATA统计 

        // 收到的总包数 
        SAV_TYPE_INT64 iVideoTotalPackets;
        SAV_TYPE_INT64 iAudioTotalPackets;

        // 收到的总帧数 
        SAV_TYPE_INT64 iVideoTotalFrames;
        SAV_TYPE_INT64 iAudioTotalFrames;

        // 总字节数  // 文件传输时：和BVCSP_DialogParam.BVCSP_FileTarget中文件起始偏移、文件总大小一起，用于计算传输进度。
        unsigned int iRecvTotalDatas; // Byte
        unsigned int iSendTotalDatas; // Byte

        // 接收数据，网络部分长时间统计数据 
        int iVideoLostRateLongTerm;// 丢包(或帧)率，单位1/10000 
        int iAudioLostRateLongTerm;// 丢包(或帧)率，单位1/10000 
        int iVideoRecvFPSLongTerm;// 网络接收帧率，单位1/10000帧每秒 
        int iVideoKbpsLongTerm;// 视频数据码率，单位 Kbits/second 
        int iAudioKbpsLongTerm;// 音频数据码率，单位 Kbits/second 

        // 接收数据，网络部分短时间时间统计数据 
        int iVideoLostRateShortTerm;
        int iAudioLostRateShortTerm;
        int iVideoRecvFPSShortTerm;
        int iVideoKbpsShortTerm;
        int iAudioKbpsShortTerm;

        // 发送数据，网络部分短时间时间统计数据 
        int iVideoLostRateShortTerm_Send;
        int iAudioLostRateShortTerm_Send;
        int iVideoKbpsShortTerm_Send;
        int iAudioKbpsShortTerm_Send;
        int iGuessBandwidthSend; // 库内评估的上行带宽，单位 Kbits/second。 > 0 时有效。
    }BVCSP_DialogInfo;

    typedef struct _BVCSP_Event_DialogCmd
    {
        BVCU_Result iResult;// 错误码 
        BVCSP_DialogParam* pDialogParam; // 会话参数，可能为NULL 
    }BVCSP_Event_DialogCmd;


    /*================================Session========================================*/
    typedef struct _BVCSP_EntityInfo {
        //  设备基本信息  
        BVCU_PUCFG_DeviceInfo* pPUInfo; // 当iClientType为BVCSP_CLIENT_TYPE_PU时不能为NULL
        //  设备通道数 pChannelList的个数  
        int  iChannelCount; // 当iClientType为BVCSP_CLIENT_TYPE_PU时不该为零
        //  设备通道信息列表  
        BVCU_PUCFG_ChannelInfo* pChannelList; // 当iClientType为BVCSP_CLIENT_TYPE_PU时不该为NULL

        // 动态信息。
        int  iOnlineThrough; // 上线途径,BVCU_PU_ONLINE_THROUGH_*的某个值。
        int  iBootDuration;  // 设备开机运行了多长时间，设备上报，设备上线时上报设备已经开机多长时间， 单位 （秒）
        int  iStreamPathWay; // 数据流传输路径，见BVCU_STREAM_PATHWAY_*，值来自BVCU_PUCFG_RegisterServer.iStreamPathWay
        // 当前设备位置，GPS坐标 
        int  iLongitude; //经度，东经是正值，西经负值，单位1/10000000度。大于180度或小于-180度表示无效值
        int  iLatitude; //纬度，北纬是正值，南纬是负值，单位1/10000000度。大于180度或小于-180度表示无效值
    }BVCSP_EntityInfo;

    // Session参数 
    typedef struct _BVCSP_SessionParam {
        // 本结构体的大小，分配者应初始化为sizeof(BVCSP_ServerParam) 
        int iSize;

        // 用户自定义数据。通常用于回调通知 
        void* pUserData;

        // 用户类型。 见 BVCSP_CLIENT_TYPE_*  
        int   iClientType;

        // 一个通道能够同时被打开几路流。 iClientType为PU时有效，0：不限制个数。库内最大支持64  
        int   iMaxChannelOpenCount;

        /* 登录实体的具体信息
            注意： 这里BVCSP_EntityInfo中的指针，库内会根据iClientType类型深拷贝。 */
        BVCSP_EntityInfo stEntityInfo;

        // Server地址，只支持IP地址，如127.0.0.1，域名需上层转换为IP地址 
        char szServerAddr[BVCU_MAX_HOST_NAME_LEN + 1];

        // Server端口号 
        int  iServerPort;

        /* Client ID。库内部会根据ClientType自动添加"CU_/PU_/NRU_/VTDU_/CUSTOM_"等前缀。
            如果szClientID中包含了'_'字符，库内将不再添加前缀。
            调用者应选择一种ID分配方式，尽量使登录的ID不同。如果为空，则由库内部生成ID。
            PU设备必须填写该字段，并且该ID字符串为16进制数值字符串
            登录后调用者可以调用 BVCSP_GetSessionInfo()接口 获得Client ID */
        char szClientID[BVCU_MAX_ID_LEN + 1];

        // 应用程序名称。该名称被Server端记录到Log中 
        char szUserAgent[BVCU_MAX_NAME_LEN + 1];

        // 登录用户名 
        char szUserName[BVCU_MAX_NAME_LEN + 1];

        // 登录密码 
        char szPassword[BVCU_MAX_PASSWORD_LEN + 1];

        // Ukey ID。从UKey中获得的UKeyID。登录验证中使用。如果为空，表示没有UKey 
        char szUKeyID[BVCU_MAX_ID_LEN + 1];

        // Ukey 授权码。 从UKey中获得的验证授权码。如果为空，表示没有UKey授权码 
        char szUkeyCode[BVCU_MAX_PASSWORD_LEN + 1];

        /* 与Server之间命令通道使用的传输层协议类型，参见BVCU_PROTOTYPE_*。
            目前仅支持 UDP 和 TCP */
        int iCmdProtoType;

        // 命令超过iTimeOut未收到回响则认为失败，单位毫秒。必须>0 默认：30*1000毫秒 
        int iTimeOut;

        /* 该Session相关的事件。函数BVCSP_GetSessionInfo可以用来获得BVCSP_ServerParam参数。
            iEventCode:事件码，参见Session事件
            pParam: 每个事件对应的参数，具体类型参考各个事件码的说明 */
        void(*OnEvent)(BVCSP_HSession hSession, int iEventCode, void* pParam);

        /* 收到的Control/Query命令
            pCommand：库内部的一个BVCSP_Command对象指针。应用程序完成命令处理后，应调用pCommand->OnEvent
            返回：BVCU_RESULT_S_OK表示应用程序要处理本命令，其他值表示应用程序忽略该命令，由库决定如何处理。
        */
        BVCU_Result(*OnCommand)(BVCSP_HSession hSession, BVCSP_Command* pCommand);

        /* 收到Server的Notify通知后的回调函数
            返回：库会根据返回值构造给Server的回响包
        */
        BVCU_Result(*OnNotify)(BVCSP_HSession hSession, BVCSP_NotifyMsgContent* pData);

        /*======================被动Dialog接口，例如PU会收到Invite相关请求=====================*/
        /* 收到Dialog相关命令，被动Dialog接口。
            同步处理：应用程序可以在回调内完成命令处理，并设置pParam相关参数，尤其是：媒体信息和回调。return BVCU_RESULT_S_OK。
            异步处理：应用程序可以保存pParam相关参数，尤其是OnEvent回调（库提供的通知接口），等完成命令处理后，
            回调OnEvent（库提供的）,在OnEvent的pParam参数中传入媒体信息和事件回调（应用程序通知接口）。return BVCU_RESULT_S_PENDING。
            hDialog:库内部创建的Diglog会话句柄。
            iEventCode: 事件码，参见Dialog事件,BVCSP_EVENT_DIALOG_*
            pParam：库内部的一个BVCSP_DialogParam对象指针。
        */
        /*
        收到Invite命令。 BVCSP_EVENT_DIALOG_OPEN
            返回：BVCU_RESULT_S_OK,BVCU_RESULT_S_PENDING表示应用程序接受这个invite，其他值表示不接受。
            接受invite后，需要根据媒体方向，设置BVCSP_DialogParam.szMyselfVideo/BVCSP_DialogParam.szMyselfAudio；记住该hDialog。
            当收到pParam.OnEvent通知OPEN成功后，可以开始发送数据。
        收到ReInvite命令。BVCSP_EVENT_DIALOG_UPDATE
            返回：BVCU_RESULT_S_OK,BVCU_RESULT_S_PENDING表示应用程序接受这个Reinvite，其他值表示不接受。
            接受Reinvite后，需要处理BVCSP_DialogParam中媒体方向，编码参数可能发生变化。
        */
        BVCU_Result(*OnDialogCmd)(BVCSP_HDialog hDialog, int iEventCode, BVCSP_DialogParam* pParam);

        // 发生心跳间包隔时长，单位毫秒。 默认：4*1000毫秒 
        int iKeepaliveInterval;

        // 保留，必须初始化为0 
        int iReserved[3];
    }BVCSP_SessionParam;

    //  Session信息。一次登录创建一个Session。 
    typedef struct _BVCSP_SessionInfo {
        // 创建该Session的Param 
        BVCSP_SessionParam stParam;

        // 服务器ID 
        char szServerID[BVCU_MAX_ID_LEN + 1];

        // SmartEye域名 
        char szDomain[BVCU_MAX_SEDOMAIN_NAME_LEN + 1];

        // 服务器名 
        char szServerName[BVCU_MAX_NAME_LEN + 1];

        // Server版本号 
        char szServerVersion[BVCU_MAX_NAME_LEN + 1];

        // 本地IP 
        char szLocalIP[BVCU_MAX_HOST_NAME_LEN + 1];

        // 本地命令端口 
        int iLocalPort;

        // 是否在线 BVCSP_SESSION_STATUS_*  
        int iOnlineStatus;

        // 登录时刻，从1970-01-01 00:00:00 +0000 (UTC)开始的毫秒数 
        SAV_TYPE_INT64 iLoginTime;

        // 本次登录持续时间，单位毫秒 
        SAV_TYPE_INT64 iOnlineTime;

        // CMS分配的用户标识 
        int iApplierID;

        // 保留，必须初始化为0 
        int iReserved[3];
    }BVCSP_SessionInfo;

    // 实时音视频流加解密密钥
    typedef struct _BVCSP_EncryptKey {
        int   iSize;  // 本结构体的大小，分配者应初始化为sizeof(BVCSP_EncryptKey) 
        char  szDevID[BVCU_MAX_ID_LEN + 1]; // 设备ID号，"default"表示默认配置，即没有单独配置的设备使用默认密钥。
        char  szAlgoName[16]; // 加解密算法名称，不区分大小写。目前支持"SM4"、
        char* pKey; // 密钥数据，NULL：删除配置，使用默认配置。空数据（pKey!=NULL && iKeyLen==0）：表示不需要加解密。
        int iKeyLen; // 密钥长度，byte。
        // 保留，必须初始化为0 
        int iReserved[3];
    }BVCSP_EncryptKey;

    /*===============================================================================*/
    /**
    *初始化BVCSP库，只能在应用程序启动时调用一次。
    任何其他BVCSP库函数只有在BVCSP_Initialize成功后才可以调用。
    如果库内创建线程处理事件，那么来自库的回调为库内的线程。bAutoHandle=1。
    如果调用者自己调用BVCSP_HandleEvent()接口处理库内事件（不可多线程调用)。bAutoHandle=0.
    * @param[in] bAutoHandle: 库内是否创建线程自动处理事件。0：否，1：是。
    * @param[in] iTCPDataPort: 当数据连接是被动TCP时，监听的TCP数据端口。0：库内随机。
    指定端口号可以方便调用者自己配置防火墙,net等端口安全策略。
    * @return: 是否成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Initialize(int bAutoHandle, int iTCPDataPort);

    /**
    *库的轮询，用于库处理内部事务。
    当BVCSP_Initialize() bAutoHandle = 0 时，由调用者通过BVCSP_HandleEvent()处理库内部事务。
    注意 BVCSP库提供了线程安全，库内所有回调中不能包含任何阻塞操作。只能由某个固定的线程调用。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 正确完成。。
    *        BVCU_RESULT_E_FAILED： bAutoHandle != 0。库内部自己处理事务。
    */
    LIBBVCSP_API BVCU_Result BVCSP_HandleEvent();

    /**
    *停止使用BVCSP库
    */
    LIBBVCSP_API BVCU_Result BVCSP_Finish();

    /**
    * 设置日志回调函数
    * @param[in] callBack: 日志回调函数
    * @param[in] level: 日志等级，大于等于该等级才输出，BVCU_LOG_LEVEL_*
    */
    LIBBVCSP_API BVCU_Result BVCSP_SetLogCallback(BVCU_Log_Callback callBack, int level);

    /*=======================login/logout============================================*/
    /*
    注意：登录完成后，应用程序随时可能收到BVCSP_EVENT_SESSION_CLOSE事件回调，
    回调之后，Session被SDK摧毁，BVCSP_HSession变成无效值
    */

    /**
    *登录Server。该函数是异步的。如果登录成功，在返回前或者返回后会产生BVCSP_ServerParam.OnEvent回调。
    * @param[out] phSession: 返回登录Session
    * @param[in] pParam: Server信息
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 正确完成。结果通过OnEvent通知调用者。
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Login(BVCSP_HSession* phSession, BVCSP_SessionParam* pParam);

    /**
    *获得登录Session相关信息
    *@param[in] hSession: BVCSP_Login返回的登录Session.
    *@param[out] pInfo: BVCSP_SessionInfo
    *@return: BVCU_Result
    */
    LIBBVCSP_API BVCU_Result BVCSP_GetSessionInfo(BVCSP_HSession hSession, BVCSP_SessionInfo* pInfo);

    /**
    * 退出登录。该函数是异步的，在返回前或者返回后会产生OnEvent回调。
    * 注意：(1)该函数必须在BVCSP_Login登录成功且BVCSP_Login的OnEvent回调函数被调用之后才可以调用
    *  (2)不能在任何OnEvent/OnNotify中调用BVCSP_Logout
    * @param[in] hSession: BVCSP_Login返回的登录Session.
    * @return: 常见返回值
    *    BVCU_RESULT_S_OK: 正确完成。结果通过OnEvent通知调用者。
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Logout(BVCSP_HSession hSession);

    /*===============================command=========================================*/
    /**
    * 发送命令。该函数是异步的，命令完成后触发BVCSP_Command.OnEvent回调通知。
    * @param[in] hSession: BVCSP_Login返回的登录Session.
    * @param[in] pCommand: 需要发送的命令。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 调用正确完成。结果通过OnEvent通知调用者。
    *        BVCU_RESULT_E_NOTEXIST: 登录Session不存在，即未登录
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_SendCmd(BVCSP_HSession hSession, BVCSP_Command* pCommand);

    /**
    * 发送通知。
    * @param[in] hSession: BVCSP_Login返回的登录Session.
    * @param[in] pNotify: 需要发送的通知。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 调用正确完成。
    *        BVCU_RESULT_E_NOTEXIST: 登录Session不存在，即未登录
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_SendNotify(BVCSP_HSession hSession, BVCSP_Notify* pNotify);


    /*===================主动Dialog接口，例如CU会发送Invite相关请求====================*/
    /*
    注意：会话过程中，随时可能收到BVCSP_EVENT_DIALOG_CLOSE事件回调，
    回调之后，会话被SDK摧毁，BVCSP_HDialog变成无效值
    */

    /**
    * 创建会话。该函数是异步的。如果创建会话成功，在返回前或者返回后会产生OnEvent回调函数，
    * 事件码是BVCSP_EVENT_DIALOG_OPEN，如果事件参数的iResult是失败代码，则会话创建失败，不会再通知BVCSP_Dialog_Close
    * @param[out] phDialog: 返回会话句柄.
    * @param[in] pParam: 会话参数。
    * @param[in] pControl: 本地接收数据参数控制。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 调用正确完成。结果通过OnEvent通知调用者。
    *        BVCU_RESULT_E_UNSUPPORTED: 不支持的操作，例如在不支持对讲的通道上要求对讲
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Dialog_Open(BVCSP_HDialog* phDialog, BVCSP_DialogParam* pParam, BVCSP_DialogControlParam* pControl);

    /**
    *获得会话相关信息，也可以用于被动Dialog。
    *@param[in] hDialog: BVCSP_Dialog_Open返回的hDialog.
    *@param[out] pInfo: BVCSP_DialogInfo
    *@return: BVCU_Result
    */
    LIBBVCSP_API BVCU_Result BVCSP_GetDialogInfo(BVCSP_HDialog hDialog, BVCSP_DialogInfo* pInfo);

    /**
    * 更改已建立的会话，需要与Server通讯。对已建立的会话，允许修改pParam->iAVStreamDir。
    * 该函数是异步的。
    * 该函数可能发出异步BVCSP_EVENT_DIALOG_UPDATE事件，携带结果状态码。如果结果状态码失败，可能会出现两种情况：
    * (1)Dialog仍然处于Update之前的打开状态。例如只传音/视频=>音视频同传失败
    * (2)Dialog关闭，接着会发送BVCSP_EVENT_DIALOG_CLOSE事件。例如只传音频=>只传视频或相反；更改iMajorIndex等
    * @param[in] hDialog: BVCSP_Dialog_Open返回的hDialog.
    * @param[in] pParam: 会话参数。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK:调用正确完成。结果通过OnEvent通知调用者，事件码是BVCSP_EVENT_DIALOG_UPDATE/BVCSP_EVENT_DIALOG_CLOSE。
    *        BVCU_RESULT_E_NOTEXIST: 会话不存在
    *        BVCU_RESULT_E_BUSY:上一次的会话操作还未完成
    *        BVCU_RESULT_E_UNSUPPORTED:不支持的操作，例如在不支持对讲的通道上要求对讲 BVCU_RESULT_E_FAILED： 其他错误导致失败
    */
    LIBBVCSP_API BVCU_Result BVCSP_Dialog_Update(BVCSP_HDialog hDialog, BVCSP_DialogParam* pParam);

    /**
    * 更改会话的本地设置。此函数不需要与Server通讯。
    * @param[in] hDialog: BVCSP_Dialog_Open返回的Dialog句柄.
    * @param[in] pParam: 控制参数。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 成功
    *        BVCU_RESULT_E_NOTEXIST: 会话不存在
    *        BVCU_RESULT_E_UNSUPPORTED: 不支持的操作
    *        BVCU_RESULT_E_FAILED或其他： 其他错误导致失败
    */
    LIBBVCSP_API BVCU_Result BVCSP_Dialog_Control(BVCSP_HDialog hDialog, BVCSP_DialogControlParam* pControl);

    /**
    * 向会话通道中写数据。也可以用于被动Dialog。
    * @param[in] hDialog: BVCSP_Dialog_Open返回的Dialog句柄.
    * @param[in] pData: 写入的数据。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 成功
    *        BVCU_RESULT_E_NOTEXIST: 会话不存在
    *        BVCU_RESULT_E_UNSUPPORTED: 不支持的操作
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Dialog_Write(BVCSP_HDialog hDialog, BVCSP_Packet* pData);

    /**
    * 关闭会话。该函数是异步的，在返回前或者返回后会产生OnEvent回调函数。也可以用于被动Dialog，BVCSP_SessionParam.OnDialogCmd回调通知。
    * 注意：(1)该函数必须在BVCSP_Dialog_Open成功且BVCSP_Dialog_Open的OnEvent回调函数被调用之后才可以调用
    * (2)不能在任何OnEvent/OnNotify中调用BVCSP_Dialog_Close
    * @param[in] hDialog: BVCSP_Dialog_Open返回的Dialog句柄.
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 正确完成。结果通过OnEvent通知调用者。
    *        BVCU_RESULT_S_IGNORE:  会话不存在
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Dialog_Close(BVCSP_HDialog hDialog);

    /**
        * 设置音视频数据加解密配置。用于配置对不同设备的码流加解密。
        * 注意：设备ID为"default"表示默认配置，没有"default"配置时，默认不加解密。
        * 注意：明确设置某个设备不加解密（无论"default"如何配置），需单独设置该设备密钥为空字符串（pKey不为NULL，iKenLen==0）。
        * 注意：如果设备ID的通道已经被打开则需要等下次打开才生效，调用者可以主动Close关闭通道重新打开。
        * @param[in] pEncryptKey: 设置的加解密参数
        * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 正确完成。
    *        BVCU_RESULT_E_INVALIDARG: 无效的参数。
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Encrypt_Set(BVCSP_EncryptKey* pEncryptKey);

    /**
        * 获取音视频数据加解密设置。
        * 注意：设备ID为"default"表示默认配置。
        * @param[in,out] pEncryptKey: szDevID传入需要查询的设备ID号，pKey指向调用者分配的内存区，iKeyLen是pKey内存大小。
        * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 正确完成。只有这个情况下pEncryptKey的数据有效。
    *        BVCU_RESULT_E_INVALIDARG: 无效的参数。
    *        BVCU_RESULT_E_NOTFOUND:  没有找到配置对象。不加解密。
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Encrypt_Get(BVCSP_EncryptKey* pEncryptKey);


    /**
    * 向会话通道中写数据。也可以用于被动Dialog。
    * @param[in] hDialog: BVCSP_Dialog_Open返回的Dialog句柄.
    * @param[in] pData: 写入的数据。
    * @return: 常见返回值
    *        BVCU_RESULT_S_OK: 成功
    *        BVCU_RESULT_E_NOTEXIST: 会话不存在
    *        BVCU_RESULT_E_UNSUPPORTED: 不支持的操作
    *        BVCU_RESULT_E_BADSTATE： 库没有初始化成功。
    */
    LIBBVCSP_API BVCU_Result BVCSP_Dialog_WriteRTP(BVCSP_HDialog hDialog, BVCSP_Packet* pData);

#ifdef  __cplusplus
};
#endif

#endif
