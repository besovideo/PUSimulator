
#include <string>
#include <cstring>
#include "../utils.h"
#include "dialog.h"
#include "session.h"


CChannelBase::bvcsp_OnDialogEvent CChannelBase::g_bvcsp_onevent = 0;
CChannelBase::CChannelBase(int IndexBase)
    : m_index(0)
    , m_channelIndexBase(IndexBase)
    , m_supportMediaDir(0)
    , m_hDialog(0)
    , m_openMediaDir(0)
    , m_bOpening(false)
{
    memset(m_name, 0x00, sizeof(m_name));
}

CChannelBase::~CChannelBase()
{
    if (m_hDialog != 0)
    {
        BVCSP_Dialog_Close(m_hDialog);
        m_hDialog = 0;
    }
    m_openMediaDir = 0;
}

void CChannelBase::SetName(const char* name)
{
    strncpy_s(m_name, sizeof(m_name), name, _TRUNCATE);
}

BVCU_Result CChannelBase::OnRecvPacket(const BVCSP_Packet* packet)
{
    if (m_channelIndexBase == BVCU_SUBDEV_INDEXMAJOR_MIN_CHANNEL)
    {
        CAVChannelBase* pChannel = dynamic_cast<CAVChannelBase*>(this);
        if (pChannel && packet->iDataType == BVCSP_DATA_TYPE_AUDIO)
            pChannel->OnRecvAudio(packet->iPTS, packet->pData, packet->iDataSize);
    }
    else if (m_channelIndexBase == BVCU_SUBDEV_INDEXMAJOR_MIN_TSP)
    {
        CTSPChannelBase* pChannel = dynamic_cast<CTSPChannelBase*>(this);
        if (pChannel)
            pChannel->OnRecvData(packet->pData, packet->iDataSize);
    }
    return BVCU_RESULT_S_OK;
}

CAVChannelBase::CAVChannelBase(bool bVideoIn, bool bAudioIn, bool bAudioOut, bool ptz)
    : CChannelBase(BVCU_SUBDEV_INDEXMAJOR_MIN_CHANNEL)
{
    m_bptz = ptz;
    m_supportMediaDir = 0;
    if (bVideoIn)
        m_supportMediaDir |= BVCU_MEDIADIR_VIDEOSEND;
    if (bAudioIn)
        m_supportMediaDir |= BVCU_MEDIADIR_AUDIOSEND;
    if (bAudioOut)
        m_supportMediaDir |= BVCU_MEDIADIR_AUDIORECV;
}

BVCU_Result CAVChannelBase::ReplySDP(BVCU_Result result, const BVCSP_VideoCodec* video, const BVCSP_AudioCodec* audio)
{
    if (CChannelBase::g_bvcsp_onevent != 0 && m_hDialog != 0)
    {
        if (m_bOpening == false)
            return BVCU_RESULT_E_BADREQUEST;
        BVCSP_DialogInfo info;
        memset(&info, 0x00, sizeof(info));
        BVCU_Result iResult = BVCSP_GetDialogInfo(m_hDialog, &info);
        if (BVCU_Result_FAILED(iResult))
            return iResult;
        BVCSP_DialogParam* pParam = &info.stParam;
        if (video)
            pParam->szMyselfVideo = *video;
        if (audio)
        {
            pParam->szMyselfAudio = *audio;
            pParam->szTargetAudio = *audio;
        }
        pParam->afterRecv = CPUSessionBase::afterDialogRecv;
        pParam->OnEvent = CPUSessionBase::OnDialogEvent;
        //pParam->iOptions |= BVCSP_DIALOG_OPTIONS_NOFMTP;
        BVCSP_Event_DialogCmd rep;
        rep.iResult = result;
        rep.pDialogParam = pParam;
        CChannelBase::g_bvcsp_onevent(m_hDialog, BVCSP_EVENT_DIALOG_OPEN, &rep);
        SetBOpening(false);
        return BVCU_RESULT_S_OK;
    }
    return BVCU_RESULT_E_BADREQUEST;
}
BVCU_Result CAVChannelBase::WriteVideo(long long iPTS, const char* pkt, int len)
{
    if (m_hDialog != 0 && len > 0)
    {
        BVCSP_Packet packet;
        memset(&packet, 0x00, sizeof(packet));
        packet.iDataType = BVCSP_DATA_TYPE_VIDEO;
        packet.iDataSize = len;
        packet.pData = (void*)pkt;
        packet.iPTS = iPTS;
        return BVCSP_Dialog_Write(m_hDialog, &packet);
    }
    return BVCU_RESULT_E_BADSTATE;
}
BVCU_Result CAVChannelBase::WriteAudio(long long iPTS, const char* pkt, int len)
{
    if (m_hDialog != 0 && len > 0)
    {
        BVCSP_Packet packet;
        memset(&packet, 0x00, sizeof(packet));
        packet.iDataType = BVCSP_DATA_TYPE_AUDIO;
        packet.iDataSize = len;
        packet.pData = (void*)pkt;
        packet.iPTS = iPTS;
        return BVCSP_Dialog_Write(m_hDialog, &packet);
    }
    return BVCU_RESULT_E_BADSTATE;
}

CGPSChannelBase::CGPSChannelBase()
    : CChannelBase(BVCU_SUBDEV_INDEXMAJOR_MIN_GPS)
{
    m_supportMediaDir = BVCU_MEDIADIR_DATASEND;
}

BVCU_Result CGPSChannelBase::WriteData(const BVCU_PUCFG_GPSData* pGPSData)
{
    if (m_hDialog != 0)
    {
        BVCSP_Packet packet;
        memset(&packet, 0x00, sizeof(packet));
        packet.iDataType = BVCSP_DATA_TYPE_GPS;
        packet.iDataSize = sizeof(*pGPSData);
        packet.pData = (void*)pGPSData;
        return BVCSP_Dialog_Write(m_hDialog, &packet);
    }
    return BVCU_RESULT_E_BADSTATE;
}

CTSPChannelBase::CTSPChannelBase()
    : CChannelBase(BVCU_SUBDEV_INDEXMAJOR_MIN_TSP)
{
    m_supportMediaDir = BVCU_MEDIADIR_DATASEND | BVCU_MEDIADIR_DATARECV;
}
BVCU_Result CTSPChannelBase::WriteData(const char* pkt, int len)
{
    if (m_hDialog != 0)
    {
        BVCSP_Packet packet;
        memset(&packet, 0x00, sizeof(packet));
        packet.iDataType = BVCSP_DATA_TYPE_TSP;
        packet.iDataSize = len;
        packet.pData = (void*)pkt;
        return BVCSP_Dialog_Write(m_hDialog, &packet);
    }
    return BVCU_RESULT_E_BADSTATE;
}
