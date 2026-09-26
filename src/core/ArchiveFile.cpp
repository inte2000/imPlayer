#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "ArchiveFile.h"
#include "ArchiveState.h"

namespace {

constexpr std::size_t SKIP_CHUNK_SIZE = 16 * 1024;

std::size_t ValidateWindowSize(std::size_t windowSize)
{
    if ((windowSize == 0) || ((windowSize % 4096) != 0)) {
        throw std::invalid_argument("archive window size must be a non-zero multiple of 4096");
    }

    return windowSize;
}

bool SkipBytesSequential(ar_archive* archive, std::size_t bytesToSkip)
{
    if (archive == nullptr) {
        return false;
    }

    std::vector<uint8_t> skipBuffer(SKIP_CHUNK_SIZE);
    std::size_t remaining = bytesToSkip;
    while (remaining > 0)
    {
        const std::size_t onceSkip = std::min<std::size_t>(remaining, skipBuffer.size());
        if (!ar_entry_uncompress(archive, skipBuffer.data(), onceSkip)) {
            return false;
        }
        remaining -= onceSkip;
    }

    return true;
}

}

CArchiveFile::CArchiveFile(std::shared_ptr<ArchiveState> state,
                           std::string entryNameUtf8,
                           std::size_t entrySize,
                           std::size_t windowSize)
    : m_state(std::move(state))
    , m_entryNameUtf8(std::move(entryNameUtf8))
    , m_entrySize(entrySize)
    , m_curPos(0)
    , m_windowSize(ValidateWindowSize(windowSize))
    , m_windowBegin(0)
    , m_windowEnd(0)
{
}

bool CArchiveFile::FillWindowAt(std::size_t windowBegin)
{
    if (m_state == nullptr) {
        return false;
    }

    if (windowBegin > m_entrySize) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_state->m_mutex);
    if (m_state->m_archive == nullptr) {
        return false;
    }

    if (windowBegin < m_windowBegin && m_state->m_stream != nullptr) {
        ar_seek(m_state->m_stream, 0, SEEK_SET);
    }

    if (!ar_parse_entry_for(m_state->m_archive, m_entryNameUtf8.c_str())) {
        return false;
    }

    const std::size_t entrySize = ar_entry_get_size(m_state->m_archive);
    m_entrySize = entrySize;
    if (windowBegin >= entrySize) {
        m_windowBuffer.clear();
        m_windowBegin = entrySize;
        m_windowEnd = entrySize;
        return true;
    }

    if (!SkipBytesSequential(m_state->m_archive, windowBegin)) {
        return false;
    }

    const std::size_t bytesToRead = std::min<std::size_t>(m_windowSize, entrySize - windowBegin);
    m_windowBuffer.assign(bytesToRead, 0);
    if (bytesToRead > 0 && !ar_entry_uncompress(m_state->m_archive, m_windowBuffer.data(), bytesToRead)) {
        m_windowBuffer.clear();
        return false;
    }

    m_windowBegin = windowBegin;
    m_windowEnd = m_windowBegin + bytesToRead;
    return true;
}

bool CArchiveFile::EnsureWindowContains(std::size_t pos)
{
    if (pos >= m_entrySize) {
        m_windowBuffer.clear();
        m_windowBegin = m_entrySize;
        m_windowEnd = m_entrySize;
        return true;
    }

    if (!m_windowBuffer.empty() && pos >= m_windowBegin && pos < m_windowEnd) {
        return true;
    }

    const std::size_t newBegin = (pos / m_windowSize) * m_windowSize;
    if (!FillWindowAt(newBegin)) {
        return false;
    }

    return pos >= m_windowBegin && pos < m_windowEnd;
}

uint32_t CArchiveFile::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)timeout;
    if (pBuf == nullptr || size == 0 || m_curPos >= m_entrySize) {
        return 0;
    }

    uint8_t* outBuf = static_cast<uint8_t*>(pBuf);
    std::size_t copied = 0;
    std::size_t remaining = std::min<std::size_t>(size, m_entrySize - m_curPos);

    while (remaining > 0)
    {
        if (!EnsureWindowContains(m_curPos)) {
            break;
        }
        if (m_curPos >= m_windowEnd) {
            break;
        }

        const std::size_t windowOffset = m_curPos - m_windowBegin;
        const std::size_t available = m_windowEnd - m_curPos;
        const std::size_t onceRead = std::min<std::size_t>(available, remaining);
        if (onceRead == 0) {
            break;
        }

        std::memcpy(outBuf + copied, m_windowBuffer.data() + windowOffset, onceRead);

        copied += onceRead;
        remaining -= onceRead;
        m_curPos += onceRead;
    }

    return static_cast<uint32_t>(copied);
}

std::size_t CArchiveFile::GetLength() const
{
    return m_entrySize;
}

void CArchiveFile::Seek(uint64_t off)
{
    const std::size_t target = std::min<std::size_t>(static_cast<std::size_t>(off), m_entrySize);

    if (!m_windowBuffer.empty()) {
        if (target >= m_windowBegin && target <= m_windowEnd) {
            m_curPos = target;
            return;
        }

        const std::size_t windowSize = m_windowEnd - m_windowBegin;
        if (windowSize == 0) {
            m_curPos = target;
            return;
        }

        if (target > m_windowEnd || target < m_windowBegin) {
            const std::size_t newBegin = (target / m_windowSize) * m_windowSize;
            if (FillWindowAt(newBegin)) {
                m_curPos = target;
            }
            return;
        }
    }

    if (target < m_entrySize) {
        const std::size_t newBegin = (target / m_windowSize) * m_windowSize;
        if (FillWindowAt(newBegin)) {
            m_curPos = target;
        }
        return;
    }

    m_curPos = target;
}

std::size_t CArchiveFile::Tell() const
{
    return m_curPos;
}
