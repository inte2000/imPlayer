#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <chrono>

#include "FileStream.h"
#include "MemBufStream.h"
#include "StreamMateSource.h"

namespace {

std::filesystem::path CreateTestDir()
{
    auto dir = std::filesystem::temp_directory_path() / "iPlayer_FileStreamMateSourceTests";
    std::filesystem::create_directories(dir);
    return dir;
}

void WriteFile(const std::filesystem::path& path, const char* data)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out.is_open());
    out.write(data, static_cast<std::streamsize>(std::char_traits<char>::length(data)));
}

} // namespace

TEST_CASE("File stream errors preserve UTF-8 filenames", "[core][stream][encoding]")
{
    const auto uniqueId = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("implayer-missing-" + std::to_string(uniqueId))
        / u8"“Ù¿÷.wav";
    const auto utf8Path = path.u8string();
    const std::string filename(reinterpret_cast<const char*>(utf8Path.data()), utf8Path.size());
    REQUIRE_FALSE(std::filesystem::exists(path));
    REQUIRE_THROWS_WITH(MakeFileStream(path.wstring(), true), "Fail to open file: " + filename);
}

TEST_CASE("CFileStream supports MateSource create", "[core][stream][mate]")
{
    const auto testDir = CreateTestDir();
    const auto sourcePath = testDir / "track.wv";
    const auto matePath = testDir / "track.wvc";

    WriteFile(sourcePath, "wvdata");
    WriteFile(matePath, "wvcdata");

    CFileStream sourceStream(true);
    REQUIRE(sourceStream.Open(sourcePath.wstring()));

    auto* mateSource = sourceStream.QuerySource<MateSource>();
    REQUIRE(mateSource != nullptr);

    std::unique_ptr<CDataStream> mateStream = mateSource->CreateMateStream(L"track.wvc");
    REQUIRE(mateStream != nullptr);
    CHECK(mateStream->GetName() == matePath.wstring());

    char readBuf[4] = {};
    CHECK(mateStream->Read(readBuf, static_cast<uint32_t>(sizeof(readBuf))) == sizeof(readBuf));

    mateStream.reset();
    sourceStream.Close();
    std::filesystem::remove(matePath);
    std::filesystem::remove(sourcePath);
    std::filesystem::remove(testDir);
}

TEST_CASE("CMemoryBufStream does not expose MateSource", "[core][stream][mate]")
{
    CMemoryBufStream memStream(true);
    CHECK(memStream.QuerySource<MateSource>() == nullptr);
}
