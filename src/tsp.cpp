#include <string>
#include "tsp.h"


CTSPChannel::CTSPChannel()
    : m_interval(5)
{
    m_interval = 5;
    SetName("tsp");
    m_lasttime = time(NULL) - m_interval;
}

void CTSPChannel::SendData()
{
    if (!BOpen())
        return;
    // ========================  定时从设备中获取最新位置，并上报， 下面是模拟位置
    time_t now = time(NULL);
    int dely = now - m_lasttime;
    if (dely >= m_interval)
    {
        m_lasttime = now;
        char tspData[128];
        sprintf(tspData, "hello world. %lld\n", now);
        int tsplen = strlen(tspData) + 1;
        WriteData(tspData, tsplen);
        printf("send tsp Data, %s\n", tspData);
    }
}

BVCU_Result CTSPChannel::OnSetName(const char* name)
{
    SetName(name);
    return BVCU_RESULT_S_OK;
}
BVCU_Result CTSPChannel::OnOpenRequest()
{
    // 这里打开您的tsp设备，并将收到的硬件数据发给平台。
    printf("================  recv open tsp request \n");
    return BVCU_RESULT_S_OK;
}
void CTSPChannel::OnOpen()
{
    // 通道已经建立成功，可以开始上报数据了。
    printf("================  open tsp success \n");
}
void CTSPChannel::OnClose()
{
    // 这里应该可以关闭您的tsp设备了。
    printf("================  tsp closed \n");
    return ;
}
void CTSPChannel::OnRecvData(const void* pkt, int len)
{
    printf("================  tsp recv data. len: %d ", len);
    const char* pData = (const char*)pkt;
    for (int i = 0; i < len; ++ i)
    {
        if (i % 32 == 0)
            printf("\n");
        printf("%02X ", pData[i]);
    }
    printf("\n");
}
const BVCU_PUCFG_SerialPort* CTSPChannel::OnGetTSPParam()
{
    static BVCU_PUCFG_SerialPort param;
    memset(&param, 0x00, sizeof(param));
    param.iAddress = 1;
    param.iType = 2;
    param.stRS232.iBaudRate = 9600;
    param.stRS232.iDataBit = 8;
    param.stRS232.iFlowControl = 0;
    param.stRS232.iParity = 0;
    param.stRS232.iStopBit = 0;
    return &param;
}
BVCU_Result CTSPChannel::OnSetTSPParam(const BVCU_PUCFG_SerialPort* pParam)
{
    return BVCU_RESULT_S_OK;
}
