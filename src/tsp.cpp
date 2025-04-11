#include <string>
#include <cstdlib>
#include <stdio.h>
#include "utils.h"
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
    // ========================  ��ʱ���豸�л�ȡ����λ�ã����ϱ��� ������ģ��λ��
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
    printf("================  tsp set name. %s \n", name);
    SetName(name);
    return BVCU_RESULT_S_OK;
}
BVCU_Result CTSPChannel::OnOpenRequest()
{
    // ���������tsp�豸�������յ���Ӳ�����ݷ���ƽ̨��
    printf("================  recv open tsp request \n");
    return BVCU_RESULT_S_OK;
}
void CTSPChannel::OnOpen()
{
    // ͨ���Ѿ������ɹ������Կ�ʼ�ϱ������ˡ�
    printf("================  open tsp success \n");
}
void CTSPChannel::OnClose()
{
    // ����Ӧ�ÿ��Թر�����tsp�豸�ˡ�
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
    printf("================  tsp get param. \n");
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
    printf("================  tsp set param. baudrate: %d \n", pParam->stRS232.iBaudRate);
    return BVCU_RESULT_S_OK;
}
