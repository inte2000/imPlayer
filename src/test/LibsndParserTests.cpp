#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include "LibsndFunc.h"
#include "MemBufStream.h"

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

std::filesystem::path WriteProbeWavFile()
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "implayer_libsnd_parser_probe.wav";
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    const auto header = MakeMinimalWavHeader();
    file.write(reinterpret_cast<const char*>(header.data()), static_cast<std::streamsize>(header.size()));
    return path;
}

void FillProbeStream(CMemoryBufStream& stream)
{
    const auto header = MakeMinimalWavHeader();
    if (!stream.Open(64)) {
        throw std::runtime_error("failed to open memory stream");
    }
    if (stream.Write(header.data(), static_cast<uint32_t>(header.size())) != header.size()) {
        throw std::runtime_error("failed to seed memory stream");
    }
}

}

TEST_CASE("ParseStreamFormatByLibsndStream detects WAV from seekable stream and restores cursor", "[decoder][libsnd][stream]")
{
    CMemoryBufStream stream(false);
    FillProbeStream(stream);
    stream.Seek(SeekBase::Begin, 10);

    const std::size_t oldPos = stream.Tell();
    CHECK(ParseStreamFormatByLibsndStream(&stream) == StreamFormatWav);
    CHECK(stream.Tell() == oldPos);
}

TEST_CASE("ParseStreamFormatByLibsndfile detects WAV from file", "[decoder][libsnd][file]")
{
    const std::filesystem::path probeFile = WriteProbeWavFile();
    CHECK(ParseStreamFormatByLibsndfile(probeFile.string().c_str()) == StreamFormatWav);

    std::error_code ec;
    std::filesystem::remove(probeFile, ec);
}
