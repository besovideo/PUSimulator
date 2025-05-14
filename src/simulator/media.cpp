#include <string>
#include <cstdlib>
#include <stdio.h>
#include "../utils/utils.h"
#include "media.h"
#include "config.h"
#include "../function/bv.h"

#ifdef _MSC_VER
#include <Windows.h>
#include <process.h>
#endif

int m_interval;     // 上报数据时间间隔，毫秒。
int m_lasttime;     // 上次上报时间。毫秒。GetTickCount();
int m_lastAdjtime;  // 上次调整码率时间。毫秒。GetTickCount();
int m_lastPlitime;  // 上次产生关键帧时间。毫秒。GetTickCount();
long long m_pts;    // 上次时间戳。
int m_audioPackLen; // 每个音频帧数据大小
bool m_needPLI; // 是否收到关键帧请求，需要通知编码器生成关键帧

CCritSec locker; // 锁，用于保护file句柄的并发操作
FILE* m_audioFile;  // 音频输入文件
FILE* m_videoFile;  // 音频输入文件

BVCU_PUCFG_PTZAttr m_ptzAttr; // 云台测试

void GetMediaInfo(bvChannelParam* param)
{ // 获取音视频通道参数
    m_interval = 40;
    m_lasttime = 0;
    m_lastAdjtime = 0;
    m_lastPlitime = 0;
    m_pts = 0;
    m_audioFile = 0;
    m_videoFile = 0;
    m_audioPackLen = 320;
    // 这里从配置文件中读取配置参数
    PUConfig puconfig;
    LoadConfig(&puconfig);
    // 视频编码参数
    if (strstr(puconfig.videoFile, "265") != nullptr)
        strncpy_s(param->szVideoCodec, sizeof(param->szVideoCodec), "H265", _TRUNCATE);
    else
        strncpy_s(param->szVideoCodec, sizeof(param->szVideoCodec), "H264", _TRUNCATE);
    // 音频编码参数
    param->iBitrate = 32000;
    param->iChannelCount = 1;
    param->iSampleRate = 8000;
    if (strstr(puconfig.audioFile, "g726") != nullptr)
    {
        m_audioPackLen = 160;
        strncpy_s(param->szAudioCodec, sizeof(param->szAudioCodec), "G726", _TRUNCATE);
    }
    else if (strstr(puconfig.audioFile, "maac") != nullptr)
    {
        m_audioPackLen = 2048; // maac 长度在文件中。
        m_interval = 32;       // 自带的 maac的音频间隔是32毫秒，根据你的数据源决定。
        strncpy_s(param->szAudioCodec, sizeof(param->szAudioCodec), "AAC", _TRUNCATE);
        param->iBitrate = 64000;
        param->iSampleRate = 32000;
    }
    else
    {
        m_audioPackLen = 320; // g711a
        strncpy_s(param->szAudioCodec, sizeof(param->szAudioCodec), "PCMA", _TRUNCATE);
    }
}

static char* findhead(char* data, int len)
{
    if (len < 4)
        return 0;
    for (int i = 0; i <= len - 3; i++)
    {
        if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)
        {
            if (i != 0 && data[i - 1] == 0)
                return &data[i - 1];
            return &data[i];
        }
    }
    return 0;
}
char* ReadVideo(char* buf, int* len)
{
    int buflen = *len - 160;
    *len = 0;
    if (m_videoFile == 0)
        return 0;
    char* pRead = buf;
    char* pStart = 0;
    char* pEnd = 0;
    int readlen = 0;
    int minPackLen = 80; // 读取帧最小大小，sps/pps要和I帧一起。
    while (readlen < buflen)
    {
        int onelen = fread(pRead, 1, 160, m_videoFile);
        readlen += onelen;
        if (onelen < 160)
        { // 可能读到文件结尾了，从头再来
            fseek(m_videoFile, 0, SEEK_SET);
            if (pStart != 0)
                pEnd = pRead + onelen;
            break;
        }
        if (pRead != buf)
        { // 防止hand头被切断，导致找不到。
            pRead -= 3;
            onelen += 3;
        }
        // 找00 00 00 01 头
        if (pStart == 0)
        {
            pStart = findhead(pRead, onelen);
            if (pStart != 0)
            { // 往后移，准备找end
                onelen = onelen - (pStart - pRead) - 3;
                pRead = pStart + 3;
                if (pStart[3] == 0x67 || pStart[4] == 0x67)
                    minPackLen = 980; // I帧不会比这个值小
            }
        }
        if (pStart != 0 && pEnd == 0 && readlen > minPackLen) // 已经找到开始，没有找到结尾，一个帧包不小于160，防止sps\pps单独发送。
        {
            pEnd = findhead(pRead, onelen);
            if (pEnd != 0)
            { // 多读了，
                int moreLen = onelen - (pEnd - pRead);
                fseek(m_videoFile, -moreLen, SEEK_CUR);
                break;
            }
        }
        pRead += onelen;
    }
    readlen = 0;
    if (pEnd != 0)
        readlen = pEnd - pStart;
    if (readlen > 0)
        *len = readlen;
    return pStart;
}
char* ReadAudio(char* buf, int* len)
{
    int buflen = *len;
    *len = 0;
    if (buflen < m_audioPackLen || m_audioFile == 0)
        return 0;
    int packlen = m_audioPackLen;
    if (m_audioPackLen == 2048) // maac, 读取包长度
    {
        int readlen = fread(&packlen, 1, 4, m_audioFile);
        if (readlen < 4)
            packlen = 0;
    }
    if (packlen <= 0 || packlen > buflen)
    { // 读取到的 包大小错误
        fseek(m_audioFile, 0, SEEK_SET);
        return 0;
    }
    int readlen = fread(buf, 1, packlen, m_audioFile);
    if (readlen < packlen)
    { // 可能读到文件结尾了，从头再来
        fseek(m_audioFile, 0, SEEK_SET);
        readlen = fread(buf, 1, packlen, m_audioFile);
    }
    if (readlen == packlen)
        *len = packlen;
    return buf;
}

// 模拟编码器线程，定时从文件中读取音视频数据，发送到服务器。
#ifdef _MSC_VER
unsigned __stdcall codec_thread(void*)
#else
void* codec_thread(void*)
#endif
{
    while ((m_audioFile || m_videoFile) && m_lasttime)
    {
        // ========================  定时从文件中读取音视频数据，并发送
        int now = GetTickCount();
        int dely = now - m_lasttime;
        if (dely < 0)
        {
            dely = m_interval;
            m_lasttime = now - m_interval;
        }
        locker.Lock();
        while (dely >= m_interval)
        {
            m_lasttime += m_interval;
            dely -= m_interval;

            m_pts = m_pts + m_interval * 1000;
            static char g_packetbuf[1 * 1024 * 1024]; // 这里限制了一个帧的大小不要超过 1M，因为自带的音视频文件中帧数据都很小。
            if (m_audioFile != 0)
            { // 音频被打开了，发送音频数据。
                int len = sizeof(g_packetbuf);
                char* pdata = ReadAudio(g_packetbuf, &len);
                if (len > 0)
                    SendAudioData(m_pts, pdata, len);
                // printf("send audio Data, %d\n", len);
            }
            if (m_videoFile != 0)
            { // 视频被打开了，发送视频数据。
                int len = sizeof(g_packetbuf);
                char* pdata = ReadVideo(g_packetbuf, &len);
                if (len > 0)
                    SendVideoData(m_pts, pdata, len);
                // printf("send audio Data, %d\n", len);
            }
        }
        // =======================  获取网络发送统计数据，根据猜测码率，动态调整编码码率（这里是假的）。
        dely = now - m_lastAdjtime;
        if (dely >= 2 * 1000 || m_lastAdjtime == 0)
        {
            m_lastAdjtime = now;
            int bandwidth = GetGuessBandwidth();
            printf("================  guess bandwidth = %d kbps\n", bandwidth);
        }
        // 判断是否需要关键帧
        if (m_needPLI) {
            if (m_videoFile)
            { // 这里跳转到文件开始位置，是因为 文件开始位置是关键帧。
                fseek(m_videoFile, 0, SEEK_SET);
            }
            if (m_audioFile)
            { // 这里跳转到文件开始位置，是因为 文件开始位置是关键帧。
                fseek(m_audioFile, 0, SEEK_SET);
            }
            m_needPLI = false;
        }
        locker.Unlock();
        Sleep(100);
    }
#ifdef _MSC_VER
    return 0;
#endif
}

// 开启采集和编码器,这里是打开音视频文件。
void OpenMedia(bool video, bool audio, bool audioOut)
{
    locker.Lock();
    bool bHasOpen = m_videoFile || m_audioFile; // 如果已经打开了，就不需要再创建编码线程
    Sleep(100); // 模拟打开摄像头和编码器延迟。
    // 音视频通道可能会多次收到打开请求，因为请求多会修改需要的媒体类型。所以要注意不要重复打开。
    printf("================  recv open media request \n");
    PUConfig puconfig;
    LoadConfig(&puconfig);
    if (video)
    { // 请求视频，打开视频输入设备
        printf("================  open video media now: %s\n", puconfig.videoFile);
        if (m_videoFile == 0)
            m_videoFile = fopen(puconfig.videoFile, "rb");
        if (m_videoFile == 0)
        {
            printf(">>>>>>>>>>>>>>> open video file failed!!!!!! %s", puconfig.videoFile);
        }
    }
    else if (m_videoFile)
    { // 不需要视频，如果已经打开，请关闭
        fclose(m_videoFile);
        m_videoFile = 0;
    }
    if (audio)
    { // 请求音频，打开音频输入设备
        printf("================  open audio media now: %s\n", puconfig.audioFile);
        if (m_audioFile == 0)
            m_audioFile = fopen(puconfig.audioFile, "rb");
        if (strstr(puconfig.audioFile, "g726") != nullptr)
            m_audioPackLen = 160;
        else if (strstr(puconfig.audioFile, "maac") != nullptr)
        {
            m_audioPackLen = 2048; // maac 长度在文件中。
            m_interval = 32;       // 自带的 maac的音频间隔是32毫秒，根据你的数据源决定。
        }
        else
            m_audioPackLen = 320; // g711a
    }
    else if (m_audioFile)
    { // 不需要音频，如果已经打开，请关闭
        fclose(m_audioFile);
        m_audioFile = 0;
    }
    if (audioOut)
    { // 请求音频输出，打开音频输出设备
        printf("================  open audio out now\n");
    }
    locker.Unlock();
    m_lasttime = GetTickCount();
    m_pts = time(0) * 1000000;
    if (!bHasOpen) {
#ifdef _MSC_VER
        _beginthreadex(NULL, 0, codec_thread, NULL, 0, NULL);
#else
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, codec_thread, NULL);
#endif
    }
}

void CloseMedia()
{
    // 这里应该可以关闭您的音视频输入设备了。
    printf("================  media closed \n");
    locker.Lock();
    m_lasttime = 0;
    m_lastAdjtime = 0;
    m_lastPlitime = 0;
    if (m_audioFile)
    {
        fclose(m_audioFile);
        m_audioFile = 0;
    }
    if (m_videoFile)
    {
        fclose(m_videoFile);
        m_videoFile = 0;
    }
    locker.Unlock();
}

void ReqPLI()
{
    // 这里应该通知您的视频编码器生成关键帧。
    printf("================  media pli \n");
    int now = GetTickCount();
    int dely = now - m_lastPlitime;
    if (dely >= 2 * 1000 || m_lastPlitime == 0) // 最快2秒请求一次，不建议关键帧过多。
    {
        m_lastPlitime = now;
        m_needPLI = true;
    }
}
