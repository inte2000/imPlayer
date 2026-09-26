#ifndef ARCHIVE_FILE_H
#define ARCHIVE_FILE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct ArchiveState;

class CArchiveFile
{
public:
    static constexpr std::size_t DEFAULT_WINDOW_SIZE = 4 * 1024 * 1024;

    CArchiveFile(std::shared_ptr<ArchiveState> state,
                 std::string entryNameUtf8,
                 std::size_t entrySize,
                 std::size_t windowSize = DEFAULT_WINDOW_SIZE);
    ~CArchiveFile() = default;

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0);
    std::size_t GetLength() const;
    void Seek(uint64_t off);
    std::size_t Tell() const;
    void SetWindowSize(std::size_t windowSize);

private:
    bool FillWindowAt(std::size_t windowBegin);
    bool EnsureWindowContains(std::size_t pos);

    std::shared_ptr<ArchiveState> m_state;
    std::string m_entryNameUtf8;
    std::size_t m_entrySize;
    std::size_t m_curPos;
    std::size_t m_windowSize;
    std::size_t m_windowBegin;
    std::size_t m_windowEnd;
    std::vector<uint8_t> m_windowBuffer;
};

#endif // ARCHIVE_FILE_H
