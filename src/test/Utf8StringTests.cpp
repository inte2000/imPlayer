#include <catch2/catch_test_macros.hpp>

#include <string>

#include "Utf8String.h"

TEST_CASE("is_valid_utf8 detects valid and invalid UTF-8", "[core][utf8string]")
{
    const std::string valid = "hello \xE4\xB8\xAD\xE6\x96\x87";
    const std::string invalid = std::string("\xC3\x28", 2);

    CHECK(is_valid_utf8(valid));
    CHECK_FALSE(is_valid_utf8(invalid));
}

TEST_CASE("is_valid_gbk accepts ASCII and common GBK pair", "[core][utf8string]")
{
    const std::string ascii = "abc123";
    const std::string gbkPair = std::string("\xD6\xD0", 2);
    const std::string invalid = std::string("\x81", 1);

    CHECK(is_valid_gbk(ascii));
    CHECK(is_valid_gbk(gbkPair));
    CHECK_FALSE(is_valid_gbk(invalid));
}

TEST_CASE("DecodeUnknownString normalizes UTF-8 and handles empty", "[core][utf8string]")
{
    const std::string decomposedUtf8 = "e\xCC\x81";
    CHECK(DecodeUnknownString(decomposedUtf8) == std::string("\xC3\xA9"));
    CHECK(DecodeUnknownString("").empty());
}
