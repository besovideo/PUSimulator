#ifndef __BVAUTH_H__
#define __BVAUTH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_AUTH 1

#include "bvrauth.h"
#if ENABLE_AUTH

/**
    认证终端
    param->tagInfo.RandCode带回本地标识,
    更新认证信息时, 需要传入RandCode
*/
LIBBVCSP_API BVCU_Result BVCSP_Auth(BVRAuthParam* param);

/**
    设置认证服务器地址(当前版本不支持)
*/
LIBBVCSP_API BVCU_Result BVCSP_SetServer(char* host, int port);

/**
    获取认证结果信息
*/
LIBBVCSP_API BVCU_Result BVCSP_GetAuthInfo(BVRAuthParam* info);

/**
    获取认证结果
    authResult:
        AUTH_Result_OK 认证成功
*/
BVCU_Result BVCSP_GetAuthResult(BVRAuthResult* authResult);

/**
    获取需要加密的数据, 使用开发者的公钥进行加密. 初始化param->appInfo
*/
LIBBVCSP_API BVCU_Result BVCSP_GetEncryptedData(char* data, int* len);

/**
    申请试用(功能未开放, 目前仅支持本公司产品)
*/
LIBBVCSP_API BVCU_Result BVCSP_Trial(int authCode);

/**
    是否重连, 1重连; 2不重连; 其它
*/
LIBBVCSP_API BVCU_Result BVCSP_SetReConnect(int isReConnect);

/**
    是否连接 1连接, 2不连接(一次都不连接)
*/
LIBBVCSP_API BVCU_Result BVCSP_SeConnect(int isConnect);

/**
    加密BVCSP_GetEncryptedData()获得的数据,
    (用于测试, 不推荐使用)
*/
LIBBVCSP_API BVCU_Result BVCSP_EncryptData(char* n, char* e, char *input, int ilen, char *output, int* olen);

/**
    获取模块共享数据(Base64数据)
*/ 
LIBBVCSP_API BVCU_Result BVCSP_GetModuleShareData(char *output, int* olen);

/** 认证模块
    module_key: 模块证书
 */
LIBBVCSP_API BVCU_Result BVCSP_AuthModule(int authCode, char* module_key);

/** 获取支持的模块列表(Json)  参数data==NULL, len传出负载大小
 */
LIBBVCSP_API BVCU_Result BVCSP_GetModuleList(int authCode, char *data, int* len);

/**
    对BVRAuthParam.OnMessage()的回复
    注意 BVRTPMessage.SessionID、BVRTPMessage.Seq要一致
*/
LIBBVCSP_API BVCU_Result BVCSP_AuthSendMessage(BVRAuthParam* info, struct BVRTPMessage* msg);

/**
    执行命令, 如上传PU信息, CMS获取黑白名单等
*/
LIBBVCSP_API BVCU_Result BVCSP_AuthSendCommond(BVRAuthParam* info, struct BVRCommond* cmd);


/**
    测试-测试-测试
*/
LIBBVCSP_API BVCU_Result BVCSP_Test(char* inputData, char* outputData);

#endif

#ifdef __cplusplus
}
#endif

#endif
