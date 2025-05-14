#include <string.h>
#include <stdlib.h>
#include "PUConfig.h"
#include "gps.h"
#include "config.h"

BVCU_PUCFG_GPSData m_position; // 当前位置。模拟的，您可以从GPS设备中获取。
int m_lat;                     // 中心位置，模拟位置以中心位置为圆点，画圆运动。
int m_lng;
int m_chagedu; // 半径, 模拟画圆的半径。

void OpenGPS()
{

    memset(&m_position, 0x00, sizeof(m_position));
    m_position.bAntennaState = 1;
    m_position.bOrientationState = 1;
    m_position.iStarCount = 3;
    m_position.iSatelliteSignal = BVCU_PUCFG_SATELLITE_BDS;

    // 从配置文件中读取配置的中心位置，模拟位置以中心位置为圆点，画圆运动。
    PUConfig puconfig;
    LoadConfig(&puconfig);
    if (0 >= puconfig.interval || puconfig.interval <= 10 * 60)
        puconfig.interval = 5;
    m_lat = puconfig.lat;
    m_lng = puconfig.lng;
    m_position.iSatelliteSignal = puconfig.satelliete;
    m_chagedu = 200000;
    m_position.iLatitude = m_lat + m_chagedu;
    m_position.iLongitude = m_lng - m_chagedu;
}

BVCU_PUCFG_GPSData* GetGPSData()
{
    // 模拟从设备中读取GPS位置数据
    time_t now = time(NULL);     // 时间应该也是从GPS设备中读取
    tm* ptm = (tm*)gmtime(&now); // 除非特殊说明，网传的时间都是UTC时间。
    m_position.stTime.iYear = ptm->tm_year + 1900;
    m_position.stTime.iMonth = ptm->tm_mon + 1;
    m_position.stTime.iDay = ptm->tm_mday;
    m_position.stTime.iHour = ptm->tm_hour;
    m_position.stTime.iMinute = ptm->tm_min;
    m_position.stTime.iSecond = ptm->tm_sec;
    if (m_position.iLongitude > m_lng)
    {
        if (m_position.iLatitude > m_lat)
        {
            m_position.iLongitude += 2000;
            if (m_position.iLongitude > m_lng + m_chagedu)
            {
                m_position.iLongitude = m_lng + m_chagedu;
                m_position.iLatitude = m_lat;
                m_position.iAngle = 45000;
            }
            else
            {
                m_position.iLatitude -= 2000;
                m_position.iAngle = 135000;
            }
        }
        else
        {
            m_position.iLongitude -= 2000;
            m_position.iLatitude -= 2000;
            m_position.iAngle = 225000;
            if (m_position.iStarCount > 0)
                m_position.iStarCount = 0;
            else
                m_position.iStarCount = 6;
        }
    }
    else if (m_position.iLongitude == m_lng)
    {
        if (m_position.iLatitude > m_lat)
        {
            m_position.iLongitude += 2000;
            m_position.iLatitude -= 2000;
            m_position.iAngle = 135000;
        }
        else
        {
            m_position.iLongitude -= 2000;
            m_position.iLatitude += 2000;
            m_position.iAngle = 315000;
        }
    }
    else
    {
        if (m_position.iLatitude >= m_lat)
        {
            m_position.iLongitude += 2000;
            m_position.iLatitude += 2000;
            m_position.iAngle = 45000;
        }
        else
        {
            m_position.iLongitude -= 2000;
            if (m_position.iLongitude < m_lng - m_chagedu)
            {
                m_position.iLongitude = m_lng - m_chagedu;
                m_position.iLatitude = m_lat;
                m_position.iAngle = 225000;
            }
            m_position.iLatitude += 2000;
            m_position.iAngle = 315000;
        }
    }
    return &m_position;
}