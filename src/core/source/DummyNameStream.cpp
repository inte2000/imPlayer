#include "DummyNameStream.h"

std::unique_ptr<CDummyNameStream> MakeDummyNameStream(const std::wstring& filename)
{
    auto stream = std::make_unique<CDummyNameStream>();
    if (!stream->Open(filename)) {
        return nullptr;
    }
    return stream;
}

bool CDummyNameStream::Open(const std::wstring& filename)
{
    m_name = filename;
    return true;
}

uint32_t CDummyNameStream::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)pBuf;
    (void)size;
    (void)timeout;
    return 0;
}

uint32_t CDummyNameStream::Write(const void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)pBuf;
    (void)size;
    (void)timeout;
    return 0;
}

std::size_t CDummyNameStream::GetLength() const
{
    return 0;
}

void CDummyNameStream::Seek(SeekBase base, long long off)
{
    (void)base;
    (void)off;
}

std::size_t CDummyNameStream::Tell()
{
    return 0;
}
