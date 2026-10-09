#include <cstdint>
#include "ArchivePackage.h"
#include "ArchiveFileStream.h"

std::unique_ptr<CDataStream> MakeArchiveFileStream(const std::wstring& archiveFile, const std::wstring& zipFile, bool bReadOnly)
{
    auto stream = std::make_unique<CArchiveFileStream>(bReadOnly);
    if (!stream->Open(archiveFile, zipFile)) {
        return nullptr;
    }
    return stream;
}

bool CArchiveFileStream::Open(const std::wstring& archiveFile, const std::wstring& zipFile)
{
    Close();
    if (archiveFile.empty() || zipFile.empty()) {
        return false;
    }

    CArchivePackage archive;
    if (!archive.Open(archiveFile)) {
        return false;
    }

    m_file = archive.OpenFile(zipFile);
    if (!m_file) {
        return false;
    }

    m_archiveFilePath = archiveFile;
    m_zipFilePath = zipFile;
    m_name = archiveFile + L"|" + zipFile;
    return true;
}

void CArchiveFileStream::Close()
{
    m_file.reset();
    m_archiveFilePath.clear();
    m_zipFilePath.clear();
}

uint32_t CArchiveFileStream::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    if (!m_file) {
        return 0;
    }
    return m_file->Read(pBuf, size, timeout);
}

uint32_t CArchiveFileStream::Write(const void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)pBuf;
    (void)size;
    (void)timeout;
    return 0;
}

std::size_t CArchiveFileStream::GetLength() const
{
    if (!m_file) {
        return 0;
    }
    return m_file->GetLength();
}

void CArchiveFileStream::Seek(SeekBase base, long long off)
{
    if (!m_file) {
        return;
    }

    const int64_t totalBytes = static_cast<int64_t>(m_file->GetLength());
    const int64_t curBytes = static_cast<int64_t>(m_file->Tell());
    int64_t target = curBytes;
    if (base == SeekBase::Begin) {
        target = off;
    }
    else if (base == SeekBase::Cur) {
        target = curBytes + off;
    }
    else {
        target = totalBytes + off;
    }

    if (target < 0 || target > totalBytes) {
        return;
    }

    m_file->Seek(static_cast<uint64_t>(target));
}

std::size_t CArchiveFileStream::Tell()
{
    if (!m_file) {
        return 0;
    }
    return m_file->Tell();
}

std::unique_ptr<CDataStream> CArchiveFileStream::CreateMateStream(const wchar_t* name)
{
    if (name == nullptr || m_archiveFilePath.empty()) {
        return nullptr;
    }
    return MakeArchiveFileStream(m_archiveFilePath, name, true);
}
