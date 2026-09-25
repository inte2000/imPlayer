#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

#include "Archive.h"
#include "ArchiveFile.h"

namespace {

std::wstring BuildArchivePath()
{
    std::filesystem::path path = std::filesystem::path(TEST_SOURCE_ROOT)
        / "thirdparty"
        / "unarr-1.1.1"
        / "test"
        / "corpus"
        / "integration"
        / "lipsum.tar";
    return path.wstring();
}

}

TEST_CASE("CArchive opens tar archive and CArchiveFile supports stream operations", "[core][archive]")
{
    CArchive archive;
    REQUIRE(archive.Open(BuildArchivePath()));
    REQUIRE(archive.IsOpen());

    std::unique_ptr<CArchiveFile> archiveFile = archive.OpenFile(L"lorem_ipsum.txt");
    REQUIRE(archiveFile != nullptr);
    REQUIRE(archiveFile->GetLength() > 0);

    uint8_t firstBuf[32] = {};
    const uint32_t firstRead = archiveFile->Read(firstBuf, sizeof(firstBuf));
    REQUIRE(firstRead > 0);
    CHECK(archiveFile->Tell() == firstRead);

    archiveFile->Seek(SeekBase::Begin, 0);
    CHECK(archiveFile->Tell() == 0);

    uint8_t secondBuf[16] = {};
    const uint32_t secondRead = archiveFile->Read(secondBuf, sizeof(secondBuf));
    REQUIRE(secondRead > 0);

    archive.Close();
    CHECK_FALSE(archive.IsOpen());
}

TEST_CASE("CArchiveFile keeps archive state alive after CArchive destruction", "[core][archive]")
{
    std::unique_ptr<CArchiveFile> archiveFile;

    {
        CArchive archive;
        REQUIRE(archive.Open(BuildArchivePath()));

        archiveFile = archive.OpenFile(L"lorem_ipsum.txt");
        REQUIRE(archiveFile != nullptr);

        std::unique_ptr<CArchiveFile> missingFile = archive.OpenFile(L"not_exists.txt");
        CHECK(missingFile == nullptr);
    }

    uint8_t buf[24] = {};
    const uint32_t readed = archiveFile->Read(buf, sizeof(buf));
    CHECK(readed > 0);
    CHECK(archiveFile->Tell() == readed);
}
