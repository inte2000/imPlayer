#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include "core/EncodingParams.h"
#include "encoder/EncoderFactory.h"
#include "encoder/EncoderParamName.h"
#include "encoder/EncoderParamterDefine.h"
#include "encoder/EncoderParamterDefineUtils.h"
#include "encoder/WavEncoder.h"
#include "player/AudioTarget.h"

TEST_CASE("Build defaults from parameter definitions", "[encoder]")
{
    EncoderParamterDefine sampleRate(EncoderParamName::SampleRates, EncoderParamType::UnsignedInt, uint32_t(44100));
    sampleRate.SetOptionValues({ uint32_t(32000), uint32_t(44100), uint32_t(48000) });

    EncoderParamterDefine mode(EncoderParamName::VbrMode, EncoderParamType::String, std::string("CBR"));
    mode.SetOptionValues({ std::string("CBR"), std::string("AVR"), std::string("VBR") });

    EncoderParamterDefine quality(EncoderParamName::Quality, EncoderParamType::Float, 4.0);
    quality.SetFloatRange(0.0, 9.0);

    std::vector<EncoderParamterDefine> defines;
    defines.push_back(sampleRate);
    defines.push_back(mode);
    defines.push_back(quality);

    auto defaults = BuildDefaultEncoderParamters(defines);

    REQUIRE(defaults.size() == 3);
    REQUIRE(defaults[0].GetName() == EncoderParamName::SampleRates);
    REQUIRE(defaults[1].GetName() == EncoderParamName::VbrMode);
    REQUIRE(defaults[2].GetName() == EncoderParamName::Quality);

    REQUIRE(defaults[0].GetValue<uint32_t>().value() == 44100);
    REQUIRE(defaults[1].GetValue<std::string>().value() == "CBR");
    REQUIRE(defaults[2].GetValue<double>().value() == 4.0);
}

TEST_CASE("Query parameter definition by name", "[encoder]")
{
    std::vector<EncoderParamterDefine> defines;
    defines.emplace_back(EncoderParamName::BitsPerSample, EncoderParamType::UnsignedInt, uint32_t(16));
    defines.emplace_back(EncoderParamName::CompressLevel, EncoderParamType::UnsignedInt, uint32_t(5));

    REQUIRE(HasEncoderParamterDefine(defines, EncoderParamName::BitsPerSample));
    REQUIRE(HasEncoderParamterDefine(defines, EncoderParamName::CompressLevel));
    REQUIRE_FALSE(HasEncoderParamterDefine(defines, EncoderParamName::CodeFormat));
}

TEST_CASE("Provide common encoder parameter names", "[encoder]")
{
    REQUIRE(std::string(EncoderParamName::SampleRates) == "SampleRates");
    REQUIRE(std::string(EncoderParamName::BitsPerSample) == "BitsPerSample");
    REQUIRE(std::string(EncoderParamName::CodeFormat) == "CodeFormat");
    REQUIRE(std::string(EncoderParamName::Channels) == "Channels");
    REQUIRE(std::string(EncoderParamName::ChLayout) == "ChLayout");
    REQUIRE(std::string(EncoderParamName::VbrMode) == "VbrMode");
    REQUIRE(std::string(EncoderParamName::Quality) == "Quality");
    REQUIRE(std::string(EncoderParamName::CompressLevel) == "CompressLevel");
    REQUIRE(std::string(EncoderParamName::ExtWav) == "ExtWav");
}

TEST_CASE("Wav encoder static metadata", "[encoder]")
{
    REQUIRE(CWavEncoder::GetName() == "dr_wav Encoder");

    const auto defines = CWavEncoder::GetParameterDefine();
    REQUIRE(defines.size() == 5);
    REQUIRE(HasEncoderParamterDefine(defines, EncoderParamName::CodeFormat));
    REQUIRE(HasEncoderParamterDefine(defines, EncoderParamName::SampleRates));
    REQUIRE(HasEncoderParamterDefine(defines, EncoderParamName::BitsPerSample));
    REQUIRE(HasEncoderParamterDefine(defines, EncoderParamName::Channels));
    REQUIRE(HasEncoderParamterDefine(defines, EncoderParamName::ExtWav));

    const auto fmts = CWavEncoder::GetFormatDefine();
    REQUIRE(fmts.size() == 1);
    REQUIRE(fmts[0].streamFmt == StreamFormatWav);
    REQUIRE(fmts[0].desc == "MS-Wave format");
    REQUIRE(fmts[0].extname == ".wav");
}

TEST_CASE("EncoderFactory provides wav encoder item", "[encoder]")
{
    const auto itemOpt = CEncoderFactory::GetInstance().GetEncoderItem(CWavEncoder::GetName());
    REQUIRE(itemOpt.has_value());
    REQUIRE(itemOpt->name == CWavEncoder::GetName());
    REQUIRE(itemOpt->type == ENCODE_TYPE_NATIVE);

    const auto fmts = itemOpt->QueryFormats();
    REQUIRE_FALSE(fmts.empty());
    REQUIRE(fmts[0].streamFmt == StreamFormatWav);
}

TEST_CASE("MakeFileAudioTarget uses EncoderFactory", "[encoder]")
{
    const auto invalid = MakeFileAudioTarget(L"dummy.wav", StreamFormatWav, "missing-encoder");
    REQUIRE(invalid == nullptr);

    const std::filesystem::path output = std::filesystem::temp_directory_path() / L"iplay_encoder_factory_test.wav";
    auto target = MakeFileAudioTarget(output.wstring(), StreamFormatWav, CWavEncoder::GetName());
    REQUIRE(target != nullptr);
    REQUIRE_FALSE(target->GetName().empty());

    std::error_code ec;
    std::filesystem::remove(output, ec);
}
