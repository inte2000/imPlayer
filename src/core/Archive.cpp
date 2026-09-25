#include <algorithm>
#include <vector>
#include <cstdio>

#include "Archive.h"
#include "ArchiveFile.h"
#include "ArchiveState.h"
#include "UnicodeConvert.h"

namespace {

constexpr std::size_t SKIP_CHUNK_SIZE = 16 * 1024;

}

ArchiveState::ArchiveState()
    : m_stream(nullptr)
    , m_archive(nullptr)
{
}

ArchiveState::~ArchiveState()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_archive != nullptr) {
        ar_close_archive(m_archive);
        m_archive = nullptr;
    }
    if (m_stream != nullptr) {
        ar_close(m_stream);
        m_stream = nullptr;
    }
}

bool ArchiveState::OpenArchiveFromStream()
{
    if (m_stream == nullptr) {
        return false;
    }

    ar_seek(m_stream, 0, SEEK_SET);
    m_archive = ar_open_rar_archive(m_stream);
    if (m_archive != nullptr) {
        return true;
    }

    ar_seek(m_stream, 0, SEEK_SET);
    m_archive = ar_open_zip_archive(m_stream, false);
    if (m_archive != nullptr) {
        return true;
    }

    ar_seek(m_stream, 0, SEEK_SET);
    m_archive = ar_open_7z_archive(m_stream);
    if (m_archive != nullptr) {
        return true;
    }

    ar_seek(m_stream, 0, SEEK_SET);
    m_archive = ar_open_tar_archive(m_stream);
    return m_archive != nullptr;
}

bool ArchiveState::Open(const std::wstring& archivePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_archive != nullptr) {
        ar_close_archive(m_archive);
        m_archive = nullptr;
    }
    if (m_stream != nullptr) {
        ar_close(m_stream);
        m_stream = nullptr;
    }

    m_stream = ar_open_file_w(archivePath.c_str());
    if (m_stream == nullptr) {
        return false;
    }

    if (!OpenArchiveFromStream()) {
        ar_close(m_stream);
        m_stream = nullptr;
        return false;
    }

    return true;
}

bool ArchiveState::IsOpen() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_archive != nullptr;
}

bool ArchiveState::QueryEntrySize(const std::string& entryNameUtf8, std::size_t* entrySize)
{
    if (entrySize == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_archive == nullptr) {
        return false;
    }

    if (!ar_parse_entry_for(m_archive, entryNameUtf8.c_str())) {
        return false;
    }

    *entrySize = ar_entry_get_size(m_archive);
    return true;
}

bool ArchiveState::ReadEntryChunk(const std::string& entryNameUtf8, std::size_t offset, void* buf, std::size_t size, std::size_t* readSize)
{
    if (readSize == nullptr || buf == nullptr) {
        return false;
    }

    *readSize = 0;
    if (size == 0) {
        return true;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_archive == nullptr) {
        return false;
    }

    if (!ar_parse_entry_for(m_archive, entryNameUtf8.c_str())) {
        return false;
    }

    const std::size_t entrySize = ar_entry_get_size(m_archive);
    if (offset >= entrySize) {
        return true;
    }

    std::size_t skipBytes = offset;
    std::vector<uint8_t> skipBuffer(SKIP_CHUNK_SIZE);
    while (skipBytes > 0)
    {
        const std::size_t onceSkip = std::min<std::size_t>(skipBytes, skipBuffer.size());
        if (!ar_entry_uncompress(m_archive, skipBuffer.data(), onceSkip)) {
            return false;
        }
        skipBytes -= onceSkip;
    }

    const std::size_t canRead = std::min<std::size_t>(size, entrySize - offset);
    if (!ar_entry_uncompress(m_archive, buf, canRead)) {
        return false;
    }

    *readSize = canRead;
    return true;
}

bool CArchive::Open(const std::wstring& archivePath)
{
    std::shared_ptr<ArchiveState> state = std::make_shared<ArchiveState>();
    if (!state->Open(archivePath)) {
        return false;
    }
    m_state = std::move(state);
    return true;
}

void CArchive::Close()
{
    m_state.reset();
}

bool CArchive::IsOpen() const
{
    return (m_state != nullptr) && m_state->IsOpen();
}

std::unique_ptr<CArchiveFile> CArchive::OpenFile(const std::wstring& name)
{
    if (m_state == nullptr || name.empty()) {
        return nullptr;
    }

    const std::string nameUtf8 = Utf16ToUtf8(name);
    if (nameUtf8.empty()) {
        return nullptr;
    }

    std::size_t entrySize = 0;
    if (!m_state->QueryEntrySize(nameUtf8, &entrySize)) {
        return nullptr;
    }

    return std::make_unique<CArchiveFile>(m_state, name, nameUtf8, entrySize);
}
