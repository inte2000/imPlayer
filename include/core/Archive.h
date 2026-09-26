#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <memory>
#include <string>
#include <vector>

class CArchiveFile;
struct ArchiveState;

class CArchive
{
public:
    static constexpr std::size_t DEFAULT_WINDOW_SIZE = 4 * 1024 * 1024;

    CArchive();
    ~CArchive() = default;

    bool Open(const std::wstring& archivePath);
    void Close();
    bool IsOpen() const;
    std::vector<std::wstring> GetFileList() const;

    void SetWindowSize(std::size_t windowSizeBytes);
    std::size_t GetWindowSize() const;

    std::unique_ptr<CArchiveFile> OpenFile(const std::wstring& name);

private:
    std::shared_ptr<ArchiveState> m_state;
    std::size_t m_windowSize;
};

#endif // ARCHIVE_H
