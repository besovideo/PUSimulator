#include <string>
#include "config.h"

#ifdef _MSC_VER
#include <cstdlib>
#include <iostream>
#include <Windows.h>

void LoadConfig(PUConfig* pConfig)
{
    char tempbuf[64];
    GetPrivateProfileStringA("info", "id", DEFAULT_ID, pConfig->ID, sizeof(pConfig->ID), CONFIG_FILE_PATH_NAME);
    if (pConfig->ID[0] == 0)
        strncpy_s(pConfig->ID, DEFAULT_ID, _TRUNCATE);
    GetPrivateProfileStringA("info", "name", DEFAULT_NAME, pConfig->Name, sizeof(pConfig->Name), CONFIG_FILE_PATH_NAME);
    if (pConfig->Name[0] == 0)
        strncpy_s(pConfig->Name, DEFAULT_NAME, _TRUNCATE);
    GetPrivateProfileStringA("info", "lat", "200.0", tempbuf, sizeof(tempbuf), CONFIG_FILE_PATH_NAME);
    double lat = atof(tempbuf);
    GetPrivateProfileStringA("info", "lng", "200.0", tempbuf, sizeof(tempbuf), CONFIG_FILE_PATH_NAME);
    double lng = atof(tempbuf);
    if (-90.0 < lat && lat < 90.0)
        pConfig->lat = lat * 10000000;
    if (-180.0 < lng && lng < 180.0)
        pConfig->lng = lng * 10000000;
    // server
    GetPrivateProfileStringA("server", "ip", DEFAULT_SERVERIP, pConfig->serverIP, sizeof(pConfig->serverIP), CONFIG_FILE_PATH_NAME);
    if (pConfig->serverIP[0] == 0)
        strncpy_s(pConfig->serverIP, DEFAULT_SERVERIP, _TRUNCATE);
    pConfig->serverPort = GetPrivateProfileIntA("server", "port", DEFAULT_SERVERPORT, CONFIG_FILE_PATH_NAME);
    if (0 >= pConfig->serverPort || pConfig->serverPort >= 65535)
        pConfig->serverPort = DEFAULT_SERVERPORT;
    pConfig->protoType = GetPrivateProfileIntA("server", "type", DEFAULT_PROTOTYPE, CONFIG_FILE_PATH_NAME);
}

int SetConfig(const PUConfig* pConfig)
{
    char tempbuf[64];
    WritePrivateProfileStringA("info", "id", pConfig->ID, CONFIG_FILE_PATH_NAME);
    WritePrivateProfileStringA("info", "name", pConfig->Name, CONFIG_FILE_PATH_NAME);
    sprintf(tempbuf, "%0.07f", (double)pConfig->lat / 10000000.0);
    WritePrivateProfileStringA("info", "lat", tempbuf, CONFIG_FILE_PATH_NAME);
    sprintf(tempbuf, "%0.07f", (double)pConfig->lng / 10000000.0);
    WritePrivateProfileStringA("info", "lng", tempbuf, CONFIG_FILE_PATH_NAME);
    WritePrivateProfileStringA("server", "ip", pConfig->serverIP, CONFIG_FILE_PATH_NAME);
    sprintf(tempbuf, "%d", pConfig->serverPort);
    WritePrivateProfileStringA("server", "port", tempbuf, CONFIG_FILE_PATH_NAME);
    sprintf(tempbuf, "%d", pConfig->protoType);
    WritePrivateProfileStringA("server", "type", tempbuf, CONFIG_FILE_PATH_NAME);
    return 0;
}
#else
void LoadConfig(PUConfig* pConfig)
{
    strncpy_s(pConfig->ID, DEFAULT_ID);
    strncpy_s(pConfig->Name, DEFAULT_NAME);
    strncpy_s(pConfig->serverIP, DEFAULT_SERVERIP);
    pConfig->serverPort = DEFAULT_SERVERPORT;
    pConfig->protoType = DEFAULT_PROTOTYPE;
    pConfig->lat = 200 * 10000000;
    pConfig->lng = 200 * 10000000;
}

int SetConfig(const PUConfig* pConfig)
{
    return 0;
}
#endif
