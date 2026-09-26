#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "Archive.h"
#include "ArchiveFile.h"
#include "ZipFileStream.h"

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

    std::vector<std::wstring> fileList = archive.GetFileList();
    REQUIRE(fileList.size() == 1);
    CHECK(fileList[0] == L"lorem_ipsum.txt");

    std::unique_ptr<CArchiveFile> archiveFile = archive.OpenFile(L"lorem_ipsum.txt");
    REQUIRE(archiveFile != nullptr);
    REQUIRE(archiveFile->GetLength() > 0);

    uint8_t firstBuf[32] = {};
    const uint32_t firstRead = archiveFile->Read(firstBuf, sizeof(firstBuf));
    REQUIRE(firstRead > 0);
    CHECK(archiveFile->Tell() == firstRead);

    archiveFile->Seek(0);
    CHECK(archiveFile->Tell() == 0);

    uint8_t secondBuf[16] = {};
    const uint32_t secondRead = archiveFile->Read(secondBuf, sizeof(secondBuf));
    REQUIRE(secondRead > 0);

    archive.Close();
    CHECK_FALSE(archive.IsOpen());
}

TEST_CASE("CArchiveFile sliding window read/seek remains consistent", "[core][archive][window]")
{
    CArchive archive;
    REQUIRE(archive.Open(BuildArchivePath()));

    std::unique_ptr<CArchiveFile> baseline = archive.OpenFile(L"lorem_ipsum.txt");
    REQUIRE(baseline != nullptr);

    std::vector<uint8_t> wholeData(256, 0);
    const uint32_t wholeRead = baseline->Read(wholeData.data(), static_cast<uint32_t>(wholeData.size()));
    REQUIRE(wholeRead >= 96);

    std::unique_ptr<CArchiveFile> smallWindowFile = archive.OpenFile(L"lorem_ipsum.txt", 32);
    REQUIRE(smallWindowFile != nullptr);

    std::vector<uint8_t> blockA(48, 0);
    const uint32_t readA = smallWindowFile->Read(blockA.data(), static_cast<uint32_t>(blockA.size()));
    REQUIRE(readA == blockA.size());
    CHECK(smallWindowFile->Tell() == readA);

    std::vector<uint8_t> blockB(32, 0);
    const uint32_t readB = smallWindowFile->Read(blockB.data(), static_cast<uint32_t>(blockB.size()));
    REQUIRE(readB == blockB.size());
    CHECK(smallWindowFile->Tell() == readA + readB);

    smallWindowFile->Seek(12);
    CHECK(smallWindowFile->Tell() == 12);

    std::vector<uint8_t> seekRead(40, 0);
    const uint32_t seekReadCount = smallWindowFile->Read(seekRead.data(), static_cast<uint32_t>(seekRead.size()));
    REQUIRE(seekReadCount == seekRead.size());
    CHECK(smallWindowFile->Tell() == 12 + seekReadCount);

    for (std::size_t i = 0; i < seekReadCount; ++i)
    {
        CHECK(seekRead[i] == wholeData[12 + i]);
    }

    const uint64_t endPos = static_cast<uint64_t>(smallWindowFile->GetLength());
    smallWindowFile->Seek(endPos);
    CHECK(smallWindowFile->Tell() == endPos);

    uint8_t eofByte = 0;
    CHECK(smallWindowFile->Read(&eofByte, 1) == 0);
}

TEST_CASE("CZipFileStream reads archive entry via CArchiveFile", "[core][archive][zipstream]")
{
    std::unique_ptr<CDataStream> stream = MakeZipFileStream(BuildArchivePath(), L"lorem_ipsum.txt", true);
    REQUIRE(stream != nullptr);
    REQUIRE(stream->GetLength() > 0);

    uint8_t buf[64] = {};
    const uint32_t readed = stream->Read(buf, sizeof(buf));
    CHECK(readed > 0);

    stream->Seek(SeekBase::Begin, 0);
    CHECK(stream->Tell() == 0);
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
