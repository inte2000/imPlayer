#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include "PluginConfig.h"

using json = nlohmann::json;

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
