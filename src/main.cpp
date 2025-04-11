#include <stdio.h>
#ifndef __linux__
#include <process.h>
#else
#include <pthread.h>
#endif
#include <string>
#include "utils.h"
#include "BVCSP.h"
#include "main.h"


#include "config.h"
#include "pusession.h"
#include "file.h"

#ifdef _MSC_VER
#include <windows.h>
#ifdef _WIN64
#pragma comment(lib, "BVCSP_x64.lib")
#elif _WIN32
#pragma comment(lib, "BVCSP.lib")
#endif // _WIN32
#endif // _MSC_VER

#define UPLOAD_FILE_PATH_NAME "./uploadfile.mp4"
#define FILE_TRANS_INTERVAL 5 // 秒

static CPUSession* pSession[1024] = { 0,0,0,0 };  // 全局Session
static bool needOnline = false; // 是否需要在线，调用login和logout时修改
static int concurrency = 100; // 并发请求数

static bool fileTransContinue = false;    // 文件传输结束后继续传输
static CMyFileTrans fileTransManager; // 文件传输
static BVCU_File_HTransfer hFileTrans[1024] = { 0,0,0,0 };  // 全局文件传输状态
static int hFileStatus[1024] = { 0,0,0,0 };  // 全局文件传输状态
static time_t iFileTimes[1024] = { 0,0,0,0 }; // 全局文件传输结束时间，用于延迟继续传输

// BVCSP日志回调
void Log_Callback(int level, const char* log)
{
    printf("[BVCSP LOG] %s\n", log);
}

void  File_OnEvent(BVCU_File_HTransfer hTransfer, void* pUserData, int iEventCode, BVCU_Result iResult) {
    printf(">>>>>>>>>>>>>  file event: %d %d\n", iEventCode, iResult);
    for (int i = 0; i < sizeof(hFileTrans) / sizeof(hFileTrans[0]); i++) {
        if (hFileTrans[i] == hTransfer) {
            hFileStatus[i] = iResult;
            iFileTimes[i] = time(NULL);
        }
    }
}

static bool bRun = true;
static char cmdType[2] = { 0,0 };
static time_t lastPrintTime = 0;
// 用来定时 读取音视频、GPS、串口数据，并发送。
#ifndef __linux__
unsigned __stdcall Wall_App(void*)
#else
void *Wall_App(void*)
#endif
{
    fileTransManager.m_OnEvent = File_OnEvent;

    // 初始化库
    BVCSP_SetLogCallback(Log_Callback, BVCU_LOG_LEVEL_INFO);
    BVCSP_Initialize(0, 0);
    BVCSP_HandleEvent(); // 通知库内部使用当前线程作为事务线程。

    // 开始认证
    Auth();

    time_t exitTime = 0; // 延迟退出
    while (true)
    {
        if (!bRun) { // 退出程序，等待登录退出。
            if (exitTime == 0) {
                Logout();
                exitTime = time(NULL);
            }
        }
        HandleEvent();
        BVCSP_HandleEvent();

        if (!bRun && (time(NULL) - exitTime) > 5) // 已全部退出
            break;

        if (cmdType[0] == 'i')
        {
            Login(false);
        }
        else if (cmdType[0] == 'o')
        {
            Logout();
        }
        else if (cmdType[0] == 'a')
        {
            SendAlarm();
        }
        else if (cmdType[0] == 'c')
        {
            SendCommand();
        }
        else if (cmdType[0] == 'f')
        {
            if (cmdType[1] == 'f')
                UploadFile1(true);
            else
                UploadFile1(false);
        }
        else if (cmdType[0] == 'F')
        {
            UploadFile2();
        }
        cmdType[0] = 0;
        cmdType[1] = 0;
    }

    Logout();
    BVCSP_Finish();
    cmdType[0] = '\n';
#ifndef __linux__
    return 0;
#endif
}

int main()
{
#ifndef __linux__
    _beginthreadex(NULL, 0, Wall_App, NULL, 0, NULL);
#else
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, Wall_App, NULL);
#endif

    while (1)
    {
        char cmd[128] = { 0 };
        printf("            e :exit  i :login  o :logout  a :alarm c :command f :多文件并发 F :单文件并发 ff :持续多文件 \n");
        gets_s(cmd);
        //printf("============= %s ==============\n", cmd);
        cmdType[0] = cmd[0];
        cmdType[1] = cmd[1];
        if (cmdType[0] == 'e')
            break;

        lastPrintTime = 0;
    }
    bRun = false;
    for (int i = 0; i < 1000; i++) {
        if (cmdType[0] == '\n')
            break;
        Sleep(100); // 等待Wall_App线程退出。
    }
    return 0;
}

// 登录服务器。从配置文件中读取设备信息，和服务器信息；请提前设置好。
int Login(bool autoOption)
{
    needOnline = true;
    if (autoOption && pSession[0] != 0)
    {  // 自动操作不能重复登录。
        return 0;
    }
    if (pSession[0] == 0)
    {
        PUConfig puconfig;
        LoadConfig(&puconfig);
        concurrency = puconfig.Concurrency;
        fileTransManager.SetBandwidthLimit(puconfig.bandwidth);
        int startID = 1;
        int usersID = 1;
        char puid[32];
        char puName[32];
        char userID[32];
        if (sscanf_s(puconfig.ID, "PU_%X", &startID) != 1)
            startID = 0x55AA0000;
        for (int i = 0; i < puconfig.PUCount && i < sizeof(pSession) / sizeof(pSession[0]); i++)
        {
            sprintf_s(puid, "PU_%X", startID++);
            sprintf_s(userID, "test_%d", usersID++);
            pSession[i] = new CPUSession(puid, puconfig.relogin);
            if (pSession[i])
            {
                if (puconfig.bUA != 0) {
                    pSession[i]->SetUser(userID, "123");
                    puconfig.protoType = 1; // UA 只支持TCP
                }
                // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
                // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
                // ==================  设置设备信息，需要开发者根据自己设备情况设置 =============== 
                sprintf_s(puName, "%s-%d", puconfig.Name, i + 1);
                pSession[i]->SetName(puName);
                pSession[i]->SetServer(puconfig.serverIP, puconfig.serverPort, puconfig.protoType, 60 * 1000, 4 * 1000);
                pSession[i]->SetDevicePosition(puconfig.lat, puconfig.lng);
                pSession[i]->RegisterChannel();
                pSession[i]->SetFileManager(&fileTransManager);
            }
        }
    }
    if (pSession[0] == 0)
        return -1;
    // 上线服务器，lat,lng 应该改为当前设备定位位置。

    return 0;
}

// 退出登录
int Logout()
{
    needOnline = false;
    return 0;
}

// 发送报警信息
int SendAlarm()
{
    bool bFind = false;
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0)
        {
            pSession[i]->SendAlarm(
                BVCU_EVENT_TYPE_ALERTIN,
                0,
                0,
                0,
                "test"
            );
            bFind = true;
        }
    }
    if (bFind)
        return 0;
    return -1;
}

// 发送命令
int SendCommand()
{
    bool bFind = false;
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0)
        {
            pSession[i]->SendCommand(BVCU_METHOD_QUERY, BVCU_SUBMETHOD_CMS_HTTPAPI, NULL, NULL, NULL);
            bFind = true;
        }
    }
    if (bFind)
        return 0;
    return -1;
}

bool UploadFile(int index) {
    if (pSession[index] != 0) {
        if (pSession[index]->BOnline()) {
            time_t tnow = time(NULL);
            tm* pnow = localtime(&tnow);
            char nyr[32] = { 0 };
            char sfm[32] = { 0 };
            char remotefile[512] = { 0 };
            strftime(nyr, sizeof(nyr), "%Y%m%d", pnow);
            strftime(sfm, sizeof(sfm), "%H%M%S", pnow);

            BVCU_PUCFG_DeviceInfo* info = pSession[index]->GetDeviceInfo();
            sprintf_s(remotefile, "/%s/Video/%s/%s_00_%s_%s_LA0800.mp4", info->szID, nyr, info->szID, nyr, sfm);
            BVCU_File_HTransfer hTransfer;
            int result = pSession[index]->UploadFile(&hTransfer, UPLOAD_FILE_PATH_NAME, remotefile);
            printf("upload >>>>>>>>>>> file %d %s -> %s\n", result, UPLOAD_FILE_PATH_NAME, remotefile);
            hFileTrans[index] = hTransfer;
            hFileStatus[index] = result;
            iFileTimes[index] = time(NULL);
            return true;
        }
    }
    return false;
}
// 上传文件，测试多个设备并发上传不同文件
void UploadFile1(bool bContinue) {
    if (bContinue) {
        if (fileTransContinue) {
            fileTransContinue = false;
            return;
        }
        fileTransContinue = true;
    }
    else
        fileTransContinue = false;

    memset(hFileTrans, 0x00, sizeof(hFileTrans));

    int uploadcount = 0;
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]) && uploadcount <= concurrency; i++)
    {
        if (UploadFile(i))
            uploadcount++;
    }
}

// 上传文件，测试同一个设备并发上传同一个文件
void UploadFile2() {
    fileTransContinue = false;

    memset(hFileTrans, 0x00, sizeof(hFileTrans));
    time_t tnow = time(NULL);
    tm* pnow = localtime(&tnow);
    char nyr[32] = { 0 };
    strftime(nyr, sizeof(nyr), "%Y%m%d", pnow);

    if (pSession[0] != 0 && pSession[0]->BOnline()) {
        BVCU_PUCFG_DeviceInfo* info = pSession[0]->GetDeviceInfo();

        char remotefile[512] = { 0 };
        sprintf_s(remotefile, "/%s/Video/%s/%s_00_%s_%s_LA0800_#800kV -+=.$^&()!@~`';500kV#2500kV500kV.mp4", info->szID, nyr, info->szID, nyr, "111111");

        for (int i = 0; i < concurrency; i++)
        {
            BVCU_File_HTransfer hTransfer;
            int result = pSession[0]->UploadFile(&hTransfer, UPLOAD_FILE_PATH_NAME, remotefile);
            printf("upload >>>>>>>>>>> file %d %s -> %s\n", result, UPLOAD_FILE_PATH_NAME, remotefile);
            hFileTrans[i] = hTransfer;
            hFileStatus[i] = result;
            iFileTimes[i] = time(NULL);
        }
    }
}

// 
int HandleEvent()
{
    time_t nowTime = time(NULL);
    int totalCount = 0;
    int onlineCount = 0;
    int loginCount = 0;
    int logoutCount = 0;
    int offline = 0;
    for (int i = 0; i < sizeof(pSession) / sizeof(pSession[0]); i++)
    {
        if (pSession[i] != 0) {
            pSession[i]->HandleEvent(nowTime);
            totalCount++;

            if (pSession[i]->BLogining()) {
                loginCount++;
            }
            else if (pSession[i]->BLogouting()) {
                logoutCount++;
            }
            else if (pSession[i]->BOnline()) {
                onlineCount++;
                if (!needOnline && logoutCount == 0 && onlineCount <= concurrency) {
                    pSession[i]->Logout();
                }
            }
            else if (pSession[i]->BOffline()) {
                offline++;
                if (needOnline && offline <= concurrency && loginCount == 0 && pSession[i]->BFirst()) {
                    pSession[i]->Login(BVCU_PU_ONLINE_THROUGH_ETHERNET, 200 * 10000000, 200 * 10000000);
                }
            }
        }
    }
    fileTransManager.HandleEvent();

    int ftrans = 0, ffail = 0, fok = 0, fall = 0;
    for (int i = 0; i < sizeof(hFileTrans) / sizeof(hFileTrans[0]); i++) {
        if (hFileTrans[i] != NULL) {
            fall++;
            CFileTransfer* filetrans = fileTransManager.FindFileTransfer(hFileTrans[i]);
            if (filetrans != NULL) {
                ftrans++;
            }
            else {
                if (hFileStatus[i] != 0) {
                    ffail++;
                }
                else {
                    fok++;
                }
                if ((nowTime - iFileTimes[i]) > FILE_TRANS_INTERVAL && fileTransContinue) {
                    UploadFile(i);
                }
            }
        }
    }

    if ((nowTime - lastPrintTime) > 4)
    {
        lastPrintTime = nowTime;
        printf("  ==============  offline / logout / login / online / total    %d / %d / %d / %d / %d \n", offline, logoutCount, loginCount, onlineCount, totalCount);
        printf("  ==============  trans / failed / ok / total    %d / %d / %d / %d \n", ftrans, ffail, fok, fall);
    }
    return onlineCount + loginCount + logoutCount;
}
