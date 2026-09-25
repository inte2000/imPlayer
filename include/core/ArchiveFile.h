#ifndef ARCHIVE_FILE_H
#define ARCHIVE_FILE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

struct ArchiveState;

class CArchiveFile
{
public:
    CArchiveFile(std::shared_ptr<ArchiveState> state, std::string entryNameUtf8, std::size_t entrySize);
    ~CArchiveFile() = default;

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0);
    std::size_t GetLength() const;
    void Seek(int base, long long off);
    std::size_t Tell() const;

private:
    bool ReadChunk(std::size_t offset, void* buf, std::size_t size, std::size_t* readSize) const;

    std::shared_ptr<ArchiveState> m_state;
    std::string m_entryNameUtf8;
    std::size_t m_entrySize;
    std::size_t m_curPos;
};

#endif // ARCHIVE_FILE_H
