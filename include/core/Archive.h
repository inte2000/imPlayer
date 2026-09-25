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
    CArchive() = default;
    ~CArchive() = default;

    bool Open(const std::wstring& archivePath);
    void Close();
    bool IsOpen() const;
    std::vector<std::wstring> GetFileList() const;

    std::unique_ptr<CArchiveFile> OpenFile(const std::wstring& name);

private:
    std::shared_ptr<ArchiveState> m_state;
};

#endif // ARCHIVE_H
