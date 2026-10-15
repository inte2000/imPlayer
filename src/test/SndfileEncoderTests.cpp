#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "EncodeControl.h"
#include "EncoderParamName.h"
#include "EncoderParamterDefineUtils.h"
#include "FileStream.h"
#include "LibsndFunc.h"
#include "MemBufStream.h"

namespace
{
std::vector<EncoderParamter> Parameters(AudioDataFormat code, uint32_t bits, uint32_t channels = 1)
{
    return {
        { EncoderParamName::CodeFormat, EncoderParamType::UnsignedInt, static_cast<uint32_t>(code) },
        { EncoderParamName::BitsPerSample, EncoderParamType::UnsignedInt, bits },
        { EncoderParamName::SampleRates, EncoderParamType::UnsignedInt, uint32_t(44100) },
        { EncoderParamName::Channels, EncoderParamType::UnsignedInt, channels }
    };
}

class CTestOutputStream : public CMemoryBufStream
{
public:
    CTestOutputStream() : CMemoryBufStream(false) { Open(65536); }
    uint32_t maxWrite = UINT32_MAX;
    bool failWrite = false;
    bool throwWrite = false;
    bool throwSeek = false;
    void SetStyle(DataStreamStyle style) { m_style = style; }

    uint32_t Write(const void* data, uint32_t size, uint32_t timeout = 0) override
    {
        if (throwWrite) throw std::runtime_error("Injected write failure");
        if (failWrite) return 0;
        return CMemoryBufStream::Write(data, (std::min)(size, maxWrite), timeout);
    }
    void Seek(SeekBase base, long long offset) override
    {
        if (throwSeek) throw std::runtime_error("Injected seek failure");
        CMemoryBufStream::Seek(base, offset);
    }
};

using SndfilePtr = std::unique_ptr<SNDFILE, decltype(&sf_close)>;
SndfilePtr Decode(CDataStream& stream, SF_INFO& info)
{
    stream.Seek(SeekBase::Begin, 0);
    return SndfilePtr(SndfileOpenStream(&stream, info), &sf_close);
}
}

TEST_CASE("Every advertised sndfile output format opens with its queried defaults", "[encoder][sndfile]")
{
    std::vector<EncoderFormatDefine> formats;
    CEncodeControl::QueryFormatsDefine(formats);
    REQUIRE(formats.size() > 20);
    std::vector<uint32_t> ids;
    for (const auto& format : formats) {
        INFO(format.desc);
        REQUIRE_FALSE(format.extname.empty());
        REQUIRE(std::find(ids.begin(), ids.end(), format.streamFmt) == ids.end());
        ids.push_back(format.streamFmt);
        auto stream = MakeMemoryBufStream(65536, false);
        CEncodeControl encoder(format.streamFmt);
        std::vector<EncoderParamterDefine> definitions;
        CEncodeControl::QueryParamtersDefine(format.streamFmt, definitions);
        const auto params = BuildDefaultEncoderParamters(definitions);
        const bool opened = encoder.Open(stream.get(), params);
        INFO(encoder.GetErrorMessage());
        REQUIRE(opened);
        const auto trans = encoder.GetTransFormat();
        REQUIRE(encoder.Start(&trans));
        const std::vector<short> samples(4096 * trans.numChannels, 0);
        REQUIRE((trans.format == AudioDataFormat::PCM_S16 || trans.format == AudioDataFormat::PCM_S8));
        REQUIRE(encoder.Encode(samples.data(), 4096) == 4096);
        REQUIRE(encoder.Stop());
        REQUIRE(stream->GetLength() > 0);
        SF_INFO info{};
        if (format.streamFmt == StreamFormatRaw) {
            info.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16;
            info.samplerate = trans.sampleRate;
            info.channels = trans.numChannels;
        }
        auto decoded = Decode(*stream, info);
        REQUIRE(decoded != nullptr);
        REQUIRE(info.frames >= 4096);
        REQUIRE(info.channels == static_cast<int>(trans.numChannels));
    }
    REQUIRE(std::find(ids.begin(), ids.end(), StreamFormatMat5) != ids.end());
    REQUIRE(std::find(ids.begin(), ids.end(), StreamFormatSd2) == ids.end());
}

TEST_CASE("Sndfile encoder preserves PCM16 and 24-bit samples and multichannel layout", "[encoder][sndfile]")
{
    const auto bits = GENERATE(uint32_t(16), uint32_t(24), uint32_t(32));
    auto stream = MakeMemoryBufStream(4096, false);
    CEncodeControl encoder(StreamFormatWav);
    REQUIRE(encoder.Open(stream.get(), Parameters(AudioDataFormat::PCM_S16, bits, 2)));
    const auto trans = encoder.GetTransFormat();
    REQUIRE(trans.format == (bits == 16 ? AudioDataFormat::PCM_S16 : AudioDataFormat::PCM_S32));
    REQUIRE(trans.blockAlign == (bits == 16 ? 4 : 8));
    REQUIRE(encoder.Start(&trans));
    const std::array<int, 6> samples{ INT_MIN, -0x12345600, 0, 0x12345600, 0x7fffff00, 0 };
    const std::array<short, 6> shortSamples{ -32768, -1234, 0, 1234, 32767, 0 };
    REQUIRE(encoder.Encode(bits == 16 ? static_cast<const void*>(shortSamples.data()) : samples.data(), 3) == 3);
    REQUIRE(encoder.Stop());
    SF_INFO info{};
    auto decoded = Decode(*stream, info);
    REQUIRE(decoded != nullptr);
    REQUIRE(info.frames == 3);
    REQUIRE(info.channels == 2);
    REQUIRE((info.format & SF_FORMAT_SUBMASK) == (bits == 16 ? SF_FORMAT_PCM_16 : bits == 24 ? SF_FORMAT_PCM_24 : SF_FORMAT_PCM_32));
    if (bits == 16) {
        std::array<short, 6> actual{};
        REQUIRE(sf_readf_short(decoded.get(), actual.data(), 3) == 3);
        REQUIRE(actual == shortSamples);
    } else {
        std::array<int, 6> actual{};
        REQUIRE(sf_readf_int(decoded.get(), actual.data(), 3) == 3);
        REQUIRE(actual == samples);
    }
}

TEST_CASE("Sndfile encoder writes float32 and float64 without integer scaling", "[encoder][sndfile]")
{
    const auto bits = GENERATE(uint32_t(32), uint32_t(64));
    auto stream = MakeMemoryBufStream(4096, false);
    CEncodeControl encoder(StreamFormatWav);
    const auto format = bits == 32 ? AudioDataFormat::Float32 : AudioDataFormat::Float64;
    REQUIRE(encoder.Open(stream.get(), Parameters(format, bits)));
    const auto trans = encoder.GetTransFormat();
    REQUIRE(trans.format == format);
    REQUIRE(trans.blockAlign == bits / 8);
    REQUIRE(encoder.Start(&trans));
    const float samples32[]{ -0.75f, 0, 0.25f, 1.25f };
    const double samples64[]{ -0.75, 0, 0.25, 1.25 };
    REQUIRE(encoder.Encode(bits == 32 ? static_cast<const void*>(samples32) : samples64, 4) == 4);
    REQUIRE(encoder.Stop());
    SF_INFO info{};
    auto decoded = Decode(*stream, info);
    REQUIRE(decoded != nullptr);
    REQUIRE((info.format & SF_FORMAT_SUBMASK) == (bits == 32 ? SF_FORMAT_FLOAT : SF_FORMAT_DOUBLE));
    double actual[4]{};
    REQUIRE(sf_readf_double(decoded.get(), actual, 4) == 4);
    for (int i = 0; i < 4; ++i) REQUIRE(actual[i] == samples64[i]);
}

TEST_CASE("Sndfile encoder finalizes nonzero compressed audio", "[encoder][sndfile]")
{
    const auto codec = GENERATE(AudioDataFormat::PCM_S16, AudioDataFormat::Vorbis,
        AudioDataFormat::Opus, AudioDataFormat::MpegLayer3, AudioDataFormat::Alac24);
    const auto container = codec == AudioDataFormat::PCM_S16 ? StreamFormatFlac
        : codec == AudioDataFormat::MpegLayer3 ? StreamFormatMp3
        : codec == AudioDataFormat::Alac24 ? StreamFormatCaf : StreamFormatOgg;
    INFO(static_cast<int>(codec));
    auto stream = MakeMemoryBufStream(65536, false);
    CEncodeControl encoder(container);
    std::vector<EncoderParamterDefine> definitions;
    CEncodeControl::QueryParamtersDefine(container, definitions);
    auto params = BuildDefaultEncoderParamters(definitions);
    FindEncoderParamter(params, EncoderParamName::CodeFormat)->SetUnsignedInt(static_cast<uint32_t>(codec));
    if (codec == AudioDataFormat::Opus) {
        FindEncoderParamter(params, EncoderParamName::SampleRates)->SetUnsignedInt(48000);
    }
    const bool opened = encoder.Open(stream.get(), params);
    INFO(encoder.GetErrorMessage());
    REQUIRE(opened);
    auto input = encoder.GetTransFormat();
    InitAudioFormat(&input, AudioDataFormat::Float32, input.numChannels, input.sampleRate);
    REQUIRE(encoder.Start(&input));
    std::vector<float> samples(8192);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        samples[i] = static_cast<float>(0.5 * std::sin(2 * 3.141592653589793 * 440 * i / input.sampleRate));
    }
    REQUIRE(encoder.Encode(samples.data(), 4096) == 4096);
    REQUIRE(encoder.Encode(samples.data() + 4096, 4096) == 4096);
    REQUIRE(encoder.Stop());
    SF_INFO info{};
    auto decoded = Decode(*stream, info);
    REQUIRE(decoded != nullptr);
    REQUIRE(info.samplerate == static_cast<int>(input.sampleRate));
    std::vector<float> actual(8192);
    REQUIRE(sf_readf_float(decoded.get(), actual.data(), 8192) == 8192);
    double energy = 0;
    for (float sample : actual) energy += sample * sample;
    REQUIRE(energy / actual.size() > 0.05);
    REQUIRE(energy / actual.size() < 0.25);
    if (codec == AudioDataFormat::PCM_S16 || codec == AudioDataFormat::Alac24) {
        for (std::size_t i = 0; i < actual.size(); ++i) {
            REQUIRE(actual[i] == Catch::Approx(samples[i]).margin(1.0 / 32768));
        }
    }
}

TEST_CASE("Sndfile encoder writes through a host owned file stream", "[encoder][sndfile]")
{
    const auto path = std::filesystem::temp_directory_path() / "iplayer_sndfile_encoder_test.wav";
    struct Cleanup
    {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{ path };
    {
        auto stream = MakeFileStream(path.wstring(), false);
        CEncodeControl encoder(StreamFormatWav);
        REQUIRE(encoder.Open(stream.get(), Parameters(AudioDataFormat::PCM_U8, 8)));
        const auto trans = encoder.GetTransFormat();
        REQUIRE(trans.format == AudioDataFormat::PCM_U8);
        REQUIRE(encoder.Start(&trans));
        const uint8_t samples[]{ 0, 128, 255 };
        REQUIRE(encoder.Encode(samples, 3) == 3);
        encoder.Close();
    }
    SF_INFO info{};
    SndfilePtr file(sf_wchar_open(path.c_str(), SFM_READ, &info), &sf_close);
    REQUIRE(file != nullptr);
    REQUIRE(info.frames == 3);
    REQUIRE((info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_U8);
    short samples[3]{};
    REQUIRE(sf_readf_short(file.get(), samples, 3) == 3);
    REQUIRE(samples[0] == -32768);
    REQUIRE(samples[1] == 0);
    REQUIRE(samples[2] == 32512);
}

TEST_CASE("Sndfile native subtype selection reaches codecs outside AudioDataFormat", "[encoder][sndfile]")
{
    const int subtype = GENERATE(SF_FORMAT_DWVW_16, SF_FORMAT_VOX_ADPCM, SF_FORMAT_NMS_ADPCM_16);
    INFO(subtype);
    const uint32_t container = subtype == SF_FORMAT_DWVW_16 ? StreamFormatAiff
        : subtype == SF_FORMAT_VOX_ADPCM ? StreamFormatRaw : StreamFormatWav;
    std::vector<EncoderParamterDefine> definitions;
    CEncodeControl::QueryParamtersDefine(container, definitions);
    const auto found = std::find_if(definitions.begin(), definitions.end(), [](const auto& definition) {
        return definition.GetName() == EncoderParamName::SndfileSubtype;
    });
    REQUIRE(found != definitions.end());
    REQUIRE(std::find(found->GetOptionValues().begin(), found->GetOptionValues().end(),
        EncoderParamterDefine::ValueType(static_cast<uint32_t>(subtype))) != found->GetOptionValues().end());
    auto params = Parameters(AudioDataFormat::PCM_S16, 16);
    FindEncoderParamter(params, EncoderParamName::SampleRates)->SetUnsignedInt(8000);
    params.emplace_back(EncoderParamName::SndfileSubtype, EncoderParamType::UnsignedInt, static_cast<uint32_t>(subtype));
    auto stream = MakeMemoryBufStream(65536, false);
    CEncodeControl encoder(container);
    const bool opened = encoder.Open(stream.get(), params);
    INFO(encoder.GetErrorMessage());
    REQUIRE(opened);
    const auto trans = encoder.GetTransFormat();
    REQUIRE(encoder.Start(&trans));
    const std::vector<short> samples(4096, 1234);
    const auto written = encoder.Encode(samples.data(), 4096);
    INFO(encoder.GetErrorMessage());
    REQUIRE(written == 4096);
    REQUIRE(encoder.Stop());
    SF_INFO info{};
    if (container == StreamFormatRaw) {
        info.format = SF_FORMAT_RAW | subtype;
        info.samplerate = 8000;
        info.channels = 1;
    }
    auto decoded = Decode(*stream, info);
    REQUIRE(decoded != nullptr);
    REQUIRE((info.format & SF_FORMAT_SUBMASK) == subtype);
    std::vector<short> actual(4096);
    REQUIRE(sf_readf_short(decoded.get(), actual.data(), 4096) == 4096);
    REQUIRE(std::any_of(actual.begin(), actual.end(), [](short sample) { return sample != 0; }));
}

TEST_CASE("Sndfile encoder negotiates alternate PCM storage without changing rate or channels", "[encoder][sndfile]")
{
    const auto input = GENERATE(AudioDataFormat::PCM_U8, AudioDataFormat::PCM_S8, AudioDataFormat::PCM_64);
    auto stream = MakeMemoryBufStream(4096, false);
    CEncodeControl encoder(StreamFormatWav);
    REQUIRE(encoder.Open(stream.get(), Parameters(AudioDataFormat::Float64, 64)));
    AudioFormat actual{};
    InitAudioFormat(&actual, input, 1, 44100);
    REQUIRE(encoder.Start(&actual));
    const uint8_t unsignedSamples[]{ 0, 128, 192 };
    const int8_t signedSamples[]{ -128, 0, 64 };
    const int64_t wideSamples[]{ INT64_MIN, 0, INT64_C(4611686018427387904) };
    const void* samples = input == AudioDataFormat::PCM_U8 ? static_cast<const void*>(unsignedSamples)
        : input == AudioDataFormat::PCM_S8 ? static_cast<const void*>(signedSamples) : wideSamples;
    REQUIRE(encoder.Encode(samples, 3) == 3);
    REQUIRE(encoder.Stop());
    SF_INFO info{};
    auto decoded = Decode(*stream, info);
    REQUIRE(decoded != nullptr);
    double values[3]{};
    REQUIRE(sf_readf_double(decoded.get(), values, 3) == 3);
    REQUIRE(values[0] == Catch::Approx(-1.0));
    REQUIRE(values[1] == 0);
    REQUIRE(values[2] == Catch::Approx(0.5));
}

TEST_CASE("Sndfile encoder rejects invalid output parameters and unsafe state transitions", "[encoder][sndfile]")
{
    auto stream = MakeMemoryBufStream(4096, false);
    CEncodeControl encoder(StreamFormatWav);
    REQUIRE_FALSE(encoder.Open(nullptr, {}));
    REQUIRE_FALSE(encoder.Stop());
    REQUIRE(encoder.Encode(nullptr, 10) == 0);
    REQUIRE_FALSE(encoder.Open(stream.get(), Parameters(AudioDataFormat::PCM_64, 64)));
    auto invalid = Parameters(AudioDataFormat::PCM_S16, 16, 0);
    REQUIRE_FALSE(encoder.Open(stream.get(), invalid));
    invalid = Parameters(AudioDataFormat::PCM_S16, 16);
    FindEncoderParamter(invalid, EncoderParamName::CodeFormat)->SetUnsignedInt(UINT32_MAX);
    REQUIRE_FALSE(encoder.Open(stream.get(), invalid));
    CEncodeControl unsupported(StreamFormatUnknown);
    REQUIRE_FALSE(unsupported.Open(stream.get(), {}));
    invalid = Parameters(AudioDataFormat::PCM_S16, 16);
    FindEncoderParamter(invalid, EncoderParamName::SampleRates)->SetString("bad rate");
    REQUIRE_FALSE(encoder.Open(stream.get(), invalid));
    REQUIRE(stream->GetLength() == 0);
    REQUIRE(encoder.Open(stream.get(), {}));
    const auto trans = encoder.GetTransFormat();
    REQUIRE_FALSE(encoder.Open(stream.get(), {}));
    REQUIRE_FALSE(encoder.Start(nullptr));
    auto bad = trans;
    bad.format = AudioDataFormat::PCM_S24;
    REQUIRE_FALSE(encoder.Start(&bad));
    bad = trans;
    bad.blockAlign = 1;
    REQUIRE_FALSE(encoder.Start(&bad));
    bad = trans;
    bad.numChannels = 2;
    REQUIRE_FALSE(encoder.Start(&bad));
    REQUIRE(encoder.Start(&trans));
    REQUIRE_FALSE(encoder.Start(&trans));
    REQUIRE(encoder.Encode(nullptr, 0) == 0);
    const short sample = 123;
    REQUIRE(encoder.Encode(&sample, 1) == 1);
    REQUIRE(encoder.Stop());
    REQUIRE(encoder.Stop());
    REQUIRE_FALSE(encoder.Start(&trans));
    encoder.Close();
    REQUIRE(encoder.GetTransFormat().format == AudioDataFormat::UNKNOWN);
}

TEST_CASE("Sndfile virtual output handles short writes and contains stream exceptions", "[encoder][sndfile]")
{
    CTestOutputStream stream;
    CEncodeControl encoder(StreamFormatWav);
    SECTION("Partial writes are retried")
    {
        stream.maxWrite = 3;
        REQUIRE(encoder.Open(&stream, {}));
        const auto trans = encoder.GetTransFormat();
        REQUIRE(encoder.Start(&trans));
        const short samples[]{ -1, 0, 1 };
        REQUIRE(encoder.Encode(samples, 3) == 3);
        REQUIRE(encoder.Stop());
        SF_INFO info{};
        auto decoded = Decode(stream, info);
        REQUIRE(decoded != nullptr);
        short actual[3]{};
        REQUIRE(sf_readf_short(decoded.get(), actual, 3) == 3);
        REQUIRE(std::equal(std::begin(samples), std::end(samples), std::begin(actual)));
    }
    SECTION("Write exception while opening is returned as an error")
    {
        stream.throwWrite = true;
        REQUIRE_FALSE(encoder.Open(&stream, {}));
        REQUIRE_FALSE(encoder.GetErrorMessage().empty());
    }
    SECTION("Seek exception while opening is returned as an error")
    {
        stream.throwSeek = true;
        REQUIRE_FALSE(encoder.Open(&stream, {}));
    }
    SECTION("Non-seekable streams are rejected before writing")
    {
        stream.SetStyle(dsStyleWritable | dsStyleTellPos);
        REQUIRE_FALSE(encoder.Open(&stream, {}));
        REQUIRE(stream.GetLength() == 0);
    }
    SECTION("Output failure cannot be mistaken for successful finalization")
    {
        REQUIRE(encoder.Open(&stream, {}));
        const auto trans = encoder.GetTransFormat();
        REQUIRE(encoder.Start(&trans));
        stream.failWrite = true;
        const short sample = 123;
        encoder.Encode(&sample, 1);
        REQUIRE_FALSE(encoder.GetErrorMessage().empty());
        REQUIRE_FALSE(encoder.Stop());
    }
    SECTION("Compressed output reports stream failures during finalization")
    {
        CEncodeControl compressed(StreamFormatFlac);
        REQUIRE(compressed.Open(&stream, {}));
        const auto trans = compressed.GetTransFormat();
        REQUIRE(compressed.Start(&trans));
        const short samples[]{ -1, 0, 1 };
        REQUIRE(compressed.Encode(samples, 3) == 3);
        stream.throwWrite = true;
        REQUIRE_FALSE(compressed.Stop());
        REQUIRE_FALSE(compressed.GetErrorMessage().empty());
    }
}
