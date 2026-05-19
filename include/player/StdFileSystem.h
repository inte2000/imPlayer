#ifndef STD_FILE_SYSTEM_H
#define STD_FILE_SYSTEM_H

#include <string>
#include <vector>
#include <set>
#include <functional>

void EnumFolder(const std::wstring& folder, bool bRecursion, const std::wstring& extFilter,
    const std::function<bool(const std::wstring& pathname)>& callback);
void EnumFolder2(const std::wstring& folder, bool bRecursion, const std::set<std::wstring>& extFilters,
    const std::function<bool(const std::wstring& pathname)>& callback);

#if 0
template<typename Callback, typename... Args>
void EnumFolderWithArgs(Callback&& callback, Args&&... args) {
    std::forward<Callback>(callback)("file1.txt", std::forward<Args>(args)...);
    std::forward<Callback>(callback)("file2.jpg", std::forward<Args>(args)...);
    std::forward<Callback>(callback)("file3.png", std::forward<Args>(args)...);
}
// C++20 Concept 定义
template<typename F, typename... Args>
concept InvocableWithWstring = requires(F f, Args... args) {
    { f(std::wstring{}, args...) } -> std::same_as<void>;
};

// 使用 Concept 约束
template<typename Callback>
requires InvocableWithWstring<Callback>
void EnumFolder(const std::wstring& folderPath, Callback&& callback) {
    callback(L"file1.txt");
    callback(L"file2.jpg");
}

// 或者更简洁的版本
template<typename Callback>
void EnumFolder(const std::wstring& folderPath, Callback&& callback)
requires std::invocable<Callback, std::wstring>
{
    callback(L"file1.txt");
    callback(L"file2.jpg");
}
#endif

std::wstring GetParentFolder(const std::wstring& pathname);
std::wstring GetFileStemName(const std::wstring& pathname);

#endif //STD_FILE_SYSTEM_H
