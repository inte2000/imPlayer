#ifndef ARCHIVE_PACKAGE_H
#define ARCHIVE_PACKAGE_H

#include <memory>
#include <string>
#include <vector>

class CArchiveFile;

class CArchivePackage
{
public:
    static constexpr std::uint32_t DEFAULT_WINDOW_SIZE = 4 * 1024 * 1024;

    CArchivePackage();
    ~CArchivePackage() = default;

    bool Open(const std::wstring& archivePath);
    void Close();
    bool IsOpen() const;
    std::vector<std::wstring> GetFileList() const;

    void SetWindowSize(std::uint32_t windowSizeBytes);
    std::uint32_t GetWindowSize() const;

    std::unique_ptr<CArchiveFile> OpenFile(const std::wstring& name);

private:
    std::uint32_t m_windowSize;
    std::wstring m_archivePath;
};

#endif // ARCHIVE_PACKAGE_H
