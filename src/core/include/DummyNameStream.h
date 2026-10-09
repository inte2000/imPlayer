#ifndef DUMMY_NAME_DATA_STREAM_H
#define DUMMY_NAME_DATA_STREAM_H

#include <memory>
#include <string>

#include "DataStream.h"

class CDummyNameStream : public CDataStream
{
public:
    CDummyNameStream()
    {
        m_style = dsStyleDummyName;
    }
    ~CDummyNameStream() override { Close(); }

    bool Open(const std::wstring& filename);
    void Close() { }

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    uint32_t Write(const void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    std::size_t GetLength() const override;
    void Seek(SeekBase base, long long off) override;
    std::size_t Tell() override;
};

std::unique_ptr<CDummyNameStream> MakeDummyNameStream(const std::wstring& filename);

#endif // DUMMY_NAME_DATA_STREAM_H
