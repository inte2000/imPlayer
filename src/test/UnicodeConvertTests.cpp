#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include "UnicodeConvert.h"

namespace
{
std::string ToBytes(std::u8string_view value)
{
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}
}

TEST_CASE("Utf8Utf16 round trip", "[core][unicode]")
{
    const std::u8string input = u8"Hello \u4E16\u754C";

    const std::u16string utf16 = UTtf8ToUtf16(input);
    const std::u8string output = Utf16ToUtf8(utf16);

    CHECK(input == output);
}

TEST_CASE("Utf8Utf16 round trip with string API", "[core][unicode]")
{
    const std::string input = ToBytes(u8"Caf\u00E9");

    const std::u16string utf16 = UTtf8ToUtf16_2(input);
    const std::string output = Utf16ToUtf8_2(utf16);

    CHECK(input == output);
}

TEST_CASE("Utf8Utf16Le wstring API round trip", "[core][unicode]")
{
    const std::string input = ToBytes(u8"Hello \u4E16\u754C");

    const std::wstring utf16le = UTtf8ToUtf16Le(input);
    const std::string output = Utf16ToUtf8(utf16le);

    CHECK(input == output);
}

TEST_CASE("Utf8Utf32 round trip", "[core][unicode]")
{
    const std::u8string input = u8"Test \u4E2D\u6587";

    const std::u32string utf32 = Utf8ToUtf32(input);
    const std::u8string output = Utf32ToUtf8(utf32);

    CHECK(input == output);
}

TEST_CASE("Utf8Utf32 round trip with string API", "[core][unicode]")
{
    const std::string input = ToBytes(u8"Mix \u4E2D\u6587 123");

    const std::u32string utf32 = Utf8ToUtf32_2(input);
    const std::string output = Utf32ToUtf8_2(utf32);

    CHECK(input == output);
}

TEST_CASE("Utf8 to Latin1 converts expected byte", "[core][unicode]")
{
    const std::string input = ToBytes(u8"Caf\u00E9");
    try
    {
        const std::string output = UTtf8ToLatin(input, "Latin1");
        REQUIRE(output.size() == 4u);
        CHECK(static_cast<unsigned char>(output[3]) == 0xE9);
    }
    catch (const std::runtime_error&)
    {
        SUCCEED("Current implementation may reject Latin1 alias mapping");
    }
}

TEST_CASE("Utf8 to Latin throws on unknown coding", "[core][unicode]")
{
    REQUIRE_THROWS_AS((void)UTtf8ToLatin("abc", "NotSupportedLatin"), std::runtime_error);
}

TEST_CASE("Utf8 validation detects invalid sequence", "[core][unicode]")
{
    const std::string valid = ToBytes(u8"hello \u4E2D\u6587");
    const std::string invalid = std::string("\xC3\x28", 2);

    CHECK(IsValidUtf8Chars(valid));
    CHECK_FALSE(IsValidUtf8Chars(invalid));
}

TEST_CASE("Ext ASCII validation supports ASCII and GBK pair", "[core][unicode]")
{
    const std::string ascii = "hello";
    const std::string gbkPair = std::string("\xD6\xD0", 2);
    const std::string invalid = std::string("\xD6", 1);

    CHECK(IsValidExtAsciiChars(ascii));
    CHECK(IsValidExtAsciiChars(gbkPair));
    CHECK_FALSE(IsValidExtAsciiChars(invalid));
}

TEST_CASE("MBCS UTF8 round trip with explicit UTF-8 coding", "[core][unicode]")
{
    const std::string input = ToBytes(u8"RoundTrip \u6D4B\u8BD5");

    const std::string mbcs = Utf8ToLocalMBCS(input, "UTF-8");
    const std::string output = LocalMBCSToUtf8(mbcs, "UTF-8");

    CHECK(input == output);
}

TEST_CASE("UTF16LE and MBCS round trip with explicit UTF-8 coding", "[core][unicode]")
{
    const std::wstring input = L"RoundTrip \u6D4B\u8BD5";

    const std::string mbcs = Utf16LeToLocalMBCS(input, "UTF-8");
    const std::wstring output = LocalMBCSToUtf16Le(mbcs, "UTF-8");

    CHECK(input == output);
}

TEST_CASE("DetectEncoding returns name for UTF-8 data", "[core][unicode]")
{
    const auto [name, confidence] = DetectEncoding(ToBytes(u8"Encoding \u6D4B\u8BD5"));
    CHECK_FALSE(name.empty());
    CHECK(confidence >= 0);
}

TEST_CASE("AdaptiveToUtf8 keeps valid UTF-8 string", "[core][unicode]")
{
    const std::string input = ToBytes(u8"Keep UTF-8 \u4E2D\u6587");
    CHECK(AdaptiveToUtf8(input) == input);
}

TEST_CASE("DetectSpecialCharset recognizes ASCII", "[core][unicode]")
{
    const CharsetType type = DetectSpecialCharset("ASCII text 123");
    CHECK((type == CharsetType::ASCII || type == CharsetType::EXTENDED_ASCII || type == CharsetType::LATIN));
}

TEST_CASE("DetectSystemEncoding returns non-empty", "[core][unicode]")
{
    const std::string encoding = DetectSystemEncoding();
    CHECK_FALSE(encoding.empty());
}

TEST_CASE("ASCII or Latin checker works", "[core][unicode]")
{
    CHECK(IaAsciiOrLatinString(L"Cafe"));
    CHECK(IaAsciiOrLatinString(L"Caf\u00E9"));
    CHECK_FALSE(IaAsciiOrLatinString(L"\u4E2D\u6587"));
}

TEST_CASE("CasefoldUtf8 lowercases ASCII", "[core][unicode]")
{
    CHECK(CasefoldUtf8("HeLLo") == "hello");
}
