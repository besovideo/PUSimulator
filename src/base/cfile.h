#pragma once
#include <stdio.h>
#include "BVCSP.h"

// 安全打开文件，写独占。
typedef FILE *(*bv_fsopen)(const char *pathName, const char *mode, int bWrite);
typedef int (*bv_fclose)(FILE *_File);                                             // 关闭文件
typedef int (*bv_fread)(void *_DstBuf, int _ElementSize, int _Count, FILE *_File); // 读文件
typedef int (*bv_fwrite)(const void *_Str, int _Size, int _Count, FILE *_File);    // 写文件

typedef int (*bv_fseek)(FILE *_File, long _Offset, int _Origin); // 定位文件指针
typedef long (*bv_ftell)(FILE *_File);                           // 获取文件指针位置
typedef bool (*bv_access)(const char *pathname);                 // 判断文件是否存在

struct bv_file
{
    bv_fsopen fsopen;
    bv_fclose fclose;
    bv_fread fread;
    bv_fwrite fwrite;
    bv_fseek fseek;
    bv_ftell ftell;
    bv_access access;
};

// 设置 文件读写接口，非特殊文件系统不用设置，cfile.cpp中有基于c库的默认实现.
void SetFileFunc(bv_file *bvfile);
