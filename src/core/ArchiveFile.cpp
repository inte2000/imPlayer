#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <archive_entry.h>

#include "ArchiveFile.h"
#include "ArchiveState.h"
#include "LibarchiveApi.h"
#include "UnicodeConvert.h"

namespace {

constexpr std::size_t SKIP_CHUNK_SIZE = 16 * 1024;

std::size_t ValidateWindowSize(std::size_t windowSize)
{
    if ((windowSize == 0) || ((windowSize % 4096) != 0)) {
        throw std::invalid_argument("archive window size must be a non-zero multiple of 4096");
    }

    return windowSize;
}

archive* OpenArchiveHandle(const std::wstring& archivePath)
{
    archive* handle = archive_read_new();
    if (handle == nullptr) {
        return nullptr;
    }

    if (archive_read_support_filter_all(handle) != LIBARCHIVE_OK) {
        archive_read_free(handle);
        return nullptr;
    }
    if (archive_read_support_format_all(handle) != LIBARCHIVE_OK) {
        archive_read_free(handle);
        return nullptr;
    }
    if (archive_read_open_filename_w(handle, archivePath.c_str(), 10240) != LIBARCHIVE_OK) {
        archive_read_free(handle);
        return nullptr;
    }

    return handle;
}

bool SkipBytesSequential(archive* archiveHandle, std::size_t bytesToSkip)
{
    if (archiveHandle == nullptr) {
        return false;
    }

    std::vector<uint8_t> skipBuffer(SKIP_CHUNK_SIZE);
    std::size_t remaining = bytesToSkip;
    while (remaining > 0)
    {
        const std::size_t onceSkip = std::min<std::size_t>(remaining, skipBuffer.size());
        const long long readed = archive_read_data(archiveHandle, skipBuffer.data(), onceSkip);
        if (readed <= 0 || static_cast<std::size_t>(readed) != onceSkip) {
            return false;
        }
        remaining -= onceSkip;
    }

    return true;
}

bool FindEntryByUtf8Name(archive* archiveHandle, const std::string& entryNameUtf8, archive_entry** entryOut)
{
    if ((archiveHandle == nullptr) || (entryOut == nullptr)) {
        return false;
    }

    archive_entry* entry = nullptr;
    while (archive_read_next_header(archiveHandle, &entry) == LIBARCHIVE_OK)
    {
        const char* pathUtf8 = archive_entry_pathname_utf8(entry);
        if ((pathUtf8 != nullptr) && (entryNameUtf8 == pathUtf8)) {
            *entryOut = entry;
            return true;
        }
        archive_read_data_skip(archiveHandle);
    }

    return false;
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

    std::wstring archivePath;
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        if (m_state->m_archivePath.empty()) {
            return false;
        }
        archivePath = m_state->m_archivePath;
    }

    archive* archiveHandle = OpenArchiveHandle(archivePath);
    if (archiveHandle == nullptr) {
        return false;
    }

    archive_entry* entry = nullptr;
    if (!FindEntryByUtf8Name(archiveHandle, m_entryNameUtf8, &entry)) {
        archive_read_free(archiveHandle);
        return false;
    }

    const std::size_t entrySize = static_cast<std::size_t>(std::max<la_int64_t>(0, archive_entry_size(entry)));
    m_entrySize = entrySize;
    if (windowBegin >= entrySize) {
        m_windowBuffer.clear();
        m_windowBegin = entrySize;
        m_windowEnd = entrySize;
        archive_read_free(archiveHandle);
        return true;
    }

    if (!SkipBytesSequential(archiveHandle, windowBegin)) {
        archive_read_free(archiveHandle);
        return false;
    }

    const std::size_t bytesToRead = std::min<std::size_t>(m_windowSize, entrySize - windowBegin);
    m_windowBuffer.assign(bytesToRead, 0);
    std::size_t totalRead = 0;
    while (totalRead < bytesToRead)
    {
        const long long readed = archive_read_data(archiveHandle, m_windowBuffer.data() + totalRead, bytesToRead - totalRead);
        if (readed < 0) {
            m_windowBuffer.clear();
            archive_read_free(archiveHandle);
            return false;
        }
        if (readed == 0) {
            break;
        }
        totalRead += static_cast<std::size_t>(readed);
    }
    m_windowBuffer.resize(totalRead);
    archive_read_free(archiveHandle);

    m_windowBegin = windowBegin;
    m_windowEnd = m_windowBegin + totalRead;
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
