#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include "AudioInfo.h"
#include "GmeFunc.h"
#include "MemBufStream.h"

namespace {

std::array<uint8_t, 128> MakeMinimalNsfHeader()
{
    std::array<uint8_t, 128> header = {};
    header[0] = 'N';
    header[1] = 'E';
    header[2] = 'S';
    header[3] = 'M';
    header[4] = 0x1A;
    header[5] = 0x01;
    return header;
}

std::filesystem::path WriteProbeNsfFile()
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "implayer_gme_parser_probe.nsf";
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    const auto header = MakeMinimalNsfHeader();
    file.write(reinterpret_cast<const char*>(header.data()), static_cast<std::streamsize>(header.size()));
    return path;
}

void FillProbeStream(CMemoryBufStream& stream)
{
    const auto header = MakeMinimalNsfHeader();
    if (!stream.Open(256)) {
        throw std::runtime_error("failed to open memory stream");
    }
    if (stream.Write(header.data(), static_cast<uint32_t>(header.size())) != header.size()) {
        throw std::runtime_error("failed to seed memory stream");
    }
}

}

TEST_CASE("GmeStreamFmtByName maps known identifiers", "[decoder][gme][name]")
{
    SetGmeCustomFormatBase(StreamFormatPlusBegin);

    CHECK(GmeStreamFmtByName("NSF") == GmeFormatNsf());
    CHECK(GmeStreamFmtByName("VGM") == GmeFormatVgm());
    CHECK(GmeStreamFmtByName("VGZ") == GmeFormatVgm());
    CHECK(GmeStreamFmtByName("UNKNOWN") == StreamFormatUnknown);
}

TEST_CASE("ParseStreamFormatByGmeStream detects NSF from seekable stream and restores cursor", "[decoder][gme][stream]")
{
    SetGmeCustomFormatBase(StreamFormatPlusBegin);

    CMemoryBufStream stream(false);
    FillProbeStream(stream);
    stream.Seek(SeekBase::Begin, 11);

    const std::size_t oldPos = stream.Tell();
    CHECK(ParseStreamFormatByGmeStream(&stream) == GmeFormatNsf());
    CHECK(stream.Tell() == oldPos);
}

TEST_CASE("ParseStreamFormatByGmeFile detects NSF from file", "[decoder][gme][file]")
{
    SetGmeCustomFormatBase(StreamFormatPlusBegin);

    const std::filesystem::path probeFile = WriteProbeNsfFile();
    CHECK(ParseStreamFormatByGmeFile(probeFile.string().c_str()) == GmeFormatNsf());

    std::error_code ec;
    std::filesystem::remove(probeFile, ec);
}
