#include <cstdio>
#include <string>
#include <vector>

#include "Archive.h"
#include "ArchiveFile.h"
#include "ArchiveState.h"
#include "UnicodeConvert.h"

namespace {

bool OpenArchiveFromStream(ar_stream* stream, ar_archive** archive)
{
    if (stream == nullptr || archive == nullptr) {
        return false;
    }

    *archive = nullptr;

    ar_seek(stream, 0, SEEK_SET);
    *archive = ar_open_rar_archive(stream);
    if (*archive != nullptr) {
        return true;
    }

    ar_seek(stream, 0, SEEK_SET);
    *archive = ar_open_zip_archive(stream, false);
    if (*archive != nullptr) {
        return true;
    }

    ar_seek(stream, 0, SEEK_SET);
    *archive = ar_open_7z_archive(stream);
    if (*archive != nullptr) {
        return true;
    }

    ar_seek(stream, 0, SEEK_SET);
    *archive = ar_open_tar_archive(stream);
    return *archive != nullptr;
}

}

bool CArchive::Open(const std::wstring& archivePath)
{
    if (archivePath.empty()) {
        return false;
    }

    std::shared_ptr<ArchiveState> state = std::make_shared<ArchiveState>();

    {
        std::lock_guard<std::mutex> lock(state->m_mutex);

        state->m_stream = ar_open_file_w(archivePath.c_str());
        if (state->m_stream == nullptr) {
            return false;
        }

        if (!OpenArchiveFromStream(state->m_stream, &state->m_archive)) {
            return false;
        }
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

    std::lock_guard<std::mutex> lock(m_state->m_mutex);
    if (m_state->m_archive == nullptr) {
        return names;
    }

    if (!ar_parse_entry_at(m_state->m_archive, 0)) {
        return names;
    }

    while (true)
    {
        const char* nameUtf8 = ar_entry_get_name(m_state->m_archive);
        if (nameUtf8 != nullptr && nameUtf8[0] != '\0') {
            names.push_back(UTtf8ToUtf16Le(nameUtf8));
        }

        if (!ar_parse_entry(m_state->m_archive)) {
            break;
        }
    }

    return names;
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
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        if (m_state->m_archive == nullptr) {
            return nullptr;
        }
        if (!ar_parse_entry_for(m_state->m_archive, nameUtf8.c_str())) {
            return nullptr;
        }
        entrySize = ar_entry_get_size(m_state->m_archive);
    }

    return std::make_unique<CArchiveFile>(m_state, nameUtf8, entrySize);
}
