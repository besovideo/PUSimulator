#include "bv_auth.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#endif



// 判断文件/文件夹是否存在
bool bv_is_exists(std::string path);

// 写文件(覆盖)
int bv_file_write_w(std::string filename, std::string context);

// 读文件
int bv_file_read(std::string filename, std::string& buffer);

//字符串分割函数
std::vector<std::string> string_split(std::string str, std::string pattern);


// loadAuthFile 加载解析离线信息文件
int32_t loadAuthFile(const std::string& filepath, std::string& serialNumber, std::string& ciphertext);

// storeAuthFile 保存离线信息文件
int32_t storeAuthFile(const std::string& filepath, const std::string& serialNumber, const std::string& ciphertext);


static BVRAuthParam param;       // 认证参数
static BVRAuthResult loc_authResult = AUTH_Result_FAIL; // 认证结果
static AuthInitParam loc_init_param;


// mOnAuthEvent auth result callback
static void mOnAuthEvent(
    struct BVRAuthParam* param, enum BVRAuthResult result,
    enum BVRPermission* permission, int pemsCount)
{
    printf("\n--------------- \n\
     result: %d                   \n\
     rand code: %d                \n\
     auth code: %d                \n\
     SerialNumber: %s             \n\
     issuer: %s                   \n\
     type: %s                     \n\
     id: %s                       \n\
     user_data: %d                \n\
     errstr: %s                   \n\
     ProductKey: %s               \n\
     ProductKey state: %d         \n\
     usedCount: %d                \n\
     key_result: %d               \n\
     totalCount: %d               \n\
     userlabel: %s               \n\
    ---------------\n",
    result,
    param->tagInfo.RandCode,
    param->tagInfo.AuthCode,
    param->SerialNumber,
    param->certInfo.IssueUser,
    param->termInfo.Type,
    param->termInfo.ID,
    (int)param->user_data,
    param->innerInfo.errstr,
    param->proKeyInfo.ProductKey,
    param->proKeyInfo.State,
    param->proKeyInfo.usedCount,
    param->proKeyInfo.result,
    param->proKeyInfo.totalCount,
    param->termInfo.UserLabel);

    {
        std::string serialNumber;
        std::string ciphertext;

        serialNumber.assign(param->SerialNumber);
        ciphertext.assign(param->innerInfo.ciphertext);

        storeAuthFile(loc_init_param.authFilePath, serialNumber, ciphertext);
    }

    if (AUTH_Result_App_Key_Error == result)
    {
        printf("[auth] app key error: \n\
            APP_DEV_TYPE: %s\n\
            APP_ID: %s\n\
            APP_RSA_N:%s\n\
            APP_RSA_E:\n",
            loc_init_param.appDevType.data(),
            loc_init_param.appId.data(),
            loc_init_param.appRSA_N.data(),
            loc_init_param.appRSA_E.data());
    }
    else if (AUTH_RResult_Register_Wait == result)
    {
        // 需要网页操作授权, 根据[param->tagInfo.AuthCode]查询
        printf("[auth]\n\n=============================wait auth %d=============================\n\n",
            param->tagInfo.AuthCode);
    }
    else if (AUTH_Result_OK == result)
    {
        // 认证成功了
        printf("\n\n=============================auth success=============================\n\n");
    }

    if (loc_init_param.onEvent) {
        loc_init_param.onEvent(param->tagInfo.AuthCode, result, param);
    }
}


bool isAuthSuccess()
{
    return (AUTH_Result_OK == loc_authResult || AUTH_Result_Trail_OK == loc_authResult);
}

int32_t initBVAuth(const AuthInitParam initParm)
{
    if (initParm.authFilePath.empty()) {
        printf("[auth] authFilePath is empty. %s\n", initParm.authFilePath.data());
        return -1;
    }
    if (!initParm.onInitData) {
        printf("[auth] param is NULL.\n");
        return -1;
    }

    loc_init_param = initParm;

    // 开发者信息, 需要申请
    const char* APP_DEV_TYPE = loc_init_param.appDevType.data();
    const char* APP_ID = loc_init_param.appId.data();
    const char* APP_RSA_N = loc_init_param.appRSA_N.data();
    const char* APP_RSA_E = loc_init_param.appRSA_E.data();

    std::string serialNumber;
    std::string ciphertext;
    loadAuthFile(loc_init_param.authFilePath, serialNumber, ciphertext);

    // 初始化认证信息
    {
        memset(&param, 0, sizeof(BVRAuthParam));
        param.iSize = sizeof(BVRAuthParam);

        param.OnAuthEvent = mOnAuthEvent; // 事件回调

        u_snprintf(param.appInfo.appId, sizeof(param.appInfo.appId), APP_ID);
        u_snprintf(param.SerialNumber, sizeof(param.SerialNumber) - 1, serialNumber.data());

        {
            struct BVRAuthInfo* termInfo = &param.termInfo;
            u_snprintf(termInfo->Type, sizeof(termInfo->Type) - 1, APP_DEV_TYPE);
            if (initParm.onInitData) {
                initParm.onInitData(termInfo);
            }
        }

        // 离线认证
        param.innerInfo.ciphertext = (char*)ciphertext.data();
        param.innerInfo.length = ciphertext.length();
    }

    int rc = -65535;
    char* ciphData = NULL;
    do {
        // 获取需要加密的数据
        char data[64] = { 0 };
        int data_len = sizeof(data);
        BVCU_Result result = BVCSP_GetEncryptedData(data, &data_len);
        if (BVCU_Result_FAILED(result)) {
            printf("[auth] BVCSP_GetEncryptedData() fail %d\n", result);
            rc = -2;
            break;
        }
        else {
            printf("[auth] BVCSP_GetEncryptedData() success\n");
        }

        // 加密数据
        const int CIPH_DATA_SIZE = 2048;
        ciphData = (char*)malloc(CIPH_DATA_SIZE);
        if (!ciphData) {
            printf("[auth] malloc %d fail\n", CIPH_DATA_SIZE);
            rc = -3;
            break;
        }
        memset(ciphData, 0, CIPH_DATA_SIZE);
        int outciphDataLength = CIPH_DATA_SIZE;

        // 加密
        result = BVCSP_EncryptData((char*)APP_RSA_N, (char*)APP_RSA_E, data, data_len, ciphData, &outciphDataLength);
        if (BVCU_Result_FAILED(result)) {
            printf("[auth] BVCSP_EncryptData() fail %d\n", result);
            rc = -4;
            break;
        }
        else {
            printf("[auth] BVCSP_EncryptData() success\n");
        }

        param.appInfo.ciph_data = ciphData;
        param.appInfo.ciph_len = outciphDataLength;


        // auth ...
        result = BVCSP_Auth(&param);
        if (BVCU_Result_FAILED(result)) {
            printf("[auth] BVCSP_Auth() fail %d\n", result);
            rc = -5;
            break;
        }
        else {
            printf("[auth] BVCSP_Auth() success\n");
        }

        // 
        if (ciphData) {
            free(ciphData);
        }

        // 成功了~~~
        return 0;
    } while (false);

    if (ciphData) {
        free(ciphData);
    }

    return rc;
}





const std::string PATTERN_AUTH = "[@#$%^&*()]"; // 分隔符

// loadAuthFile 加载解析离线信息文件
int32_t loadAuthFile(const std::string & filepath, std::string & serialNumber, std::string & ciphertext)
{
    if (false == bv_is_exists(filepath)) {
        return -1;
    }

    std::string fileData;
    if (bv_file_read(filepath, fileData) <= 0) {
        printf("[auth] read file fail.\n");
        return -2;
    }

    std::vector<std::string> fileLines = string_split(fileData, PATTERN_AUTH);
    if (fileLines.size() != 2) {
        printf("[auth] read file data fail. %d\n", (int32_t)fileLines.size());
        return -2;
    }

    serialNumber.assign(fileLines[0]);
    ciphertext.assign(fileLines[1]);
    return 0;
}

int32_t storeAuthFile(const std::string & filepath, const std::string & serialNumber, const std::string & ciphertext)
{
    std::string fileData;
    fileData.append(serialNumber);
    fileData.append(PATTERN_AUTH);
    fileData.append(ciphertext);

    int32_t n = bv_file_write_w(filepath, fileData);
    if (n <= 0) {
        printf("[auth] bv_file_write_w fail. %d\n", n);
        return -1;
    }

    return 0;
}


// --------------------------------------------------
// 分界线, 以下是辅助函数

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////


bool bv_is_exists(std::string path)
{
#ifdef _MSC_VER
    if (_access(path.data(), 0) != -1) {
        return true;
    }
#else
    if (access(path.data(), F_OK) != -1) {
        return true;
    }
#endif

    return false;
}

int bv_file_read(std::string filename, std::string & buffer)
{
    FILE* fp = NULL;
#ifdef _MSC_VER
    if (0 != fopen_s(&fp, filename.data(), "r")) {
        printf("open file failed!\n");
        return -1;
    }
#else
    fp = fopen(filename.data(), "r");
    if (NULL == fp) {
        printf("open file failed!\n");
        return -1;
    }
#endif

    fseek(fp, 0, SEEK_END); //把指针移动到文件的结尾 ，获取文件长度

    int len = ftell(fp); //获取文件长度
    char* pBuf = (char*)malloc(len + 1); //定义数组长度

    rewind(fp); //把指针移动到文件开头 因为我们一开始把指针移动到结尾，如果不移动回来 会出错
    len = fread(pBuf, 1, len, fp); //读文件
    pBuf[len] = 0; //把读到的文件最后一位 写为0 要不然系统会一直寻找到0后才结束

    fclose(fp); // 关闭文件

    buffer.assign(pBuf, len);

    free(pBuf);

    return len;
}

// 写文件(覆盖)
int bv_file_write_w(std::string filename, std::string context)
{
    int size = -1;
    FILE* fp = NULL;
#ifdef _MSC_VER
    if (0 != fopen_s(&fp, filename.data(), "w+")) {
        printf("open file failed!\n");
        return -3;
    }
#else
    fp = fopen(filename.data(), "w+");
    if (NULL == fp) {
        printf("open file failed!\n");
        return -3;
    }
#endif

    if (NULL != fp) {
        size = fwrite(context.data(), context.size(), 1, fp);
        fclose(fp);
    }

    return size;
}



//字符串分割函数
std::vector<std::string> string_split(std::string str, std::string pattern)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern;//扩展字符串以方便操作
    int size = str.size();

    for (int i = 0; i < size; i++)
    {
        pos = str.find(pattern, i);
        if (pos < size)
        {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }

    return result;
}

