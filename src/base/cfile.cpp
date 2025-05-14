
#include "cfile.h"
#ifdef _MSC_VER
#include <io.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

bv_file* g_file;

static FILE* bvf_fsopen(const char* pathName, const char* mode, int bWrite)
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

static int bvf_fclose(FILE* _File)
{
    if (_File != NULL)
        return fclose(_File);
    return 0;
}

static int bvf_fread(void* _DstBuf, int _ElementSize, int _Count, FILE* _File)
{
    return fread(_DstBuf, _ElementSize, _Count, _File);
}

static int bvf_fwrite(const void* _Str, int _Size, int _Count, FILE* _File)
{
    return fwrite(_Str, _Size, _Count, _File);
}

static int bvf_fseek(FILE* _File, long _Offset, int _Origin)
{
    return fseek(_File, _Offset, _Origin);
}

static long bvf_ftell(FILE* _File)
{
    return ftell(_File);
}

static bool bvf_access(const char* pathname)
{
#ifdef _MSC_VER
    if (_access(pathname, 0) != -1)
    {
        return true;
    }
#else
    if (access(pathname, F_OK) != -1)
    {
        return true;
    }
#endif

    return false;
}

void SetFileFunc(bv_file* bvfile)
{
    if (bvfile && bvfile->fsopen && bvfile->fclose && bvfile->fread && bvfile->fwrite &&
        bvfile->fseek && bvfile->ftell && bvfile->access)
    {
        g_file = bvfile;
    }
    else
    {
        static bv_file defaultFileFunc = { bvf_fsopen, bvf_fclose, bvf_fread, bvf_fwrite, bvf_fseek, bvf_ftell, bvf_access };
        g_file = &defaultFileFunc;
    }
}
