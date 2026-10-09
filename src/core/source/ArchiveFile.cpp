#include <algorithm>

#include <archive_entry.h>
#include "ArchiveFile.h"
//#include "LibarchiveApi.h"
#include "UnicodeConvert.h"

constexpr std::size_t SKIP_CHUNK_SIZE = 256 * 1024;

archive* OpenArchiveHandle(const std::wstring& archivePath)
{
    archive* handle = archive_read_new();
    if (handle == nullptr) {
        return nullptr;
    }

    if (archive_read_support_filter_all(handle) != ARCHIVE_OK) {
        archive_read_free(handle);
        return nullptr;
    }
    if (archive_read_support_format_all(handle) != ARCHIVE_OK) {
        archive_read_free(handle);
        return nullptr;
    }

    if (archive_read_open_filename_w(handle, archivePath.c_str(), 10240) != ARCHIVE_OK) {
        archive_read_free(handle);
        return nullptr;
    }

    return handle;
}

bool SkipBytesSequential(archive* archiveHandle, std::uint64_t bytesToSkip)
{
    if (archiveHandle == nullptr) {
        return false;
    }

    std::vector<uint8_t> skipBuffer(SKIP_CHUNK_SIZE);
    std::uint64_t remaining = bytesToSkip;
    while (remaining > 0)
    {
        const std::uint64_t onceSkip = std::min<std::uint64_t>(remaining, skipBuffer.size());
        const long long readed = archive_read_data(archiveHandle, skipBuffer.data(), onceSkip);
        if (readed < 0)
        {
            //const char* error = archive_error_string(archiveHandle);
            //const int errorCode = archive_errno(archiveHandle);

            return false;
        }

        if (readed == 0 || readed == ARCHIVE_EOF)
            break;

        remaining -= readed;
    }

    return (remaining == 0);
}

bool FindEntryByUtf8Name(archive* archiveHandle, const std::string& entryNameUtf8, archive_entry** entryOut)
{
    if ((archiveHandle == nullptr) || (entryOut == nullptr)) {
        return false;
    }

    archive_entry* entry = nullptr;
    while (archive_read_next_header(archiveHandle, &entry) == ARCHIVE_OK)
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

CArchiveFile::CArchiveFile(archive* archiveHandle, const std::wstring& archiveName, const std::string& utf8Name, std::uint64_t entrySize, std::uint32_t windowSize)
    : m_archiveHandle(archiveHandle)
    , m_archiveName(archiveName)
    , m_entryNameUtf8(utf8Name)
    , m_entrySize(entrySize)
    , m_curPos(0)
    , m_windowSize(windowSize)
    , m_windowBegin(0)
    , m_windowEnd(0)
{
}

CArchiveFile::~CArchiveFile()
{
    if (m_archiveHandle)
    {
        archive_read_free(m_archiveHandle);
        m_archiveHandle = nullptr;
    }
}

archive* CArchiveFile::RestartArchive()
{
    archive* newArchive = OpenArchiveHandle(m_archiveName);
    if (newArchive)
    {
        archive_entry* entry = nullptr;
        if (FindEntryByUtf8Name(newArchive, m_entryNameUtf8, &entry))
        {
            std::int64_t fileSize = archive_entry_size(entry);
            if (m_entrySize == fileSize)
                return newArchive;
        }

        archive_read_free(newArchive);
    }

    return nullptr;
}

bool CArchiveFile::FillWindowAt(std::uint64_t windowBegin)
{
    if (m_archiveHandle == nullptr)
        return false;

    if (windowBegin >= m_entrySize) {
        m_windowBuffer.clear();
        m_windowBegin = m_entrySize;
        m_windowEnd = m_entrySize;
        return true;
    }

    std::uint64_t bytesToSkip = 0;
    if (windowBegin >= m_windowEnd) //向前推进，不需要重新定位 m_archiveHandle
    {
        bytesToSkip = windowBegin - m_windowEnd;
    }
    else //windowBegin < m_windowBegin，其他情况在这之前就过滤掉了
    {
        bytesToSkip = windowBegin;
        archive* renew = RestartArchive();
        if (renew == nullptr)
            return false;

        archive_read_free(m_archiveHandle);
        m_archiveHandle = renew;
    }

    if (!SkipBytesSequential(m_archiveHandle, bytesToSkip)) {
        return false;
    }

    const std::uint64_t bytesToRead = std::min<std::uint64_t>(m_windowSize, m_entrySize - windowBegin);
    m_windowBuffer.assign(bytesToRead, 0);
    std::uint64_t totalRead = 0;
    while (totalRead < bytesToRead)
    {
        const long long readed = archive_read_data(m_archiveHandle, m_windowBuffer.data() + totalRead, bytesToRead - totalRead);
        if (readed < 0) {
            m_windowBuffer.clear();
            return false;
        }
        if (readed == 0) {
            break;
        }
        totalRead += static_cast<std::size_t>(readed);
    }
    m_windowBuffer.resize(totalRead);

    m_windowBegin = windowBegin;
    m_windowEnd = m_windowBegin + totalRead;
    return true;
}

bool CArchiveFile::EnsureWindowContains(std::uint64_t pos)
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

    const std::uint64_t newBegin = (pos / m_windowSize) * m_windowSize;
    if (!FillWindowAt(newBegin)) {
        return false;
    }

    return pos >= m_windowBegin && pos < m_windowEnd;
}

std::uint32_t CArchiveFile::Read(void* pBuf, std::uint32_t size, std::uint32_t timeout)
{
    (void)timeout;
    if (pBuf == nullptr || size == 0 || m_curPos >= m_entrySize) {
        return 0;
    }

    uint8_t* outBuf = static_cast<uint8_t*>(pBuf);
    std::uint64_t copied = 0;
    std::uint64_t remaining = std::min<std::uint64_t>(size, m_entrySize - m_curPos);

    while (remaining > 0)
    {
        if (!EnsureWindowContains(m_curPos)) {
            break;
        }
        if (m_curPos >= m_windowEnd) {
            break;
        }

        const std::uint64_t windowOffset = m_curPos - m_windowBegin;
        const std::uint64_t available = m_windowEnd - m_curPos;
        const std::uint64_t onceRead = std::min<std::uint64_t>(available, remaining);
        if (onceRead == 0) {
            break;
        }

        std::memcpy(outBuf + copied, m_windowBuffer.data() + windowOffset, onceRead);

        copied += onceRead;
        remaining -= onceRead;
        m_curPos += onceRead;
    }

    return static_cast<std::uint32_t>(copied);
}

std::uint64_t CArchiveFile::GetLength() const
{
    return m_entrySize;
}

void CArchiveFile::Seek(std::uint64_t off)
{
    const std::uint64_t target = std::min<std::uint64_t>(off, m_entrySize);

    if (!m_windowBuffer.empty()) {
        if (target >= m_windowBegin && target <= m_windowEnd) {
            m_curPos = target;
            return;
        }
    }

    if (target <= m_entrySize) {
        const std::uint64_t newBegin = (target / m_windowSize) * m_windowSize;
        if (FillWindowAt(newBegin)) {
            m_curPos = target;
        }
        return;
    }

    m_curPos = target;
}

std::uint64_t CArchiveFile::Tell() const
{
    return m_curPos;
}
