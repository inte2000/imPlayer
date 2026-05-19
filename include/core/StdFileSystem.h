#ifndef STD_FILE_SYSTEM_H
#define STD_FILE_SYSTEM_H

#include <string>
#include <vector>
#include <set>
#include <functional>
#include <filesystem>


namespace stdfs = std::filesystem;

void EnumFolder(const std::wstring& folder, bool bRecursion, const std::wstring& extFilter,
    const std::function<bool(const std::wstring& pathname)>& callback);
void EnumFolder2(const std::wstring& folder, bool bRecursion, const std::set<std::wstring>& extFilters,
    const std::function<bool(const std::wstring& pathname)>& callback);
void EnumFolder3(const std::wstring& folder, bool bRecursion, const std::set<std::wstring>& extFiltersExclusive,
    const std::function<bool(const std::wstring& pathname)>& callback);

std::wstring GetParentFolder(const std::wstring& pathname);
std::wstring GetFileStemName(const std::wstring& pathname);
std::wstring ReplaceExtension(const std::wstring& pathname, const std::wstring& extname);
std::wstring GetFileExtensionName(const std::wstring& pathname);

stdfs::path GetApplicationPathname();
inline stdfs::path GetApplicationBasePath() {
    stdfs::path pathname = GetApplicationPathname();
    return pathname.parent_path();
}


#endif //STD_FILE_SYSTEM_H
