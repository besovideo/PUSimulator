#include <string>
#include <stdio.h>
#include "file.h"

FILE* CMyFileTrans::bv_fsopen(const char* pathName, const char* mode, int bWrite)
{
#ifdef _MSC_VER
    return _fsopen(pathName, mode, 0x80); // _SH_SECURE);
#else
    FILE* pFile = fopen(pathName, mode);
    if (pFile)
    {
        if (bWrite)
        {
            if (flock(fileno(pFile), LOCK_EX | LOCK_NB) == 0)
                return pFile;
        }
        else
        {
            if (flock(fileno(pFile), LOCK_SH | LOCK_NB) == 0)
                return pFile;
        }
        fclose(pFile);
    }
#endif
    return NULL;
}

int CMyFileTrans::bv_fclose(FILE* _File)
{
    if (_File != NULL)
        return fclose(_File);
    return 0;
}

int CMyFileTrans::bv_fread(void* _DstBuf, int _ElementSize, int _Count, FILE* _File)
{
    return fread(_DstBuf, _ElementSize, _Count, _File);
}

int CMyFileTrans::bv_fwrite(const void* _Str, int _Size, int _Count, FILE* _File)
{
    return fwrite(_Str, _Size, _Count, _File);
}

int CMyFileTrans::bv_fseek(FILE* _File, long _Offset, int _Origin)
{
    return fseek(_File, _Offset, _Origin);
}

long CMyFileTrans::bv_ftell(FILE* _File)
{
    return ftell(_File);
}

BVCU_Result CMyFileTrans::OnFileRequest(BVCU_File_HTransfer hTransfer, BVCU_File_TransferParam* pParam)
{
    return BVCU_RESULT_E_UNSUPPORTED;
}

void CMyFileTrans::OnFileEvent(BVCU_File_HTransfer hTransfer, void* pUserData, int iEventCode, BVCU_Result iResult)
{
    if (m_OnEvent != NULL)
        m_OnEvent(hTransfer, pUserData, iEventCode, iResult);
}

CMyFileTrans::CMyFileTrans()
    : CFileTransManager()
{
}

CMyFileTrans::~CMyFileTrans()
{
}
