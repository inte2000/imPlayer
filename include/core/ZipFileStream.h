#ifndef ZIP_FILE_STREAM_H
#define ZIP_FILE_STREAM_H

#include <memory>
#include <string>

#include "Archive.h"
#include "ArchiveFile.h"
#include "DataStream.h"
#include "StreamMateSource.h"

class CZipFileStream : public CDataStream, public MateSource
{
public:
    CZipFileStream(bool bReadOnly = true)
        : m_bReadOnly(bReadOnly)
    {
        m_style = dsStyleFixedLength | dsStyleSeekable | dsStyleTellPos;
        if (!bReadOnly) {
            m_style |= dsStyleWritable;
        }
    }

    ~CZipFileStream() override { Close(); }

    bool Open(const std::wstring& archiveFile, const std::wstring& zipFile);
    void Close();

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    uint32_t Write(const void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    std::size_t GetLength() const override;
    void Seek(SeekBase base, long long off) override;
    std::size_t Tell() override;
    std::unique_ptr<CDataStream> CreateMateStream(const wchar_t* name) override;

private:
    bool m_bReadOnly;
    std::wstring m_archiveFilePath;
    std::wstring m_zipFilePath;
    CArchive m_archive;
    std::unique_ptr<CArchiveFile> m_file;
};

std::unique_ptr<CDataStream> MakeZipFileStream(const std::wstring& archiveFile, const std::wstring& zipFile, bool bReadOnly);

#endif // ZIP_FILE_STREAM_H
