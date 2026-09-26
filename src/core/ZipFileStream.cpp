#include <cstdint>

#include "ZipFileStream.h"

std::unique_ptr<CDataStream> MakeZipFileStream(const std::wstring& archiveFile, const std::wstring& zipFile, bool bReadOnly)
{
    auto stream = std::make_unique<CZipFileStream>(bReadOnly);
    if (!stream->Open(archiveFile, zipFile)) {
        return nullptr;
    }
    return stream;
}

bool CZipFileStream::Open(const std::wstring& archiveFile, const std::wstring& zipFile)
{
    Close();
    if (archiveFile.empty() || zipFile.empty()) {
        return false;
    }

    if (!m_archive.Open(archiveFile)) {
        return false;
    }

    m_file = m_archive.OpenFile(zipFile);
    if (!m_file) {
        m_archive.Close();
        return false;
    }

    m_archiveFilePath = archiveFile;
    m_zipFilePath = zipFile;
    m_name = archiveFile + L"|" + zipFile;
    return true;
}

void CZipFileStream::Close()
{
    m_file.reset();
    m_archive.Close();
    m_archiveFilePath.clear();
    m_zipFilePath.clear();
}

uint32_t CZipFileStream::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    if (!m_file) {
        return 0;
    }
    return m_file->Read(pBuf, size, timeout);
}

uint32_t CZipFileStream::Write(const void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)pBuf;
    (void)size;
    (void)timeout;
    return 0;
}

std::size_t CZipFileStream::GetLength() const
{
    if (!m_file) {
        return 0;
    }
    return m_file->GetLength();
}

void CZipFileStream::Seek(SeekBase base, long long off)
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

std::size_t CZipFileStream::Tell()
{
    if (!m_file) {
        return 0;
    }
    return m_file->Tell();
}

std::unique_ptr<CDataStream> CZipFileStream::CreateMateStream(const wchar_t* name)
{
    if (name == nullptr || m_archiveFilePath.empty()) {
        return nullptr;
    }
    return MakeZipFileStream(m_archiveFilePath, name, true);
}
