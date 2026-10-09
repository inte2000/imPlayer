#ifndef __SYSTEM_FUNCTION_H__
#define __SYSTEM_FUNCTION_H__

#include "std_def.h"

std::wstring GetSystemDocPath();
std::wstring GetSystemMusicPath();
std::wstring GetSystemTmpPath();
std::wstring GetSystemDownloadPath();

BOOL StartProgramExe(const wchar_t* progname, const wchar_t* param);

#endif //__SYSTEM_FUNCTION_H__