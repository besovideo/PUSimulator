
#include <string>
#include "dialog.h"

CChannelBase::CChannelBase(int IndexBase)
    : m_index(0)
    , m_channelIndexBase(IndexBase)
    , m_supportMediaDir(0)
    , m_hDialog(0)
    , m_iOpenMediaDir(0)
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
    m_iOpenMediaDir = 0;
}

void CChannelBase::SetName(const char* name)
{
    strncpy_s(m_name, sizeof(m_name), name, _TRUNCATE);
}

CAVChannelBase::CAVChannelBase()
    : CChannelBase(BVCU_SUBDEV_INDEXMAJOR_MIN_CHANNEL)
{
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
