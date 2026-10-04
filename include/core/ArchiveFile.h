#ifndef ARCHIVE_FILE_H
#define ARCHIVE_FILE_H

#include <memory>
#include <string>
#include <vector>
#include <archive.h>

archive* OpenArchiveHandle(const std::wstring& archivePath);

class CArchiveFile
{
public:
    CArchiveFile(archive* archiveHandle, const std::wstring& archiveName, const std::string& utf8Name, std::uint64_t entrySize, std::uint32_t windowSize);
    ~CArchiveFile();

    std::uint32_t Read(void* pBuf, std::uint32_t size, std::uint32_t timeout = 0);
    std::uint64_t GetLength() const;
    void Seek(std::uint64_t off);
    std::uint64_t Tell() const;

private:
    archive* RestartArchive();
    bool FillWindowAt(std::uint64_t windowBegin);
    bool EnsureWindowContains(std::uint64_t pos);

    archive* m_archiveHandle;
    std::wstring m_archiveName;
    std::string m_entryNameUtf8;
    std::uint64_t m_entrySize;
    std::uint64_t m_curPos;
    std::uint64_t m_windowSize;
    std::uint64_t m_windowBegin;
    std::uint64_t m_windowEnd;
    std::vector<std::uint8_t> m_windowBuffer;
};

#endif // ARCHIVE_FILE_H
