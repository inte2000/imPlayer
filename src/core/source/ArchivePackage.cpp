#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include <archive.h>
#include <archive_entry.h>
#include "StringEx.h"
#include "ArchivePackage.h"
#include "ArchiveFile.h"
//#include "LibarchiveApi.h"
#include "UnicodeConvert.h"
#include "ScopeGuard.h"

namespace {

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

CArchivePackage::CArchivePackage() : m_windowSize(DEFAULT_WINDOW_SIZE)
{
}

bool CArchivePackage::Open(const std::wstring& archivePath)
{
    if (!archivePath.empty() && std::filesystem::exists(archivePath))
    {
        m_archivePath = archivePath;
        return true;
    }

    m_archivePath.clear();
    return false;
}

void CArchivePackage::Close()
{
    m_archivePath.clear();
}

bool CArchivePackage::IsOpen() const
{
    return !m_archivePath.empty();
}

std::vector<std::wstring> CArchivePackage::GetFileList() const
{
    std::vector<std::wstring> names;
    if (!m_archivePath.empty())
    {
        archive* archiveHandle = OpenArchiveHandle(m_archivePath);
        if (archiveHandle != nullptr) 
        {
            CScopeGuard guatd_archive([&archiveHandle]() { archive_read_free(archiveHandle); });

            archive_entry* entry = nullptr;
            while (archive_read_next_header(archiveHandle, &entry) == ARCHIVE_OK)
            {
                if (archive_entry_filetype(entry) == AE_IFREG)
                {
                    std::string path = archive_entry_pathname_utf8(entry);
                    if (!path.empty()) {
                        names.push_back(UTtf8ToUtf16Le(path));
                    }
                }
                archive_read_data_skip(archiveHandle);
            }
        }
    }

    return names;
}

void CArchivePackage::SetWindowSize(std::uint32_t windowSizeBytes)
{
    if ((windowSizeBytes == 0) || ((windowSizeBytes % 4096) != 0)) {
        windowSizeBytes = DEFAULT_WINDOW_SIZE;
    }

    m_windowSize = windowSizeBytes;
}

std::uint32_t CArchivePackage::GetWindowSize() const
{
    return m_windowSize;
}

std::unique_ptr<CArchiveFile> CArchivePackage::OpenFile(const std::wstring& name)
{
    if (m_archivePath.empty())
        return nullptr;

    archive* archiveHandle = OpenArchiveHandle(m_archivePath);
    if (archiveHandle == nullptr) 
        return nullptr;

    CScopeGuard guatd_archive([&archiveHandle]() { archive_read_free(archiveHandle); });

    std::string utf8Name = Utf16ToUtf8(name);
    archive_entry* entry = nullptr;
    while (archive_read_next_header(archiveHandle, &entry) == ARCHIVE_OK)
    {
        if (archive_entry_filetype(entry) == AE_IFREG)
        {
            std::string path = archive_entry_pathname_utf8(entry);
            if (_stricmp(path.c_str(), utf8Name.c_str()) == 0)
            {
                std::int64_t fileSize = archive_entry_size(entry);
                guatd_archive.Dismiss();
                return std::make_unique<CArchiveFile>(archiveHandle, m_archivePath, utf8Name, fileSize, m_windowSize);
            }
        }
        archive_read_data_skip(archiveHandle);
    }

    return nullptr;
}
