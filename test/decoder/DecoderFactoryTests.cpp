#include <catch2/catch_test_macros.hpp>

#include <string>

#include "DecoderFactory.h"

TEST_CASE("DecoderFactory contains native decoder by default", "[decoder][factory]")
{
    CDecoderFactory& factory = CDecoderFactory::GetInstance();
    const auto decoders = factory.GetDecoders();

    REQUIRE_FALSE(decoders.empty());

    const std::string& name = std::get<0>(decoders[0]);
    const uint32_t type = std::get<1>(decoders[0]);
    const std::string& hostfile = std::get<2>(decoders[0]);
    CHECK(name == "Mpg123 Decoder");
    CHECK(type == DECODE_TYPE_NATIVE);
    CHECK(hostfile.empty());
}

TEST_CASE("DecoderFactory plugin queries are empty without plugins", "[decoder][factory]")
{
    CDecoderFactory& factory = CDecoderFactory::GetInstance();
    CHECK_FALSE(factory.GetDecoderPlugin("Mpg123 Decoder").has_value());
    CHECK(factory.GetPluginDecoderExtFilters().empty());
}

TEST_CASE("DecoderFactory can create native decoder and remove plugin validates type", "[decoder][factory]")
{
    CDecoderFactory& factory = CDecoderFactory::GetInstance();
    auto decoder = factory.MakeAudioDecoder(StreamFormatMp3);
    REQUIRE(decoder != nullptr);

    const auto [ok, msg] = factory.RemovePluginObject("Mpg123 Decoder");
    CHECK_FALSE(ok);
    CHECK_FALSE(msg.empty());
}

TEST_CASE("DecoderFactory custom mapping changes target decoder", "[decoder][factory]")
{
    CDecoderFactory& factory = CDecoderFactory::GetInstance();
    const std::string customDecoder = "NotExistDecoder";

    REQUIRE(factory.SetCustomDecoderMap(StreamFormatMp3, "MPEG Audio Layer III", customDecoder));
    REQUIRE_THROWS_AS((void)factory.MakeAudioDecoder(StreamFormatMp3), std::runtime_error);

    factory.RemoveDecoderMap(customDecoder);
    auto decoder = factory.MakeAudioDecoder(StreamFormatMp3);
    REQUIRE(decoder != nullptr);
}
