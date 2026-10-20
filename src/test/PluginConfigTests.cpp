#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <utility>

#include <nlohmann/json.hpp>

#include "PluginConfig.h"
#include "ScopeGuard.h"

using json = nlohmann::json;

TEST_CASE("Plugin config preserves UTF-8 paths and contents", "[decoder][plugin-config][encoding]")
{
    const auto uniqueId = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto folder = std::filesystem::temp_directory_path()
        / ("implayer-config-" + std::to_string(uniqueId));
    std::filesystem::create_directories(folder);
    CScopeGuard cleanup([&]() {
        std::error_code ec;
        std::filesystem::remove_all(folder, ec);
    });
    const auto path = folder / u8"插件-测试.config";
    const auto utf8Path = path.u8string();
    const std::string filename(reinterpret_cast<const char*>(utf8Path.data()), utf8Path.size());
    const std::vector<PluginConfig> expected = {
        { "测试解码器", "imPlayer Group", "Decoder", "test.ipdplus" }
    };

    REQUIRE(SavePluginConfigFile(filename, expected));
    REQUIRE(std::filesystem::exists(path));
    std::vector<PluginConfig> actual;
    REQUIRE(LoadPluginConfigFile(filename, actual));
    REQUIRE(actual.size() == 1);
    CHECK(actual[0].name == expected[0].name);
    CHECK(actual[0].hostfile == expected[0].hostfile);
}

TEST_CASE("Parse plugin config json to vector", "[decoder][plugin-config]")
{
    const json pluginJson = {
        { "plugins", json::array({
            {
                { "hostfile", "libsnd_decoder.ipdplus" },
                { "name", "libsndfile decoder" },
                { "publisher", "imPlayer Group" },
                { "type", "Decoder" }
            },
            {
                { "hostfile", "ffmpeg_decoder.ipdplus" },
                { "name", "ffmpeg decoder" },
                { "publisher", "imPlayer Group" },
                { "type", "Decoder" }
            }
        }) }
    };

    std::vector<PluginConfig> plusItems;
    const bool ok = ParsePluginConfigJson(pluginJson, plusItems);

    REQUIRE(ok);
    REQUIRE(plusItems.size() == 2);
    CHECK(plusItems[0].hostfile == "libsnd_decoder.ipdplus");
    CHECK(plusItems[0].name == "libsndfile decoder");
    CHECK(plusItems[0].publisher == "imPlayer Group");
    CHECK(plusItems[0].type == "Decoder");
    CHECK(plusItems[1].hostfile == "ffmpeg_decoder.ipdplus");
}

TEST_CASE("Build plugin config json from vector", "[decoder][plugin-config]")
{
    const std::vector<PluginConfig> plusItems = {
        { "libsndfile decoder", "imPlayer Group", "Decoder", "libsnd_decoder.ipdplus" },
        { "ffmpeg decoder", "imPlayer Group", "Decoder", "ffmpeg_decoder.ipdplus" }
    };

    json pluginJson;
    const bool ok = BuildPluginConfigJson(plusItems, pluginJson);

    REQUIRE(ok);
    REQUIRE(pluginJson.contains("plugins"));
    REQUIRE(pluginJson["plugins"].is_array());
    REQUIRE(pluginJson["plugins"].size() == 2);
    CHECK(pluginJson["plugins"][0]["hostfile"] == "libsnd_decoder.ipdplus");
    CHECK(pluginJson["plugins"][0]["name"] == "libsndfile decoder");
    CHECK(pluginJson["plugins"][0]["publisher"] == "imPlayer Group");
    CHECK(pluginJson["plugins"][0]["type"] == "Decoder");
}
