#include <format>
#include <ranges>
#include <filesystem>
#include "StdFileSystem.h"


namespace stdfs = std::filesystem;

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
            if (!EnumFolderEntry(entry, bRecursion, extName, callback))
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
            if (!EnumFolderEntry2(entry, bRecursion, extNames, callback))
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
