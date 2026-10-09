#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "AudioDecoderMap.h"

TEST_CASE("AudioDecoderMap base and custom mapping works", "[decoder][map]")
{
    CAudioDecoderMap decoderMap;
    decoderMap.SetDecoderMap(100, "fmt100", "native-a");
    decoderMap.SetDecoderMap(100, "fmt100-new", "native-b");

    CHECK(decoderMap.GetDecoderName(100) == "native-a");

    decoderMap.AddCustomDecoderMap(100, "fmt100", "plugin-x");
    CHECK(decoderMap.GetDecoderName(100) == "plugin-x");

    decoderMap.RemoveCustomDecoderMap("plugin-x");
    CHECK(decoderMap.GetDecoderName(100) == "native-a");

    decoderMap.RemoveDecoderMap("native-a");
    REQUIRE_THROWS_AS((void)decoderMap.GetDecoderName(100), std::runtime_error);
}

TEST_CASE("AudioDecoderMap throws when empty", "[decoder][map]")
{
    CAudioDecoderMap decoderMap;
    REQUIRE_THROWS_AS((void)decoderMap.GetDecoderName(1), std::runtime_error);
}

TEST_CASE("AudioDecoderMap save and load round trip", "[decoder][map]")
{
    const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "implayer_decoder_map_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    const std::vector<DecoderDesc> source = {
        { 1, "mp3", "Mpg123 Decoder" },
        { 2, "flac", "Plugin Decoder" },
    };

    REQUIRE(SaveDecoderMapFile(tmp.string(), source));

    std::vector<DecoderDesc> loaded;
    REQUIRE(LoadDecoderMapFile(tmp.string(), loaded));
    REQUIRE(loaded.size() == source.size());
    CHECK(loaded[0].st == 1);
    CHECK(loaded[0].decodername == "Mpg123 Decoder");
    CHECK(loaded[1].st == 2);
    CHECK(loaded[1].desc == "flac");

    std::filesystem::remove(tmp, ec);
}
