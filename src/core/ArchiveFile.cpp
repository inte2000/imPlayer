#include <algorithm>
#include <cstdlib>

#include "ArchiveFile.h"
#include "ArchiveState.h"

CArchiveFile::CArchiveFile(std::shared_ptr<ArchiveState> state, std::wstring fileName, std::string entryNameUtf8, std::size_t entrySize)
    : m_state(std::move(state))
    , m_entryNameUtf8(std::move(entryNameUtf8))
    , m_entrySize(entrySize)
    , m_curPos(0)
{
    m_name = std::move(fileName);
    m_style = dsStyleFixedLength | dsStyleSeekable | dsStyleTellPos;
}

uint32_t CArchiveFile::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)timeout;
    if (m_state == nullptr || pBuf == nullptr || size == 0) {
        return 0;
    }
    if (m_curPos >= m_entrySize) {
        return 0;
    }

    std::size_t readSize = 0;
    if (!m_state->ReadEntryChunk(m_entryNameUtf8, m_curPos, pBuf, size, &readSize)) {
        return 0;
    }

    m_curPos += readSize;
    return static_cast<uint32_t>(readSize);
}

uint32_t CArchiveFile::Write(const void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)pBuf;
    (void)size;
    (void)timeout;
    return 0;
}

std::size_t CArchiveFile::GetLength() const
{
    return m_entrySize;
}

void CArchiveFile::Seek(SeekBase base, long long off)
{
    const bool bNegLeft = (off < 0);
    const std::size_t poff = static_cast<std::size_t>(std::abs(off));

    std::size_t newpos = 0;
    if (base == SeekBase::Begin) {
        newpos = bNegLeft ? 0 : poff;
    }
    else if (base == SeekBase::End) {
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

std::size_t CArchiveFile::Tell()
{
    return m_curPos;
}
