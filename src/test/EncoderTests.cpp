#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "EncodingParams.h"
#include "EncoderFactory.h"
#include "EncoderParamName.h"
#include "EncoderParamterDefine.h"
#include "EncoderParamterDefineUtils.h"
#include "WavEncoder.h"
#include "AudioTarget.h"
#include "LibsndFunc.h"
#include "MemBufStream.h"
#include "PlayInterface.h"
#include "UnicodeConvert.h"
#include "ScopeGuard.h"

namespace
{
using SndfilePtr = std::unique_ptr<SNDFILE, decltype(&sf_close)>;

struct EncoderState
{
    bool opened = false;
    bool openResult = true;
    bool metaResult = true;
    bool stopResult = true;
    uint32_t metaCalls = 0;
    uint32_t closeCalls = 0;
    uint32_t encodeCalls = 0;
    AudioFormat startedFmt{};
    CMediaTag tags;
};

class CTestEncoder : public CAudioEncoder
{
public:
    explicit CTestEncoder(std::shared_ptr<EncoderState> state) : m_state(std::move(state)) {}
    void Init() override {}
    bool Open(const std::vector<EncoderParamter>&) override
    {
        m_state->opened = m_state->openResult;
        return m_state->opened;
    }
    AudioFormat GetTransFormat() const override
    {
        AudioFormat format{};
        InitAudioFormat(&format, AudioDataFormat::PCM_S16, 2, 44100);
        return format;
    }
    bool SetMetaInfo(const CMediaTag& tags) override
    {
        REQUIRE(m_state->opened);
        ++m_state->metaCalls;
        m_state->tags = tags;
        return m_state->metaResult;
    }
    bool Start(const AudioFormat* format) override
    {
        m_state->startedFmt = *format;
        return true;
    }
    uint32_t Encode(const void*, uint32_t frames) override
    {
        ++m_state->encodeCalls;
        return frames;
    }
    bool Stop() override { return m_state->stopResult; }
    void Close() override
    {
        ++m_state->closeCalls;
        m_state->opened = false;
    }

private:
    std::shared_ptr<EncoderState> m_state;
};
}

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

    target.reset();
    std::error_code ec;
    std::filesystem::remove(output, ec);
}

TEST_CASE("Wav encoder separates opening from starting and finalizes on Stop", "[encoder][wav]")
{
    auto stream = MakeMemoryBufStream(4096, false);
    CWavEncoder encoder(StreamFormatWav);
    encoder.Init();
    REQUIRE(encoder.Attach(stream.get()) == nullptr);
    REQUIRE(encoder.Open({}));
    REQUIRE(stream->GetLength() == 0);
    auto format = encoder.GetTransFormat();
    REQUIRE(format.format == AudioDataFormat::PCM_S16);
    const std::array<short, 6> samples{ -32768, 32767, -123, 456, 0, -1 };
    REQUIRE(encoder.Encode(samples.data(), 3) == 0);
    REQUIRE_FALSE(encoder.Stop());
    REQUIRE_FALSE(encoder.Open({}));
    REQUIRE_FALSE(encoder.Start(nullptr));

    auto mismatch = format;
    mismatch.sampleRate = 48000;
    REQUIRE_FALSE(encoder.Start(&mismatch));
    mismatch = format;
    mismatch.blockAlign = 1;
    REQUIRE_FALSE(encoder.Start(&mismatch));
    REQUIRE(stream->GetLength() == 0);
    REQUIRE(encoder.Start(&format));
    REQUIRE_FALSE(encoder.Start(&format));
    format.sampleRate = 96000; // 修改调用方的格式不影响已经启动的编码器。
    REQUIRE(encoder.Encode(samples.data(), 3) == 3);
    REQUIRE(encoder.Stop());
    const auto length = stream->GetLength();
    REQUIRE(encoder.Stop());
    REQUIRE(stream->GetLength() == length);
    REQUIRE(encoder.Encode(samples.data(), 3) == 0);
    REQUIRE_FALSE(encoder.Start(&format));

    stream->Seek(SeekBase::Begin, 0);
    SF_INFO info{};
    SndfilePtr decoded(SndfileOpenStream(stream.get(), info), &sf_close);
    REQUIRE(decoded != nullptr);
    REQUIRE(info.frames == 3);
    REQUIRE(info.channels == 2);
    REQUIRE(info.samplerate == 44100);
    std::array<short, 6> actual{};
    REQUIRE(sf_readf_short(decoded.get(), actual.data(), 3) == 3);
    REQUIRE(actual == samples);
    decoded.reset();

    encoder.Close();
    encoder.Close();
    REQUIRE(encoder.GetTransFormat().format == AudioDataFormat::UNKNOWN);
    auto another = MakeMemoryBufStream(4096, false);
    encoder.Attach(another.get());
    REQUIRE(encoder.Open({}));
    format = encoder.GetTransFormat();
    REQUIRE(encoder.Start(&format));
    REQUIRE(encoder.Encode(samples.data(), 3) == 3);
    REQUIRE(encoder.Stop());
}

TEST_CASE("Wav encoder Close and destruction finalize active output", "[encoder][wav]")
{
    auto stream = MakeMemoryBufStream(4096, false);
    {
        CWavEncoder encoder(StreamFormatWav);
        encoder.Attach(stream.get());
        REQUIRE(encoder.Open({}));
        const auto format = encoder.GetTransFormat();
        REQUIRE(encoder.Start(&format));
        const short samples[]{ -100, 100, -200, 200 };
        REQUIRE(encoder.Encode(samples, 2) == 2);
        SECTION("Explicit Close") { encoder.Close(); }
        SECTION("Destructor cleanup") {}
    }
    stream->Seek(SeekBase::Begin, 0);
    SF_INFO info{};
    SndfilePtr decoded(SndfileOpenStream(stream.get(), info), &sf_close);
    REQUIRE(decoded != nullptr);
    REQUIRE(info.frames == 2);
}

TEST_CASE("Wav encoder rejects invalid parameters before writing output", "[encoder][wav]")
{
    auto stream = MakeMemoryBufStream(4096, false);
    CWavEncoder encoder(StreamFormatWav);
    REQUIRE_FALSE(encoder.Open({}));
    encoder.Attach(stream.get());
    const std::vector<EncoderParamter> params{
        { EncoderParamName::BitsPerSample, EncoderParamType::UnsignedInt, uint32_t(24) }
    };
    REQUIRE_THROWS_AS(encoder.Open(params), std::runtime_error);
    REQUIRE(encoder.GetTransFormat().format == AudioDataFormat::UNKNOWN);
    REQUIRE(stream->GetLength() == 0);
    REQUIRE(encoder.Open({}));
    encoder.Close();
    CWavEncoder unsupported(StreamFormatFlac);
    unsupported.Attach(stream.get());
    REQUIRE_FALSE(unsupported.Open({}));
}

TEST_CASE("Audio target caches metadata until Open and forwards updates immediately", "[encoder][target]")
{
    auto state = std::make_shared<EncoderState>();
    CAudioTarget target(MakeMemoryBufStream(4096, false), std::make_unique<CTestEncoder>(state));
    CMediaTag tags;
    tags.AddTagString(MediaTag_Title, "Pending title");
    target.SetMetaInformation(tags);
    REQUIRE(state->metaCalls == 0);
    REQUIRE(target.Open({}));
    REQUIRE(state->metaCalls == 1);
    REQUIRE(state->tags.QueryTagString(MediaTag_Title).value() == "Pending title");
    REQUIRE(target.GetInputFormat().format == AudioDataFormat::PCM_S16);
    tags.AddTagString(MediaTag_Title, "Updated title");
    target.SetMetaInformation(tags);
    REQUIRE(state->metaCalls == 2);
    REQUIRE(state->tags.QueryTagString(MediaTag_Title).value() == "Updated title");
    target.Close();
    REQUIRE_FALSE(state->opened);
    REQUIRE(target.GetInputFormat().format == AudioDataFormat::UNKNOWN);
    tags.AddTagString(MediaTag_Title, "Reopened title");
    target.SetMetaInformation(tags);
    REQUIRE(state->metaCalls == 2);
    REQUIRE(target.Open({}));
    REQUIRE(state->tags.QueryTagString(MediaTag_Title).value() == "Reopened title");
}

TEST_CASE("Audio target cleans up failed Open and retries pending metadata", "[encoder][target]")
{
    auto state = std::make_shared<EncoderState>();
    CAudioTarget target(MakeMemoryBufStream(4096, false), std::make_unique<CTestEncoder>(state));
    CMediaTag tags;
    tags.AddTagString(MediaTag_Title, "Retry title");
    target.SetMetaInformation(tags);
    SECTION("Encoder Open fails") { state->openResult = false; }
    SECTION("Encoder rejects metadata") { state->metaResult = false; }
    REQUIRE_FALSE(target.Open({}));
    REQUIRE_FALSE(state->opened);
    REQUIRE(state->closeCalls == 1);
    REQUIRE(target.GetInputFormat().format == AudioDataFormat::UNKNOWN);
    state->openResult = true;
    state->metaResult = true;
    REQUIRE(target.Open({}));
    REQUIRE(state->tags.QueryTagString(MediaTag_Title).value() == "Retry title");
    state->metaResult = false;
    REQUIRE_THROWS_AS(target.SetMetaInformation(tags), std::runtime_error);
}

TEST_CASE("Audio target starts with negotiated format and locks it until Close", "[encoder][target]")
{
    auto state = std::make_shared<EncoderState>();
    {
        CAudioTarget target(MakeMemoryBufStream(4096, false), std::make_unique<CTestEncoder>(state));
        AudioFormat negotiated{};
        InitAudioFormat(&negotiated, AudioDataFormat::Float32, 1, 48000);
        const float samples[]{ -0.5f, 0, 0.5f };
        const auto* bytes = reinterpret_cast<const uint8_t*>(samples);
        REQUIRE_FALSE(target.Start(negotiated));
        REQUIRE(target.WriteBuffer(bytes, 3, negotiated) == 0);
        REQUIRE(target.FlushBuffer() == 0);
        REQUIRE(target.Open({}));
        REQUIRE_FALSE(target.Open({}));
        REQUIRE(target.WriteBuffer(bytes, 3, negotiated) == 0);
        REQUIRE(target.Start(negotiated));
        REQUIRE(IsSameAudioFormat(&state->startedFmt, &negotiated));
        REQUIRE_FALSE(target.Start(negotiated));
        REQUIRE(target.WriteBuffer(bytes, 3, negotiated) == 3);
        auto changed = negotiated;
        changed.sampleRate = 44100;
        REQUIRE_THROWS_AS(target.WriteBuffer(bytes, 3, changed), std::runtime_error);
        REQUIRE(state->encodeCalls == 1);
        SECTION("Successful Stop") { REQUIRE(target.FlushBuffer() == 1); }
        SECTION("Failed Stop")
        {
            state->stopResult = false;
            REQUIRE(target.FlushBuffer() == 0);
        }
        REQUIRE(target.WriteBuffer(bytes, 3, negotiated) == 0);
        REQUIRE_FALSE(target.Start(negotiated));
    }
    REQUIRE(state->closeCalls == 1);
    REQUIRE_FALSE(state->opened);
}

TEST_CASE("Conversion negotiates audio format before starting and finalizes WAV output", "[encoder][convert]")
{
    const auto input = std::filesystem::temp_directory_path() / "iplayer_encoder_convert_input.wav";
    const auto output = std::filesystem::temp_directory_path() / "iplayer_encoder_convert_output.wav";
    struct Cleanup
    {
        std::filesystem::path input;
        std::filesystem::path output;
        ~Cleanup()
        {
            std::error_code error;
            std::filesystem::remove(input, error);
            std::filesystem::remove(output, error);
        }
    } cleanup{ input, output };
    {
        SF_INFO info{};
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        info.channels = 2;
        info.samplerate = 48000;
        SndfilePtr file(sf_wchar_open(input.c_str(), SFM_WRITE, &info), &sf_close);
        REQUIRE(file != nullptr);
        std::vector<short> samples(4800 * 2);
        for (std::size_t frame = 0; frame < 4800; ++frame) {
            samples[frame * 2] = samples[frame * 2 + 1] = static_cast<short>(
                16000 * std::sin(2 * 3.141592653589793 * 440 * frame / 48000));
        }
        REQUIRE(sf_writef_short(file.get(), samples.data(), 4800) == 4800);
    }
    std::ostringstream progressOutput;
    {
        auto* oldBuffer = std::cout.rdbuf(progressOutput.rdbuf());
        auto restore = MakeGuard([&] { std::cout.rdbuf(oldBuffer); });
        REQUIRE(ConvertMediaFileInterface(Utf16LeToLocalMBCS(input.wstring()),
            Utf16LeToLocalMBCS(output.wstring()), "wav", 48000, "f32", 1) == 0);
    }
    REQUIRE(progressOutput.str().find("Conversion progress:   0%") != std::string::npos);
    REQUIRE(progressOutput.str().find("100% (complete)") != std::string::npos);
    REQUIRE(progressOutput.str().back() == '\n');
    SF_INFO info{};
    SndfilePtr file(sf_wchar_open(output.c_str(), SFM_READ, &info), &sf_close);
    REQUIRE(file != nullptr);
    REQUIRE(info.channels == 1);
    REQUIRE(info.samplerate == 48000);
    REQUIRE((info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT);
    REQUIRE(info.frames == 4800);
    std::vector<float> actual(static_cast<std::size_t>(info.frames));
    REQUIRE(sf_readf_float(file.get(), actual.data(), info.frames) == info.frames);
    double energy = 0;
    for (const float sample : actual) energy += sample * sample;
    REQUIRE(energy / actual.size() > 0.01);
}

TEST_CASE("Conversion applies only explicit format options over encoder preferences", "[encoder][convert]")
{
    auto& factory = CEncoderFactory::GetInstance();
    EncoderItem item;
    item.name = "Conversion preference test encoder";
    item.type = ENCODE_TYPE_NATIVE;
    item.Creator = [](uint32_t streamFmt) { return std::make_unique<CWavEncoder>(streamFmt); };
    item.QueryFormats = [] { return std::vector<EncoderFormatDefine>{ { StreamFormatWav, "Test WAV", ".negowav" } }; };
    item.QueryParamters = [](uint32_t) {
        auto defs = CWavEncoder::GetParameterDefine();
        for (auto& def : defs) {
            if (def.GetName() == EncoderParamName::CodeFormat) {
                def.SetDefaultValue(static_cast<uint32_t>(AudioDataFormat::Float32));
            }
            else if (def.GetName() == EncoderParamName::BitsPerSample) {
                def.SetDefaultValue(uint32_t(32));
            }
            else if (def.GetName() == EncoderParamName::SampleRates) {
                def.SetDefaultValue(uint32_t(32000));
            }
            else if (def.GetName() == EncoderParamName::Channels) {
                def.SetDefaultValue(uint32_t(1));
            }
        }
        return defs;
    };
    REQUIRE(factory.AddEncoderItem(item));
    auto unregister = MakeGuard([&] { factory.RemoveEncoderItem(item.name); });

    const auto input = std::filesystem::temp_directory_path() / "iplayer_convert_preferences_input.wav";
    const auto output = std::filesystem::temp_directory_path() / "iplayer_convert_preferences_output.wav";
    auto cleanup = MakeGuard([&] {
        std::error_code error;
        std::filesystem::remove(input, error);
        std::filesystem::remove(output, error);
    });
    {
        SF_INFO info{};
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        info.channels = 2;
        info.samplerate = 48000;
        SndfilePtr file(sf_wchar_open(input.c_str(), SFM_WRITE, &info), &sf_close);
        REQUIRE(file != nullptr);
        std::vector<short> samples(4800 * 2);
        for (std::size_t frame = 0; frame < 4800; ++frame) {
            samples[frame * 2] = samples[frame * 2 + 1] = static_cast<short>(
                16000 * std::sin(2 * 3.141592653589793 * 440 * frame / 48000));
        }
        REQUIRE(sf_writef_short(file.get(), samples.data(), 4800) == 4800);
    }

    // 全部省略、各自单独指定、两两指定以及全部指定，共八种组合。
    for (unsigned mask = 0; mask < 8; ++mask) {
        CAPTURE(mask);
        const auto sampleRate = (mask & 1) ? std::optional<uint32_t>(48000) : std::nullopt;
        const auto cfmt = (mask & 2) ? std::optional<std::string>("S24") : std::nullopt;
        const auto channels = (mask & 4) ? std::optional<uint32_t>(2) : std::nullopt;
        REQUIRE(ConvertMediaFileInterface(Utf16LeToLocalMBCS(input.wstring()),
            Utf16LeToLocalMBCS(output.wstring()), "negowav", sampleRate, cfmt, channels) == 0);

        SF_INFO info{};
        SndfilePtr file(sf_wchar_open(output.c_str(), SFM_READ, &info), &sf_close);
        REQUIRE(file != nullptr);
        REQUIRE(info.samplerate == ((mask & 1) ? 48000 : 32000));
        REQUIRE(info.channels == ((mask & 4) ? 2 : 1));
        REQUIRE((info.format & SF_FORMAT_SUBMASK) == ((mask & 2) ? SF_FORMAT_PCM_24 : SF_FORMAT_FLOAT));
        REQUIRE(info.frames == info.samplerate / 10);
        std::vector<float> samples(static_cast<std::size_t>(info.frames * info.channels));
        REQUIRE(sf_readf_float(file.get(), samples.data(), info.frames) == info.frames);
        double energy = 0;
        for (float sample : samples) energy += sample * sample;
        REQUIRE(energy / samples.size() > 0.01);
    }
}

TEST_CASE("Conversion rejects explicit invalid options instead of using defaults", "[encoder][convert]")
{
    REQUIRE_THROWS_WITH(ConvertMediaFileInterface("missing.wav", "unused.wav", "wav", 0), "invalid sample rate");
    REQUIRE_THROWS_WITH(ConvertMediaFileInterface("missing.wav", "unused.wav", "wav", std::nullopt, std::nullopt, 0), "invalid channels");
    REQUIRE_THROWS_WITH(ConvertMediaFileInterface("missing.wav", "unused.wav", "wav", std::nullopt, ""), "unsupported cfmt: ");
    REQUIRE_THROWS_WITH(ConvertMediaFileInterface("missing.wav", "unused.wav", "wav", std::nullopt, "invalid"), "unsupported cfmt: invalid");
}

TEST_CASE("Conversion does not report completion when encoder finalization fails", "[encoder][convert][progress]")
{
    auto state = std::make_shared<EncoderState>();
    state->stopResult = false;
    EncoderItem item;
    item.name = "Failing progress test encoder";
    item.type = ENCODE_TYPE_NATIVE;
    item.Creator = [state](uint32_t) { return std::make_unique<CTestEncoder>(state); };
    item.QueryFormats = [] { return std::vector<EncoderFormatDefine>{ { StreamFormatWav, "Test output", ".failprogress" } }; };
    item.QueryParamters = [](uint32_t) { return CWavEncoder::GetParameterDefine(); };
    auto& factory = CEncoderFactory::GetInstance();
    REQUIRE(factory.AddEncoderItem(item));
    auto unregister = MakeGuard([&] { factory.RemoveEncoderItem(item.name); });
    const auto input = std::filesystem::temp_directory_path() / "iplayer_progress_failure_input.wav";
    const auto output = std::filesystem::temp_directory_path() / "iplayer_progress_failure_output.wav";
    auto cleanup = MakeGuard([&] {
        std::error_code error;
        std::filesystem::remove(input, error);
        std::filesystem::remove(output, error);
    });
    {
        SF_INFO info{};
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        info.samplerate = 44100;
        info.channels = 2;
        SndfilePtr file(sf_wchar_open(input.c_str(), SFM_WRITE, &info), &sf_close);
        REQUIRE(file != nullptr);
        const short samples[]{ -123, 123, 456, -456 };
        REQUIRE(sf_writef_short(file.get(), samples, 2) == 2);
    }
    std::ostringstream progressOutput;
    {
        auto* oldBuffer = std::cout.rdbuf(progressOutput.rdbuf());
        auto restore = MakeGuard([&] { std::cout.rdbuf(oldBuffer); });
        REQUIRE_THROWS_WITH(ConvertMediaFileInterface(Utf16LeToLocalMBCS(input.wstring()),
            Utf16LeToLocalMBCS(output.wstring()), "failprogress"), "failed to stop encoder output");
    }
    REQUIRE(progressOutput.str().find("Conversion progress:") != std::string::npos);
    REQUIRE(progressOutput.str().find("100%") == std::string::npos);
    REQUIRE(progressOutput.str().back() == '\n');
}
