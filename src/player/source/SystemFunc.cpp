#include "framework.h"
#include <shellapi.h>
#include <Shlobj.h>
#include "SystemFunc.h"

static std::wstring GetKnownFolderPath(REFKNOWNFOLDERID rfid)
{
    std::wstring strFolerPath;
    wchar_t* path = nullptr;
    if (SHGetKnownFolderPath(rfid, 0, nullptr, &path) == S_OK)
    {
        strFolerPath = path;
        CoTaskMemFree(path);
        path = nullptr;
    }

    return strFolerPath;
}

std::wstring GetSystemDocPath()
{
    return GetKnownFolderPath(FOLDERID_Documents);
}

std::wstring GetSystemMusicPath()
{
    return GetKnownFolderPath(FOLDERID_Music);
}

std::wstring GetSystemTmpPath()
{
    return GetKnownFolderPath(FOLDERID_Templates);
}

std::wstring GetSystemDownloadPath()
{
    return GetKnownFolderPath(FOLDERID_Downloads);
}

BOOL StartProgramExe(const wchar_t* progname, const wchar_t* param)
{
    SHELLEXECUTEINFO sei = { 0 };
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpFile = progname;
    sei.lpParameters = param;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpVerb = _T("open");
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteEx(&sei))
    {
        ::WaitForInputIdle(sei.hProcess, 3000); 
        ::CloseHandle(sei.hProcess);

        return TRUE;
    }

    return FALSE;
}
