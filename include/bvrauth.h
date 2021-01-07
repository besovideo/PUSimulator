#ifndef __LIB_AAA_H__
#define __LIB_AAA_H__

#include "bvrconst.h"

#ifdef __cplusplus
extern "C" {
#endif


// 终端认证信息
typedef struct BVRAuthInfo {
    char Type[64]; // PU、MPU、CU、CMS、VTDU、NRU ...
    char ID[128]; // PU_ID、CU_ID、NRU_ID ...
    char Name[128]; // PU NAME、CU NAME

    char ModelNumber[128]; // 验证型号

    char MAC[64]; // 网卡地址
    char IMEI[64]; // 安卓设备(非安卓设备必须置空)
    char HardwareProvider[128]; // 硬件提供商 UTF-8, 内部覆盖
    char HardwareSN[1024]; // 硬件序列号
    char HardwareVersion[64]; // 硬件版本号

    char SoftwareProvider[128]; // 软件提供商 UTF-8
    char SoftwareVersion[64]; // 软件版本号

    char OSType[64]; // 系统类型 window、linux、android、ios
    char OSVersion[32]; // 系统版本
    char OSID[64]; // 系统ID
    char CPU[64]; // 处理器信息

    char UserLabel[64]; // 用户标签, 后台管理用户通过用户标签, 访问未授权终端

    char DeviceModelNumber[64]; // 设备型号
    char Desc[256]; // 其他信息 UTF-8
    char KeyID[256]; // 设备唯一标识, 必须保证唯一, 可以置空

    char ProductKey[64]; // 产品密钥
}BVRAuthInfo;

// 证书信息
typedef struct BVRCertInfo {
    char Type[64]; // 'Unregistered' 、'Registered'
    char State[64]; // 'Normal' 、'Revoke'
    char IssueUser[64]; // 颁发者
    char IssueData[64]; // 颁发日期
    char dataBegin[64]; // 证书有效日期 起始
    char dataEnd[64]; // 证书有效日期 结束
}BVRCertInfo;

typedef struct BVRTagInfo {
    int RandCode; // 随机码, 本地标识
    int AuthCode; // 认证码, 远程标识
    // int AuthCodeHigh32; // 高32位
}BVRTagInfo;

// 通知消息
typedef struct BVRTPMessage {
    int iSize;
    int Seq; // 消息序列号, 内部标识
    char SessionID[64]; // 连接ID
    char* Data; // 负载数据
    int Length; // 负载长度
}BVRTPMessage;

// 库内部信息, 关闭时需要保存
typedef struct BVRInnerInfo {
    char* ciphertext;
    int length;

    char* errstr; // 错误字符串
    int netState; // 网络状态: 1 连接 2 断开
} BVRInnerInfo;

// 开发者的app信息
typedef struct BVRAppInfo {
    char appId[64]; // app id

    char* ciph_data; // Base64(RSA(要加密的数据)): 使用app的公钥加密 从bvr_get_encrypted_data()获得的 数据, 然后base64加密
    int ciph_len;
} BVRAppInfo;

// 认证参数
typedef struct BVRAuthParam {
    int iSize;
    char SerialNumber[SERIAL_NUMBER_SIZE]; // 输入信息, 没有认证终端, 置空
    struct BVRAuthInfo termInfo; // 输入信息
    struct BVRCertInfo certInfo; // 只读信息
    struct BVRTagInfo tagInfo; // 只读信息
    struct BVRInnerInfo innerInfo; // 内部信息, OnAuthEvent回调时保存
    struct BVRAppInfo appInfo; // app信息
    struct BVRProductKeyInfo proKeyInfo; // 产品密钥信息, 只读信息

    /** 认证结果通知,
        回调时, 保存param.innerInfo
        成功时, 需要保存param.SerialNumber
        permission: 权限列表, BVRPermission数组 */
    void(*OnAuthEvent)(struct BVRAuthParam* param, enum BVRAuthResult result, enum BVRPermission* permission, int pemsCount);
    /** 收到数据 */
    void(*OnMessage)(struct BVRAuthParam* param, struct BVRTPMessage* msg);

    void* user_data;
    int iReserved[4];
}BVRAuthParam;


typedef struct BVRContext BVRContext;

// 初始化
int bvr_init(BVRContext** ctx);
// 释放库
int bvr_deinit(BVRContext* ctx);

// 获取需要加密的数据
int bvr_get_encrypted_data(BVRContext* ctx, char* data, int* len);

// 获取共享数据(加密的port+nonce)
int bvr_get_share_data(BVRContext* ctx, char* data, int* len);

// 设置服务器地址、端口号
int bvr_set_server(BVRContext* ctx, char* host, int port);

// loop
int bvr_loop(BVRContext* ctx);
int bvr_loop_handle(BVRContext* ctx, int iTickCount);

// 传入认证信息, info->tagInfo.RandCode带回标识, 需要更新认证信息BVRAuthParam, 传入时需要设置info->tagInfo.RandCode
int bvr_auth(BVRContext* ctx, struct BVRAuthParam* info);

// 申请试用, randCode: bvr_auth()函数带回的BVRAuthParam.tagInfo.RandCode
// 推荐: 在bvr_auth -> OnAuthEvent申请试用
int bvr_trial(BVRContext* ctx, int authCode);

// 模块证书
int bvr_auth_module(BVRContext* ctx, int authCode, char* module_key);
// 获取支持的模块列表(返回json数据) 参数data==NULL, len传出负载大小
int bvr_get_module_list(BVRContext* ctx, int authCode, char *data, int* len);

// 获取认证信息, 通过info->tagInfo.RandCode比对查找
int bvr_get_info(BVRContext* ctx, struct BVRAuthParam* info);

// 获取认证结果, 第一个认证结果
BVRAuthResult bvr_get_result(BVRContext* ctx);
// 获取认证结果
BVRAuthResult bvr_get_result_2(BVRContext* ctx, int randCode);

// 回复消息
BVRAuthResult bvr_send_message(BVRContext* ctx, struct BVRAuthParam* info, struct BVRTPMessage* msg);

// 发送命令
BVRAuthResult bvr_send_commond(BVRContext* ctx, struct BVRAuthParam* info, struct BVRCommond* cmd);

// 加密 base64(rsa(data)), 用于测试, 内置的rsa加密, 不推荐使用
int bvr_data_encrypt(char* n, char* e, char *input, int ilen, char *output, int* olen);

int bvr_log(char* data);

// 是否重连, 1重连; 2不重连; 其它
int bvr_set_config_reconnect(BVRContext* ctx, int isReConnect);

#ifdef __cplusplus
}
#endif

#endif // !__LIB_AAA_H__
