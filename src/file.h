#pragma once
#include "BVCSP.h"
#include "base/filetransfer.h"

class CMyFileTrans : public CFileTransManager
{
public:
    // 上层实现文件打开/关闭/读写 等相关接口
    virtual FILE* bv_fsopen(const char* pathName, const char* mode, int bWrite);
    virtual int bv_fclose(FILE* _File);
    virtual int bv_fread(void* _DstBuf, int _ElementSize, int _Count, FILE* _File);
    virtual int bv_fwrite(const void* _Str, int _Size, int _Count, FILE* _File);
    virtual int bv_fseek(FILE* _File, long _Offset, int _Origin);
    virtual long bv_ftell(FILE* _File);

    virtual BVCU_Result OnFileRequest(BVCU_File_HTransfer hTransfer, BVCU_File_TransferParam* pParam);
    virtual void  OnFileEvent(BVCU_File_HTransfer hTransfer, void* pUserData, int iEventCode, BVCU_Result iResult);

public:
    CMyFileTrans();
    virtual ~CMyFileTrans();

    void (*m_OnEvent)(BVCU_File_HTransfer hTransfer, void* pUserData, int iEventCode, BVCU_Result iResult);
};
