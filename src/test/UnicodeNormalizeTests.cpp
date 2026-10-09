#include <catch2/catch_test_macros.hpp>

#include <string>

#include "UnicodeNormalize.h"

TEST_CASE("NormalizeUnicode converts decomposed to NFC", "[core][unicode][normalize]")
{
    const std::u16string decomposed = u"e\u0301";
    const std::u16string normalized = NormalizeUnicode(decomposed, NormalizerType::NFC);

    CHECK(normalized == std::u16string(u"\u00E9"));
}

TEST_CASE("NormalizeUtf8 supports std::string and std::u8string", "[core][unicode][normalize]")
{
    const std::string decomposed = "e\xCC\x81";
    const std::string normalizedString = NormalizeUtf8(decomposed, NormalizerType::NFC);
    CHECK(normalizedString == std::string("\xC3\xA9"));

    const std::u8string decomposedU8 = u8"e\u0301";
    const std::u8string normalizedU8 = NormalizeUtf8(decomposedU8, NormalizerType::NFC);
    CHECK(normalizedU8 == std::u8string(u8"\u00E9"));
}

TEST_CASE("NormalizeUnicode throws on invalid normalize type", "[core][unicode][normalize]")
{
    const auto invalid = static_cast<NormalizerType>(99);
    REQUIRE_THROWS_AS((void)NormalizeUnicode(u"abc", invalid), std::runtime_error);
}
