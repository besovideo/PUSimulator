#ifndef __LIB_AAA_CONST_H__
#define __LIB_AAA_CONST_H__

#include "bvrconfig.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SERIAL_NUMBER_SIZE 128

typedef enum BVRPermission {
    Permit_UnDefined = 0,
    Permit_Video_Preview = 10,
    Permit_Record,
    Permit_Map
}BVRPermission;


typedef enum BVRAuthResult {
    AUTH_Result_FAIL = 0,
    AUTH_Result_Token_Error = 1,
    AUTH_Result_Database_Error = 2,
    AUTH_Result_Internal_Error = 3,
    AUTH_Result_No_Login = 4,
    AUTH_Result_DeSerialize_Fail = 5,
    AUTH_Result_Un_Support = 6,
    AUTH_Result_DisConnect = 7,
    AUTH_Result_Connect_Fail = 8,
    AUTH_Result_Server_Busy = 9,
    AUTH_Result_Parse_Error = 10,
    AUTH_Result_Decrypt_Fail = 11,
    AUTH_Result_Timecout = 12,

    AUTH_Result_No_AppInfo = 12, // 没有APP开发者信息
    AUTH_Result_Not_Found_AppInfo = 13,
    AUTH_Result_App_Key_Error = 14,
    AUTH_Result_App_Status_Error = 15, // 状态错误

    AUTH_Result_Not_Found = 16,
    AUTH_Result_No_Trial_Quota = 17, //没有试用名额
    AUTH_Result_Trial_Expire = 18, //试用期满
    AUTH_Result_Trial_Timeout = 19, //试用超时
    AUTH_Result_Trial_KickOut = 20, //踢下线
    AUTH_Result_Trail_Deny = 21,    // 拒绝试用, 管理员不允许改终端使用
    AUTH_Result_Type_Error = 22,    // 类型错误
    AU_Result_No_WriteBack_Times = 23, // 没有认证写回次数(即: 根据硬件信息写回授权信息的次数用完了)

    AUTH_RResult_Register_FAIL = 51,
    AUTH_RResult_Register_Wait = 52,
    AUTH_RResult_Register_Timeout = 53,
    AUTH_RResult_Register_Have_Registed = 54, // 已经注册过
    AUTH_RResult_Register_No_Cert = 55, // 没有提供证书
    AUTH_RResult_Register_Internal_Error = 56, // 注册, 内部错误
    AUTH_RResult_Register_Not_Original_Cert = 57, // 不是原始的证书

    AUTH_Result_Have_Login = 101, // 已经有终端使用该信息登录
    AUTH_Result_Cert_State_Error = 102, // 证书状态错误
    AUTH_Result_Cert_TimeOut_Error = 103, // 证书过期
    AUTH_Result_No_SerialNumber = 104, // 没有设备的信息
    AU_Result_Login_FAIL = 105,
    AU_Result_Revoke_Authorization = 106, //撤销授权
    AU_Result_Pause_Authorization = 107, //暂停授权, 后期可能恢复
    AU_Result_Login_Elsewhere = 108, // 在其它处登录
    AU_Result_Cert_UseUp = 109, // 证书授权次数用完
    AU_Result_Error_Key_Id = 110, // 错误的keyid(key_id在默认列表中)
    AU_Result_Key_Id_NotMatch = 111, // login.key_id和db.key_id不匹配

    AUTh_Result_KeepAlive_No_Token = 151, // 保活时服务器没有token
    AUTh_Result_KeepAlive_No_Session = 152,  //连接不存在

    AUTH_Result_Key_State_Error = 250, // 产品密钥状态错误
    AUTH_Result_Key_Expire = 251, // 产品密钥过期
    AUTH_Result_Key_UseUp = 252, // 产品密钥授权次数用完

    AUTH_Result_Local_Status = 260, // 内网认证检查状态错误
    AUTH_Result_Local_Expire = 261, // 内网认证过期
    AUTH_Result_Local_SN = 262, // 内网认证SN不匹配
    AUTH_Result_Local_Offline_Trial = 263, // 内网认证不允许离线试用
    AUTH_Result_Local_InnerDataDecode = 264, // 内网认证内部数据解码错误
    AUTH_Result_Local_InnerDataFormat = 265, // 内网认证内部数据解析错误

    AUTH_Result_OK = 1024,
    AUTH_Result_Trail_OK = 1025  // 试用
}BVRAuthResult;



// 命令类型
typedef enum BVRTPCmdType {
    TPCmdType_UnDefined = 0,

    TPCmdType_Put_CMS_INFO = 21, // 上传cms信息
    TPCmdType_Get_CMS_INFO = 22, // 获取cms信息, 主要是设备最大上线数, 使用黑白名单

    TPCmdType_Get_CMS_WH_List = 23, // 获取黑白名单

    TPCmdType_Put_PU_Info = 30, // 上传PU信息
    TPCmdType_Put_CMS_PU_Info_List = 31, // 上传CMS上PU信息列表

    TPCmdType_Set_AuthCodeAndKey = 40, // 认证码+聚合秘钥, 设备授权+模块授权
}BVRTPCmdType;

// cms信息
typedef struct BVRTPCmdCMSInfo {
    char cms_info_id[64];
    char cms_info_version[64];
    char cms_info_name[64];
    char cms_info_domain[64];
    int cmd_port_pu;
    int cmd_port_cu;

    char db_host[64];
    int db_port;
    char db_username[64];
    char db_password[64];

    int max_pu_count; // 允许上线的最大设备数, 只读
    int max_online_pu_count; // 允许同时在线的最大设备数, 只读
    char use_list_type[64]; // 黑白名单类型, 只读
    char cms_info_agent[64];
}BVRTPCmdCMSInfo;

// cms黑白名单项
typedef struct BVRTPCmdCMSWHItem {
    char list_item_type[64];
    char field_type[64];
    char field_value[64];
    long long item_id;
}BVRTPCmdCMSWHItem;

// cms黑白名单
typedef struct BVRTPCmdCMSWHList {
    int page;
    int page_size;
    int total_size;

    BVRTPCmdCMSWHItem* itemList;
    int itemCount;
}BVRTPCmdCMSWHList;

// pu信息
typedef struct BVRTPCmdPUInfo {
    char pu_id[64];
    char pu_name[64];
    union {
        char cms[64];          // 设备端: 上线的cms, 格式: "61.191.27.18:9702"
        char serialnumber[64]; //   CMS: 终端(如PU)的序列号
    };
    int  event; // 事件 1: 登录 2: 退出
    int  result; // 事件结果
    char* ext; // 其它信息, 要求: json字符串(utf8). 如: "{\"bvcsp_version\": \"2.3.5\", \"apk_version\": \"5.30.2\"}"
}BVRTPCmdPUInfo;

// cms的pu信息
typedef struct BVRTPCmdCMS_PUInfo {
    BVRTPCmdPUInfo* puList;
    int puCount;
}BVRTPCmdCMS_PUInfo;

// 产品密钥信息
typedef struct BVRProductKeyInfo {
    char ProductKey[64]; // 产品密钥
    int  State; // 0: '正常'; 1: 停止; 2: 删除; 3: 不存在
    int  usedCount; // 已使用数
    int  totalCount; // 总数
    char keyName[32];  // 名称
    char Type[32];     // 密钥颁发的终端类型
    char IssueUser[32]; // 颁发者
    char dateEnd[64]; // 有效日期

    enum BVRAuthResult result; // 产品密钥有无错误(过期, 没有授权次数, 使用的证书被吊销等)
} BVRProductKeyInfo;

typedef struct BVAuthFileProperty {
    char SerialNumber[SERIAL_NUMBER_SIZE];
    long long createTime; // 创建时间，UTC时间，精确到秒
    int expire; // 有效时间，精确到秒
} BVAuthFileProperty;

typedef struct BVAuthAuthCodeAndKey {
    long long auth_code; // 认证码
    char key[SERIAL_NUMBER_SIZE]; // 聚合秘钥/产品秘钥/模块秘钥. 目前仅支持聚合秘钥

    int resp_code; // 回复错误码
    char* resp_msg; // 回复错误消息
}BVAuthAuthCodeAndKey;

// 命令回复
typedef struct BVRCommondRes {
    enum BVRTPCmdType iMethod;
    enum BVRAuthResult iResult;

    // 根据iMethod确定数据
    union {
        BVRTPCmdCMSInfo* cmsInfo; // 获取CMS信息
        BVRTPCmdCMSWHList* whList; // 获取黑白名单
        BVAuthAuthCodeAndKey* authCodeAndKey; // 自动授权
        // ... 
    } data;
}BVRCommondRes;

// 命令
typedef struct BVRCommond {
    int iSize;
    enum BVRTPCmdType iMethod; // 命令类型
    char TargetID[64]; // 目标ID, 当前未使用, 置空

    // 根据iMethod确定数据, 添加变量全部为指针
    union {
        BVRTPCmdCMSInfo* cmsInfo; // 上传CMS信息
        BVRTPCmdCMSWHList* whList; // 获取黑白名单
        BVRTPCmdPUInfo* puInfo; // 上传PU信息
        BVRTPCmdCMS_PUInfo* cmsPuInfoList; // 上传CMS的PU信息列表
        BVAuthAuthCodeAndKey* authCodeAndKey; // 自动授权
        // ... 
    } data;

    void(*CmdCallBack)(struct BVRCommond* pThis, struct BVRCommondRes* pResponse);

    void* userData; // 用户数据
    int iReserved[4];
}BVRCommond;


#ifdef __cplusplus
}
#endif

#endif // !__LIB_AAA_METHOD_H__





#if COMMOND_DEMO
void demoTpCmdCallBack(struct BVRCommond* pThis, struct BVRCommondRes* pResponse)
{
    if (pResponse->iResult == AUTH_Result_OK)
    {
        if (pResponse->iMethod == TPCmdType_Get_CMS_WH_List) {
            BVRTPCmdCMSWHList* list = pResponse->data.whList;
        }
    }
}

int demo_get_cms_wh_list(struct BVRContext* ctx, struct BVRAuthParam* info, struct BVRTPCmdCMSWHList* data)
{
    BVRCommond cmd;
    memset(&cmd, 0, sizeof(BVRCommond));
    cmd.iSize = sizeof(BVRCommond);
    cmd.iMethod = TPCmdType_Get_CMS_WH_List;
    cmd.data.whList = data;
    cmd.CmdCallBack = demoTpCmdCallBack;

    return bvr_send_commond(ctx, info, &cmd);
}
#endif

