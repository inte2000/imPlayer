#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "ArchiveFile.h"
#include "ArchiveState.h"

namespace {

constexpr std::size_t SKIP_CHUNK_SIZE = 16 * 1024;

}

CArchiveFile::CArchiveFile(std::shared_ptr<ArchiveState> state, std::string entryNameUtf8, std::size_t entrySize)
    : m_state(std::move(state))
    , m_entryNameUtf8(std::move(entryNameUtf8))
    , m_entrySize(entrySize)
    , m_curPos(0)
{
}

bool CArchiveFile::ReadChunk(std::size_t offset, void* buf, std::size_t size, std::size_t* readSize) const
{
    if (m_state == nullptr || buf == nullptr || readSize == nullptr) {
        return false;
    }

    *readSize = 0;
    if (size == 0) {
        return true;
    }

    std::lock_guard<std::mutex> lock(m_state->m_mutex);
    if (m_state->m_archive == nullptr) {
        return false;
    }
    if (!ar_parse_entry_for(m_state->m_archive, m_entryNameUtf8.c_str())) {
        return false;
    }

    const std::size_t entrySize = ar_entry_get_size(m_state->m_archive);
    if (offset >= entrySize) {
        return true;
    }

    std::size_t skipBytes = offset;
    std::vector<uint8_t> skipBuffer(SKIP_CHUNK_SIZE);
    while (skipBytes > 0)
    {
        const std::size_t onceSkip = std::min<std::size_t>(skipBytes, skipBuffer.size());
        if (!ar_entry_uncompress(m_state->m_archive, skipBuffer.data(), onceSkip)) {
            return false;
        }
        skipBytes -= onceSkip;
    }

    const std::size_t canRead = std::min<std::size_t>(size, entrySize - offset);
    if (!ar_entry_uncompress(m_state->m_archive, buf, canRead)) {
        return false;
    }

    *readSize = canRead;
    return true;
}

uint32_t CArchiveFile::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)timeout;
    if (pBuf == nullptr || size == 0 || m_curPos >= m_entrySize) {
        return 0;
    }

    std::size_t readSize = 0;
    if (!ReadChunk(m_curPos, pBuf, size, &readSize)) {
        return 0;
    }

    m_curPos += readSize;
    return static_cast<uint32_t>(readSize);
}

std::size_t CArchiveFile::GetLength() const
{
    return m_entrySize;
}

void CArchiveFile::Seek(int base, long long off)
{
    const bool bNegLeft = (off < 0);
    const std::size_t poff = static_cast<std::size_t>(std::abs(off));

    std::size_t newpos = 0;
    if (base == SEEK_SET) {
        newpos = bNegLeft ? 0 : poff;
    }
    else if (base == SEEK_END) {
        newpos = bNegLeft ? (m_entrySize > poff ? m_entrySize - poff : 0) : m_entrySize;
    }
    else {
        if (bNegLeft) {
            newpos = (m_curPos > poff) ? (m_curPos - poff) : 0;
        }
        else {
            newpos = m_curPos + poff;
        }
    }

    m_curPos = std::min<std::size_t>(newpos, m_entrySize);
}

std::size_t CArchiveFile::Tell() const
{
    return m_curPos;
}
