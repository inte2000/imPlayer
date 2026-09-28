#include <cstdio>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <archive_entry.h>

#include "Archive.h"
#include "ArchiveFile.h"
#include "ArchiveState.h"
#include "LibarchiveApi.h"
#include "UnicodeConvert.h"

namespace {

void ValidateWindowSize(std::size_t windowSizeBytes)
{
    if ((windowSizeBytes == 0) || ((windowSizeBytes % 4096) != 0)) {
        throw std::invalid_argument("archive window size must be a non-zero multiple of 4096");
    }
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

std::wstring EntryPathToWide(archive_entry* entry)
{
    if (entry == nullptr) {
        return {};
    }

    const wchar_t* pathWide = archive_entry_pathname_w(entry);
    if (pathWide != nullptr) {
        return pathWide;
    }

    const char* pathUtf8 = archive_entry_pathname_utf8(entry);
    if ((pathUtf8 != nullptr) && (pathUtf8[0] != '\0')) {
        return UTtf8ToUtf16Le(pathUtf8);
    }

    return {};
}

std::string EntryPathToUtf8(archive_entry* entry)
{
    if (entry == nullptr) {
        return {};
    }

    const char* pathUtf8 = archive_entry_pathname_utf8(entry);
    if (pathUtf8 != nullptr) {
        return pathUtf8;
    }

    const wchar_t* pathWide = archive_entry_pathname_w(entry);
    if (pathWide != nullptr) {
        return Utf16ToUtf8(pathWide);
    }

    return {};
}

bool IsDirectoryEntry(archive_entry* entry)
{
    if (entry == nullptr) {
        return false;
    }

    if (archive_entry_filetype(entry) == AE_IFDIR) {
        return true;
    }

    const std::string pathUtf8 = EntryPathToUtf8(entry);
    return !pathUtf8.empty() && (pathUtf8.back() == '/');
}

}

CArchive::CArchive()
    : m_windowSize(DEFAULT_WINDOW_SIZE)
{
}

bool CArchive::Open(const std::wstring& archivePath)
{
    if (archivePath.empty()) {
        return false;
    }

    std::shared_ptr<ArchiveState> state = std::make_shared<ArchiveState>();

    {
        std::lock_guard<std::mutex> lock(state->m_mutex);

        state->m_archive = OpenArchiveHandle(archivePath);
        if (state->m_archive == nullptr) {
            return false;
        }
        state->m_archivePath = archivePath;
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
    if (m_state == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_state->m_mutex);
    return m_state->m_archive != nullptr;
}

std::vector<std::wstring> CArchive::GetFileList() const
{
    std::vector<std::wstring> names;
    if (m_state == nullptr) {
        return names;
    }

    std::wstring archivePath;
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        if (m_state->m_archive == nullptr) {
            return names;
        }
        archivePath = m_state->m_archivePath;
    }

    archive* archiveHandle = OpenArchiveHandle(archivePath);
    if (archiveHandle == nullptr) {
        return names;
    }

    archive_entry* entry = nullptr;
    while (archive_read_next_header(archiveHandle, &entry) == LIBARCHIVE_OK)
    {
        if (!IsDirectoryEntry(entry)) {
            std::wstring path = EntryPathToWide(entry);
            if (!path.empty()) {
                names.push_back(std::move(path));
            }
        }
        archive_read_data_skip(archiveHandle);
    }

    archive_read_free(archiveHandle);

    return names;
}

void CArchive::SetWindowSize(std::size_t windowSizeBytes)
{
    ValidateWindowSize(windowSizeBytes);
    m_windowSize = windowSizeBytes;
}

std::size_t CArchive::GetWindowSize() const
{
    return m_windowSize;
}

std::unique_ptr<CArchiveFile> CArchive::OpenFile(const std::wstring& name)
{
    if (m_state == nullptr || name.empty()) {
        return nullptr;
    }

    std::string entryNameUtf8;
    std::size_t entrySize = 0;
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        if (m_state->m_archive == nullptr) {
            return nullptr;
        }
    }

    archive* archiveHandle = OpenArchiveHandle(m_state->m_archivePath);
    if (archiveHandle == nullptr) {
        return nullptr;
    }

    archive_entry* entry = nullptr;
    while (archive_read_next_header(archiveHandle, &entry) == LIBARCHIVE_OK)
    {
        if (!IsDirectoryEntry(entry) && EntryPathToWide(entry) == name) {
            entryNameUtf8 = EntryPathToUtf8(entry);
            entrySize = static_cast<std::size_t>(std::max<la_int64_t>(0, archive_entry_size(entry)));
            break;
        }
        archive_read_data_skip(archiveHandle);
    }
    archive_read_free(archiveHandle);

    if (entryNameUtf8.empty()) {
        return nullptr;
    }

    return std::make_unique<CArchiveFile>(m_state, entryNameUtf8, entrySize, m_windowSize);
}
