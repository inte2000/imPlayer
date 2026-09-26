#include <array>

#include <catch2/catch_test_macros.hpp>

#include "MemBufStream.h"
#include "WavDecoder.h"

namespace {

std::array<uint8_t, 44> MakeMinimalWavHeader()
{
    return {
        'R','I','F','F',
        36,0,0,0,
        'W','A','V','E',
        'f','m','t',' ',
        16,0,0,0,
        1,0,
        2,0,
        0x44,0xAC,0x00,0x00,
        0x10,0xB1,0x02,0x00,
        4,0,
        16,0,
        'd','a','t','a',
        0,0,0,0
    };
}

}

TEST_CASE("WavQueryFileType detects WAV from seekable stream and restores cursor", "[decoder][wav][stream]")
{
    CMemoryBufStream stream(false);
    REQUIRE(stream.Open(64));

    const auto header = MakeMinimalWavHeader();
    REQUIRE(stream.Write(header.data(), static_cast<uint32_t>(header.size())) == header.size());
    stream.Seek(SeekBase::Begin, 8);

    const std::size_t oldPos = stream.Tell();
    CHECK(WavQueryFileType(L"", &stream) == StreamFormatWav);
    CHECK(stream.Tell() == oldPos);
}
