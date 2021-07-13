#include <string>
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
            }
        }
        if ( pStart != 0 && pEnd == 0 && readlen > 160) // 已经找到开始，没有找到结尾，一个帧包不小于160，防止sps\pps单独发送。
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
    int readlen = fread(buf, 1, m_audioPackLen, m_audioFile);
    if (readlen < m_audioPackLen)
    { // 可能读到文件结尾了，从头再来
        fseek(m_audioFile, 0, SEEK_SET);
        readlen = fread(buf, 1, m_audioPackLen, m_audioFile);
    }
    if (readlen == m_audioPackLen)
        *len = m_audioPackLen;
    return buf;
}
void CMediaChannel::Reply()
{
    static char videoEx[64] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x80, 0x1E, 0xDA, 0x05, 0x02, 0x11, 0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80 };
    static int  videoExLen = 20;
    if (m_bOpening)
    {
        BVCSP_VideoCodec videoSdp;
        BVCSP_AudioCodec audioSdp;
        memset(&videoSdp, 0x00, sizeof(videoSdp));
        memset(&audioSdp, 0x00, sizeof(audioSdp));
        videoSdp.codec = SAVCODEC_ID_H264;
        videoSdp.pExtraData = videoEx;
        videoSdp.iExtraDataSize = videoExLen;
        if (m_audioPackLen == 160)
            audioSdp.codec = SAVCODEC_ID_G726;
        else
            audioSdp.codec = SAVCODEC_ID_G711A;
        audioSdp.iBitrate = 32000;
        audioSdp.iChannelCount = 1;
        audioSdp.iSampleRate = 8000;
        audioSdp.eSampleFormat = SAV_SAMPLE_FMT_S16;

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
    if (BNeedVideoIn())
    {   // 请求视频，打开视频输入设备
        printf("================  open video media now\n");
        if (m_videoFile == 0)
            m_videoFile = fopen(VIDEO_FILE_PATH_NAME, "rb");
    }
    else if (m_videoFile)
    {   // 不需要视频，如果已经打开，请关闭
        fclose(m_videoFile);
        m_videoFile = 0;
    }
    if (BNeedAudioIn())
    {   // 请求音频，打开音频输入设备
        PUConfig puconfig;
        LoadConfig(&puconfig);
        printf("================  open audio media now\n");
        if (m_audioFile == 0)
            m_audioFile = fopen(puconfig.audioFile, "rb");
        if (strstr(puconfig.audioFile, "g726") != nullptr)
            m_audioPackLen = 160;
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
    }
}
void CMediaChannel::OnRecvAudio(long long iPTS, const void* pkt, int len)
{
    printf("================  media recv audio. len: %d \n", len);
}

BVCU_Result CMediaChannel::OnPTZCtrl(const BVCU_PUCFG_PTZControl* ptzCtrl)
{
    printf("================  media recv ptz control. command:%d %s\n", ptzCtrl->iPTZCommand, ptzCtrl->bStop?"stop":"start");
    return BVCU_RESULT_S_OK;
}
