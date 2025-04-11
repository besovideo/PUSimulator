﻿#include <string>
#include <cstdlib>
#include <stdio.h>
#include "utils.h"
#include "media.h"
#include "config.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif

CMediaChannel::CMediaChannel()
    : CAVChannelBase(true, true, true, true)
{
    m_interval = 40;
    PUConfig puconfig;
    LoadConfig(&puconfig);
    SetName(puconfig.mediaName);
    m_lasttime = 0;
    m_lastAdjtime = 0;
    m_lastPlitime = 0;
    m_pts = 0;
    m_audioFile = 0;
    m_videoFile = 0;
    m_audioPackLen = 320;
    // 填充测试云台数据
    memset(&m_ptzAttr, 0x00, sizeof(m_ptzAttr));
    m_ptzAttr.iPTZProtocolAll[0] = BVCU_ALERTIN_HARDWARE_485;
    m_ptzAttr.iPTZProtocolAll[1] = BVCU_ALERTIN_HARDWARE_232;
    m_ptzAttr.iPTZProtocolIndex = 1;
    m_ptzAttr.stRS232.iDataBit = 8;
    m_ptzAttr.stRS232.iStopBit = 0;
    m_ptzAttr.stRS232.iParity = 1;
    m_ptzAttr.stRS232.iBaudRate = 9600;
    m_ptzAttr.stRS232.iFlowControl = 0;
    m_ptzAttr.iAddress = 128;
    m_ptzAttr.stPreset[0].iID = 1;
    strncpy_s(m_ptzAttr.stPreset[0].szPreset, sizeof(m_ptzAttr.stPreset[0].szPreset), "test", _TRUNCATE);
    m_ptzAttr.stPreset[1].iID = -1;
    m_ptzAttr.stCruise[0].iID = -1;
}

static char* findhead(char* data, int len) {
    if (len < 4)
        return 0;
    for (int i = 0; i <= len - 3; i++) {
        if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)
        {
            if (i != 0 && data[i - 1] == 0)
                return &data[i - 1];
            return &data[i];
        }
    }
    return 0;
}
char* CMediaChannel::ReadVideo(char* buf, int* len)
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
        {  // 防止hand头被切断，导致找不到。
            pRead -= 3;
            onelen += 3;
        }
        // 找00 00 00 01 头
        if (pStart == 0)
        {
            pStart = findhead(pRead, onelen);
            if (pStart != 0)
            {  // 往后移，准备找end
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
            {  // 多读了，
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
char* CMediaChannel::ReadAudio(char* buf, int* len)
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
    {  // 读取到的 包大小错误
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
void CMediaChannel::Reply()
{
    if ((time(NULL) - m_replytime) < 0) {
        return;
    }
    //static char videoEx[64] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x80, 0x1E, 0xDA, 0x05, 0x02, 0x11, 0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80 };
    static int  videoExLen = 20;
    static unsigned char audioEx[2] = { 0x12, 0x88 };
    // 以上扩展数据，都是根据自带 音视频文件数据编码参数写死的，修改数据来源时，需要根据您的数据源确定。
    if (m_bOpening)
    {
        BVCSP_VideoCodec videoSdp;
        BVCSP_AudioCodec audioSdp;
        memset(&videoSdp, 0x00, sizeof(videoSdp));
        memset(&audioSdp, 0x00, sizeof(audioSdp));
        videoSdp.codec = m_videoCodec;

        //videoSdp.pExtraData = videoEx;
        //videoSdp.iExtraDataSize = videoExLen;
        audioSdp.iBitrate = 32000;
        audioSdp.iChannelCount = 1;
        audioSdp.iSampleRate = 8000;
        audioSdp.eSampleFormat = SAV_SAMPLE_FMT_S16;
        if (m_audioPackLen == 160)
            audioSdp.codec = SAVCODEC_ID_G726;
        else if (m_audioPackLen == 2048)
        {
            audioSdp.codec = SAVCODEC_ID_AAC;
            audioSdp.iBitrate = 64000;
            audioSdp.iSampleRate = 32000;
            audioSdp.pExtraData = (char *)audioEx;
            audioSdp.iExtraDataSize = 2;
        }
        else
            audioSdp.codec = SAVCODEC_ID_G711A;

        ReplySDP(BVCU_RESULT_S_OK, &videoSdp, &audioSdp);
    }
}
void CMediaChannel::SendData()
{
    Reply();
    if (!BOpen() || m_lasttime == 0)
        return;
    // ========================  定时从设备中获取最新位置，并上报， 下面是模拟位置
    int now = GetTickCount();
    int dely = now - m_lasttime;
    if (dely < 0) {
        dely = m_interval;
        m_lasttime = now - m_interval;
    }
    while (dely >= m_interval)
    {
        m_lasttime += m_interval;
        dely -= m_interval;

        m_pts = m_pts + m_interval * 1000;
        static char g_packetbuf[1 * 1024 * 1024]; // 这里限制了一个帧的大小不要超过 1M，因为自带的音视频文件中帧数据都很小。
        if (m_audioFile != 0)
        {  // 音频被打开了，发送音频数据。
            int len = sizeof(g_packetbuf);
            char* pdata = ReadAudio(g_packetbuf, &len);
            if (len > 0)
                WriteAudio(m_pts, pdata, len);
            //printf("send audio Data, %d\n", len);
        }
        if (m_videoFile != 0)
        {  // 视频被打开了，发送视频数据。
            int len = sizeof(g_packetbuf);
            char* pdata = ReadVideo(g_packetbuf, &len);
            if (len > 0)
                WriteVideo(m_pts, pdata, len);
            //printf("send audio Data, %d\n", len);
        }
    }
    // =======================  获取网络发送统计数据，根据猜测码率，动态调整编码码率（这里是假的）。
    dely = now - m_lastAdjtime;
    if (dely >= 2 * 1000 || m_lastAdjtime == 0)
    {
        m_lastAdjtime = now;
        BVCSP_DialogInfo dlgInfo;
        memset(&dlgInfo, 0x00, sizeof(dlgInfo));
        BVCU_Result result = BVCSP_GetDialogInfo(m_hDialog, &dlgInfo);
        if (BVCU_Result_SUCCEEDED(result))
        {
            printf("================  guess bandwidth = %d kbps\n", dlgInfo.iGuessBandwidthSend);
        }
    }
}

BVCU_Result CMediaChannel::OnSetName(const char* name)
{
    printf("================  media set name. %s \n", name);
    PUConfig puconfig;
    LoadConfig(&puconfig);
    strncpy_s(puconfig.mediaName, sizeof(puconfig.mediaName), name, _TRUNCATE);
    SetConfig(&puconfig);
    SetName(name);
    return BVCU_RESULT_S_OK;
}
BVCU_Result CMediaChannel::OnOpenRequest()
{
    // 这里打开您的音视频设备，异步打开成功后，调用ReplySDP()接口回复请求。
    // 音视频通道可能会多次收到打开请求，因为请求多会修改需要的媒体类型。所以要注意不要重复打开。
    printf("================  recv open media request \n");
    PUConfig puconfig;
    LoadConfig(&puconfig);
    if (BNeedVideoIn())
    {   // 请求视频，打开视频输入设备
        printf("================  open video media now: %s\n", puconfig.videoFile);
        if (m_videoFile == 0)
            m_videoFile = fopen(puconfig.videoFile, "rb");
        if (m_videoFile == 0) {
            printf(">>>>>>>>>>>>>>> open video file failed!!!!!! %s", puconfig.videoFile);
        }
        m_videoCodec = SAVCODEC_ID_H264;
        if (strstr(puconfig.videoFile, "265") != nullptr) {
            m_videoCodec = SAVCODEC_ID_H265;
        }
    }
    else if (m_videoFile)
    {   // 不需要视频，如果已经打开，请关闭
        fclose(m_videoFile);
        m_videoFile = 0;
    }
    if (BNeedAudioIn())
    {   // 请求音频，打开音频输入设备
        printf("================  open audio media now: %s\n", puconfig.audioFile);
        if (m_audioFile == 0)
            m_audioFile = fopen(puconfig.audioFile, "rb");
        if (strstr(puconfig.audioFile, "g726") != nullptr)
            m_audioPackLen = 160;
        else if (strstr(puconfig.audioFile, "maac") != nullptr)
        {
            m_audioPackLen = 2048; // maac 长度在文件中。
            m_interval = 32; // 自带的 maac的音频间隔是32毫秒，根据你的数据源决定。
        }
        else
            m_audioPackLen = 320; // g711a
    }
    else if (m_audioFile)
    {   // 不需要音频，如果已经打开，请关闭
        fclose(m_audioFile);
        m_audioFile = 0;
    }
    if (BNeedAudioOut())
    {   // 请求音频输出，打开音频输出设备
        printf("================  open audio out now\n");
    }
    m_replytime = time(NULL) + puconfig.Slow;
    return BVCU_RESULT_S_OK;
}
void CMediaChannel::OnOpen()
{
    // 通道已经建立成功，可以开始上报数据了。
    printf("================  open media success \n");
    m_lasttime = GetTickCount();
    m_pts = time(0) * 1000000;
}
void CMediaChannel::OnClose()
{
    // 这里应该可以关闭您的音视频输入设备了。
    printf("================  media closed \n");
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
}
void CMediaChannel::OnPLI()
{
    // 这里应该通知您的视频编码器生成关键帧。
    printf("================  media pli \n");
    int now = GetTickCount();
    int dely = now - m_lastPlitime;
    if (dely >= 2 * 1000 || m_lastPlitime == 0)
    {
        m_lastPlitime = now;
        if (m_videoFile)
        {   // 这里跳转到文件开始位置，是因为 文件开始位置是关键帧。
            fseek(m_videoFile, 0, SEEK_SET);
        }
        if (m_audioFile)
        {   // 这里跳转到文件开始位置，是因为 文件开始位置是关键帧。
            fseek(m_audioFile, 0, SEEK_SET);
        }
    }
}
void CMediaChannel::OnRecvAudio(long long iPTS, const void* pkt, int len)
{
    printf("================  media recv audio. len: %d \n", len);
}

BVCU_Result CMediaChannel::OnPTZCtrl(const BVCU_PUCFG_PTZControl* ptzCtrl)
{
    printf("================  media recv ptz control. command:%d %s\n", ptzCtrl->iPTZCommand, ptzCtrl->bStop ? "stop" : "start");
    if (ptzCtrl->iPTZCommand == BVCU_PTZ_COMMAND_PRESET_SET) {
        if (ptzCtrl->iParam1 < 256 && ptzCtrl->iParam1 >= 0 && ptzCtrl->iParam2 != NULL) {
            for (int i = 0; i < 256; ++i) {
                if (m_ptzAttr.stPreset[i].iID < 0 || m_ptzAttr.stPreset[i].iID == ptzCtrl->iParam1) {
                    if (m_ptzAttr.stPreset[i].iID <= 0 && i < 255) {
                        m_ptzAttr.stPreset[i + 1].iID = -1;
                    }
                    m_ptzAttr.stPreset[i].iID = ptzCtrl->iParam1;
                    strncpy_s(m_ptzAttr.stPreset[i].szPreset, sizeof(m_ptzAttr.stPreset[i].szPreset), (char*)(*(intptr_t*)(&ptzCtrl->iParam2)), _TRUNCATE);
                    break;
                }
            }
        }
    }
    else if (ptzCtrl->iPTZCommand == BVCU_PTZ_COMMAND_PRESET_SETNAME) {
        for (int i = 0; i < 256; ++i) {
            if (m_ptzAttr.stPreset[i].iID == ptzCtrl->iParam1 && ptzCtrl->iParam2 != NULL) {
                strncpy_s(m_ptzAttr.stPreset[i].szPreset, sizeof(m_ptzAttr.stPreset[i].szPreset), (char*)(*(intptr_t*)(&ptzCtrl->iParam2)), _TRUNCATE);
                break;
            }
        }
    }
    else if (ptzCtrl->iPTZCommand == BVCU_PTZ_COMMAND_PRESET_DEL) {
        for (int i = 0; i < 256; ++i) {
            if (m_ptzAttr.stPreset[i].iID < 0) {
                break;
            }
            else if (m_ptzAttr.stPreset[i].iID == ptzCtrl->iParam1) {
                for (int j = i; j < 255; ++j) {
                    m_ptzAttr.stPreset[j].iID = m_ptzAttr.stPreset[j + 1].iID;
                    strncpy_s(m_ptzAttr.stPreset[j].szPreset, sizeof(m_ptzAttr.stPreset[j].szPreset), m_ptzAttr.stPreset[j + 1].szPreset, _TRUNCATE);
                }
                break;
            }
        }
    }
    if (ptzCtrl->iPTZCommand == BVCU_PTZ_COMMAND_CRUISE_SET) {
        if (ptzCtrl->iParam1 < 31 && ptzCtrl->iParam1 >= 0 && ptzCtrl->iParam2 != NULL) {
            for (int i = 0; i < 31; ++i) {
                if (m_ptzAttr.stCruise[i].iID < 0 || m_ptzAttr.stCruise[i].iID == ptzCtrl->iParam1) {
                    if (m_ptzAttr.stCruise[i].iID <= 0 && i < 255) {
                        m_ptzAttr.stCruise[i + 1].iID = -1;
                    }
                    BVCU_PUCFG_Cruise* psource = (BVCU_PUCFG_Cruise*)(*(intptr_t*)(&ptzCtrl->iParam2));
                    m_ptzAttr.stCruise[i].iID = ptzCtrl->iParam1;
                    strncpy_s(m_ptzAttr.stCruise[i].szName, sizeof(m_ptzAttr.stCruise[i].szName), psource->szName, _TRUNCATE);
                    for (int j = 0; j < 32; j++)
                        m_ptzAttr.stCruise[i].stPoints[j] = psource->stPoints[j];
                    break;
                }
            }
        }
    }
    else if (ptzCtrl->iPTZCommand == BVCU_PTZ_COMMAND_CRUISE_DEL) {
        for (int i = 0; i < 256; ++i) {
            if (m_ptzAttr.stCruise[i].iID < 0) {
                break;
            }
            else if (m_ptzAttr.stCruise[i].iID == ptzCtrl->iParam1) {
                for (int j = i; j < 31; ++j) {
                    m_ptzAttr.stCruise[j] = m_ptzAttr.stCruise[j + 1];
                }
                break;
            }
        }
    }
    return BVCU_RESULT_S_OK;
}
