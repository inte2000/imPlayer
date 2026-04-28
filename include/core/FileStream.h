#ifndef FILE_DATA_STREAM_H
#define FILE_DATA_STREAM_H

#include <string>
#include <fstream>
#include "DataStream.h"

class CFileStream : public CDataStream
{
public:
    CFileStream(bool bReadOnly = true) {
        m_type = dsTypeFixedLength | dsTypeSeekable | dsTypeTellPos;
        if (!bReadOnly)
            m_type |= dsTypeWritable;
        m_length = 0;
        m_curPos = 0;
    }
    ~CFileStream() { Close(); }

    bool Open(const std::wstring& filename);
    void Close() { m_file.close(); }

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    uint32_t Write(const void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    std::size_t GetLength() const override;
    void Seek(SeekBase base, long long off) override;
    std::size_t Tell() override;
    std::unique_ptr<CDataStream> GetAccompanyStream(const std::wstring& name) const override;
    const DsMetaInfo* GetMetaInformation() const override { return nullptr; }
    void StreamControl(const CDecodeInitCtx* decodeInit) override { decodeInit = decodeInit; }
private:
    std::size_t m_length;
    std::size_t m_curPos;
    mutable std::fstream m_file;
};

std::unique_ptr<CDataStream> MakeFileStream(const std::wstring& filename, bool bReadOnly);

#endif //FILE_DATA_STREAM_H
