#include <format>
#include <ranges>
#include "StdFileSystem.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/param.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

static bool EnumFolderEntry(const stdfs::path& base_entry, bool bRecursion, const std::wstring& extName,
    const std::function<bool(const std::wstring& pathname)>& callback)
{
    bool bQuit = false;
    for (const auto& entry : stdfs::directory_iterator(base_entry))
    {
        if (entry.is_regular_file())
        {
            bool match = true;
            if (!extName.empty())
            {
                std::wstring extension = entry.path().extension().wstring();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
                match = (extName == extension);
            }
            if (match)
            {
                if (!callback(entry.path().wstring()))
                {
                    bQuit = true;
                    break;
                }
            }

        }
        else if (entry.is_directory() && bRecursion)
        {
            if (EnumFolderEntry(entry, bRecursion, extName, callback))
            {
                bQuit = true;
                break;
            }
        }
    }

    return bQuit;
}

void EnumFolder(const std::wstring& folder, bool bRecursion, const std::wstring& extFilter,
    const std::function<bool(const std::wstring& pathname)>& callback)
{
    stdfs::path base_entry(folder);
    std::wstring extname = extFilter;
    std::transform(extname.begin(), extname.end(), extname.begin(), ::towlower);
    EnumFolderEntry(base_entry, bRecursion, extname, callback);
}

static bool EnumFolderEntry2(const stdfs::path& base_entry, bool bRecursion, const std::set<std::wstring>& extNames,
    const std::function<bool(const std::wstring& pathname)>& callback)
{
    bool bQuit = false;
    for (const auto& entry : stdfs::directory_iterator(base_entry))
    {
        if (entry.is_regular_file())
        {
            bool match = true;
            if (!extNames.empty())
            {
                std::wstring extension = entry.path().extension().wstring();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
                match = (extNames.find(extension) != extNames.end());
            }
            if (match)
            {
                if (!callback(entry.path().wstring()))
                {
                    bQuit = true;
                    break;
                }
            }

        }
        else if (entry.is_directory() && bRecursion)
        {
            if (EnumFolderEntry2(entry, bRecursion, extNames, callback))
            {
                bQuit = true;
                break;
            }
        }
    }

    return bQuit;
}

void EnumFolder2(const std::wstring& folder, bool bRecursion, const std::set<std::wstring>& extFilters,
    const std::function<bool(const std::wstring& pathname)>& callback)
{
    stdfs::path base_entry(folder);
    EnumFolderEntry2(base_entry, bRecursion, extFilters, callback);
}

static bool EnumFolderEntry3(const stdfs::path& base_entry, bool bRecursion, const std::set<std::wstring>& extFiltersExclusive,
    const std::function<bool(const std::wstring& pathname)>& callback)
{
    bool bQuit = false;
    for (const auto& entry : stdfs::directory_iterator(base_entry))
    {
        if (entry.is_regular_file())
        {
            bool exclusive = false;
            if (!extFiltersExclusive.empty())
            {
                std::wstring extension = entry.path().extension().wstring();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
                exclusive = (extFiltersExclusive.find(extension) != extFiltersExclusive.end());
            }
            if (!exclusive)
            {
                if (!callback(entry.path().wstring()))
                {
                    bQuit = true;
                    break;
                }
            }

        }
        else if (entry.is_directory() && bRecursion)
        {
            if (EnumFolderEntry3(entry, bRecursion, extFiltersExclusive, callback))
            {
                bQuit = true;
                break;
            }
        }
    }

    return bQuit;
}

void EnumFolder3(const std::wstring& folder, bool bRecursion, const std::set<std::wstring>& extFiltersExclusive,
    const std::function<bool(const std::wstring& pathname)>& callback)
{
    stdfs::path base_entry(folder);
    EnumFolderEntry3(base_entry, bRecursion, extFiltersExclusive, callback);
}


std::wstring GetParentFolder(const std::wstring& pathname)
{
    stdfs::path path_entry(pathname);

    return path_entry.parent_path().wstring();
}

std::wstring GetFileStemName(const std::wstring& pathname)
{
    stdfs::path path_entry(pathname);

    return path_entry.stem().wstring();
}

std::wstring ReplaceExtension(const std::wstring& pathname, const std::wstring& extname)
{
    stdfs::path path_entry(pathname);

    path_entry.replace_extension(extname);

    return path_entry.wstring();
}

std::wstring GetFileExtensionName(const std::wstring& pathname)
{
    stdfs::path path_entry(pathname);

    return path_entry.extension().wstring();
}


stdfs::path GetApplicationPathname() 
{
#ifdef _WIN32
    // Windows 方法
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return stdfs::path(path);

#elif defined(__APPLE__)
    // macOS 方法
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return stdfs::path(path);
    }
    return stdfs::path();

#elif defined(__linux__)
    // Linux 方法
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        path[count] = '\0';
        return stdfs::path(path);
    }
    return stdfs::path();
#else
    // 其他系统返回空路径
    return fs::path();
#endif
}

std::wstring GetApplicationBasePath()
{
    stdfs::path pathname = GetApplicationPathname();

    return pathname.parent_path().wstring();
}
