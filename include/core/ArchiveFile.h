#ifndef ARCHIVE_FILE_H
#define ARCHIVE_FILE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "DataStream.h"

struct ArchiveState;

class CArchiveFile : public CDataStream
{
public:
    CArchiveFile(std::shared_ptr<ArchiveState> state, std::wstring fileName, std::string entryNameUtf8, std::size_t entrySize);
    ~CArchiveFile() override = default;

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    uint32_t Write(const void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    std::size_t GetLength() const override;
    void Seek(SeekBase base, long long off) override;
    std::size_t Tell() override;

private:
    std::shared_ptr<ArchiveState> m_state;
    std::string m_entryNameUtf8;
    std::size_t m_entrySize;
    std::size_t m_curPos;
};

#endif // ARCHIVE_FILE_H
