#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

#include "MemBufStream.h"
#include "PluginParseHelper.h"

namespace {

class CNamedNonSeekableStream : public CDataStream
{
public:
    explicit CNamedNonSeekableStream(const std::wstring& name)
    {
        m_style = dsStyleDummyName;
        m_name = name;
    }

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0) override
    {
        (void)pBuf;
        (void)size;
        (void)timeout;
        return 0;
    }

    uint32_t Write(const void* pBuf, uint32_t size, uint32_t timeout = 0) override
    {
        (void)pBuf;
        (void)size;
        (void)timeout;
        return 0;
    }

    std::size_t GetLength() const override
    {
        return 0;
    }

    void Seek(SeekBase base, long long off) override
    {
        (void)base;
        (void)off;
    }

    std::size_t Tell() override
    {
        return 0;
    }
};

}

TEST_CASE("ParsePluginFileTypeID uses seekable stream parser and rewinds to start", "[core][plugin][parser]")
{
    CMemoryBufStream stream(false);
    REQUIRE(stream.Open(32));

    const uint8_t probe[4] = { 1, 2, 3, 4 };
    REQUIRE(stream.Write(probe, sizeof(probe)) == sizeof(probe));
    stream.Seek(SeekBase::Begin, 3);

    bool fileParserCalled = false;
    const uint32_t fmt = ParsePluginFileTypeID(&stream,
        [](CDataStream* parserStream) {
            return (parserStream->Tell() == 3) ? 1234U : StreamFormatUnknown;
        },
        [&fileParserCalled](const char* filenameUtf8) {
            (void)filenameUtf8;
            fileParserCalled = true;
            return StreamFormatUnknown;
        });

    CHECK(fmt == 1234U);
    CHECK_FALSE(fileParserCalled);
    CHECK(stream.Tell() == 0);
}

TEST_CASE("ParsePluginFileTypeID falls back to stream name for non-seekable stream", "[core][plugin][parser]")
{
    CNamedNonSeekableStream stream(L"E:\\Music\\probe.flac");

    bool streamParserCalled = false;
    std::string parsedName;
    const uint32_t fmt = ParsePluginFileTypeID(&stream,
        [&streamParserCalled](CDataStream* parserStream) {
            (void)parserStream;
            streamParserCalled = true;
            return StreamFormatUnknown;
        },
        [&parsedName](const char* filenameUtf8) {
            parsedName = filenameUtf8;
            return 5678U;
        });

    CHECK(fmt == 5678U);
    CHECK_FALSE(streamParserCalled);
    CHECK(parsedName == "E:\\Music\\probe.flac");
}
