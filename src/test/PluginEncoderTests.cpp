#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <memory>
#include <stdexcept>
#include <utility>

#include "AudioTarget.h"
#include "DecoderFactory.h"
#include "EncoderDllWrapper.h"
#include "EncoderFactory.h"
#include "EncoderParamName.h"
#include "EncoderParamterDefineUtils.h"
#include "LibsndFunc.h"
#include "MemBufStream.h"
#include "PluginConfig.h"
#include "PluginEncoder.h"
#include "PluginsMgmt.h"
#include "ScopeGuard.h"
#include "StdFileSystem.h"
#include "UnicodeConvert.h"
#include "WavEncoder.h"

namespace
{
using SndfilePtr = std::unique_ptr<SNDFILE, decltype(&sf_close)>;

class CFailingOutputStream : public CMemoryBufStream
{
public:
    CFailingOutputStream() : CMemoryBufStream(false) {}
    void FailWrites() { m_failWrite = true; }
    uint32_t Write(const void* data, uint32_t size, uint32_t timeout = 0) override
    {
        return m_failWrite ? 0 : CMemoryBufStream::Write(data, size, timeout);
    }

private:
    bool m_failWrite = false;
};

std::wstring EncoderPath()
{
    return std::filesystem::path(SNDFILE_ENCODER_PATH).wstring();
}

bool IsDllPath(const std::filesystem::path& path)
{
    return _wcsicmp(path.extension().c_str(), L".dll") == 0;
}

SndfilePtr ReadOutput(CDataStream& stream, SF_INFO& info)
{
    stream.Seek(SeekBase::Begin, 0);
    return SndfilePtr(SndfileOpenStream(&stream, info), &sf_close);
}

DWORD RunPlayer(const std::filesystem::path& player, const std::wstring& arguments, const std::filesystem::path& workingDir)
{
    auto command = std::format(L"\"{}\" {}", player.wstring(), arguments);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    REQUIRE(::CreateProcessW(player.c_str(), command.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, workingDir.c_str(), &startup, &process));
    auto closeHandles = MakeGuard([&] {
        ::CloseHandle(process.hThread);
        ::CloseHandle(process.hProcess);
    });
    const auto wait = ::WaitForSingleObject(process.hProcess, 30000);
    if (wait != WAIT_OBJECT_0) {
        ::TerminateProcess(process.hProcess, 1);
        ::WaitForSingleObject(process.hProcess, 5000);
    }
    REQUIRE(wait == WAIT_OBJECT_0);
    DWORD result = 0;
    REQUIRE(::GetExitCodeProcess(process.hProcess, &result));
    return result;
}
}

TEST_CASE("Encoder wrapper validates exports and transfers module ownership", "[encoder][wrapper]")
{
    CEncoderDllWrapper empty;
    REQUIRE_FALSE(static_cast<bool>(empty));
    REQUIRE(empty.GetDllHostName().empty());
    REQUIRE(empty.GetPluginInformation(nullptr) < 0);
    REQUIRE(empty.OnRegister(nullptr, nullptr) < 0);
    REQUIRE(empty.GetEncodingFormatsDefine(nullptr) < 0);
    REQUIRE(empty.GetEncodingParamtersDefine(StreamFormatWav, nullptr) < 0);
    REQUIRE(empty.OnInitialize(nullptr) == nullptr);
    REQUIRE(empty.OpenEncoder(nullptr, nullptr) < 0);
    REQUIRE(empty.SetMetaInfo(nullptr, nullptr) < 0);
    REQUIRE(empty.StartEncoding(nullptr, nullptr) < 0);
    REQUIRE(empty.EncodeFrames(nullptr, nullptr, 1) == 0);
    REQUIRE(empty.StopEncoding(nullptr) < 0);
    REQUIRE(empty.GetErrorMessage().empty());
    empty.CloseEncoder(nullptr, nullptr);
    empty.OnRelease(nullptr);
    REQUIRE_FALSE(empty.Load((GetApplicationBasePath() / "imPlayer.exe").wstring()));
    REQUIRE(empty.GetDllHostName().empty());
    REQUIRE_FALSE(empty.Load(L"missing_encoder.opeplug"));
    REQUIRE(empty.Load(EncoderPath()));
    CEncoderDllWrapper moved(std::move(empty));
    REQUIRE_FALSE(static_cast<bool>(empty));
    REQUIRE(empty.GetDllHostName().empty());
    REQUIRE(static_cast<bool>(moved));
    empty = std::move(moved);
    REQUIRE_FALSE(static_cast<bool>(moved));
    PluginInfo info{};
    info.size = sizeof(info);
    REQUIRE(empty.GetPluginInformation(&info) == 0);
    REQUIRE(info.plug_type == PluginType::Encoder);
    REQUIRE(std::string(info.name) == "Libsndfile encoder");
    empty.Unload();
    empty.Unload();
    REQUIRE_FALSE(static_cast<bool>(empty));
}

TEST_CASE("Plugin discovery distinguishes decoder and encoder modules and configuration types", "[encoder][plugin-config]")
{
    auto encoder = CreatePluginObjectByFile(EncoderPath());
    REQUIRE(encoder.has_value());
    REQUIRE(encoder->type == "Encoder");
    REQUIRE(encoder->name == "Libsndfile encoder");
    REQUIRE(encoder->dll == nullptr);
    REQUIRE(encoder->encoderDll != nullptr);
    PluginConfig config{ encoder->name, encoder->publisher, encoder->type, Utf16ToUtf8(EncoderPath()) };
    REQUIRE(CreatePluginObjectByConfig(config).has_value());
    config.type = "Decoder";
    REQUIRE_FALSE(CreatePluginObjectByConfig(config).has_value());
    config.type = "Filter";
    REQUIRE_FALSE(CreatePluginObjectByConfig(config).has_value());
    REQUIRE_FALSE(CreatePluginObjectByFile(L"missing_plugin.opeplug").has_value());

    const auto decoderPath = GetApplicationBasePath() / "plugins" / "libsnd_decoder.ipdplus";
    auto decoder = CreatePluginObjectByFile(decoderPath.wstring());
    REQUIRE(decoder.has_value());
    REQUIRE(decoder->type == "Decoder");
    REQUIRE(decoder->dll != nullptr);
    REQUIRE(decoder->encoderDll == nullptr);
    REQUIRE_FALSE(std::get<0>(CDecoderFactory::GetInstance().AddPluginObject(*encoder)));
    REQUIRE_FALSE(std::get<0>(CEncoderFactory::GetInstance().AddPluginObject(*decoder)));
}

TEST_CASE("Encoder factory registers plugins with per-format parameters and deterministic native preference", "[encoder][factory]")
{
    auto& factory = CEncoderFactory::GetInstance();
    REQUIRE_FALSE(factory.GetEncoderPlugin(CWavEncoder::GetName()).has_value());
    REQUIRE_FALSE(std::get<0>(factory.RemovePluginObject(CWavEncoder::GetName())));
    REQUIRE_FALSE(std::get<0>(factory.AddPluginObject(PluginDllObject{})));
    auto object = CreatePluginObjectByFile(EncoderPath());
    REQUIRE(object.has_value());
    const auto [ok, message] = factory.AddPluginObject(*object);
    INFO(message);
    REQUIRE(ok);
    auto unregister = MakeGuard([&] { factory.RemovePluginObject(object->name); });
    REQUIRE_FALSE(std::get<0>(factory.AddPluginObject(*object)));
    const auto plugin = factory.GetEncoderPlugin(object->name);
    REQUIRE(plugin.has_value());
    REQUIRE_FALSE(plugin->hostfile.empty());
    REQUIRE(plugin->QueryFormats().size() > 20);
    const auto flac = plugin->QueryParamters(StreamFormatFlac);
    REQUIRE(HasEncoderParamterDefine(flac, EncoderParamName::CompressLevel));
    const auto ogg = plugin->QueryParamters(StreamFormatOgg);
    REQUIRE(HasEncoderParamterDefine(ogg, EncoderParamName::Quality));
    REQUIRE_FALSE(HasEncoderParamterDefine(ogg, EncoderParamName::CompressLevel));
    REQUIRE_THROWS_AS(plugin->QueryParamters(StreamFormatUnknown), std::runtime_error);
    REQUIRE(factory.MakeAudioEncoder(StreamFormatFlac)->GetType() == ENCODE_TYPE_PLUGIN);
    REQUIRE(factory.MakeAudioEncoder(StreamFormatWav)->GetType() == ENCODE_TYPE_NATIVE);
    REQUIRE(factory.MakeAudioEncoder(StreamFormatWav, object->name)->GetType() == ENCODE_TYPE_PLUGIN);
    REQUIRE(factory.MakeAudioEncoder(StreamFormatUnknown) == nullptr);
    REQUIRE(factory.MakeAudioEncoder(StreamFormatFlac, CWavEncoder::GetName()) == nullptr);
    const auto encoders = factory.GetEncoders();
    REQUIRE(encoders.front().name == CWavEncoder::GetName());
    REQUIRE(std::any_of(encoders.begin(), encoders.end(), [&](const EncoderItem& item) { return item.name == object->name; }));
}

TEST_CASE("Plugin encoder keeps its module alive after factory removal and encodes lossless audio", "[encoder][plugin]")
{
    auto& factory = CEncoderFactory::GetInstance();
    auto stream = MakeMemoryBufStream(65536, false);
    std::unique_ptr<CAudioEncoder> encoder;
    std::weak_ptr<CEncoderDllWrapper> module;
    {
        auto object = CreatePluginObjectByFile(EncoderPath());
        REQUIRE(object.has_value());
        module = object->encoderDll;
        REQUIRE(std::get<0>(factory.AddPluginObject(*object)));
        auto unregister = MakeGuard([&] { factory.RemovePluginObject(object->name); });
        encoder = factory.MakeAudioEncoder(StreamFormatFlac, object->name);
        REQUIRE(encoder != nullptr);
        REQUIRE(encoder->GetName() == object->name);
        REQUIRE(std::get<0>(factory.RemovePluginObject(object->name)));
        REQUIRE_FALSE(factory.GetEncoderItem(object->name).has_value());
    }
    REQUIRE_FALSE(module.expired());
    encoder->Init();
    encoder->Attach(stream.get());
    REQUIRE(encoder->Open({}));
    const auto format = encoder->GetTransFormat();
    REQUIRE(format.numChannels == 2);
    REQUIRE(encoder->Start(&format));
    const std::array<short, 10> samples{ -32768, 32767, -123, 123, 0, 0, 456, -456, 32767, -32768 };
    REQUIRE(encoder->Encode(samples.data(), 5) == 5);
    SECTION("Explicit Stop")
    {
        REQUIRE(encoder->Stop());
        REQUIRE(encoder->Stop());
        REQUIRE(encoder->Encode(samples.data(), 5) == 0);
        REQUIRE_FALSE(encoder->Start(&format));
    }
    SECTION("Destructor finalizes without Stop") {}
    encoder.reset();
    REQUIRE(module.expired());
    SF_INFO info{};
    auto decoded = ReadOutput(*stream, info);
    REQUIRE(decoded != nullptr);
    REQUIRE(info.frames == 5);
    REQUIRE((info.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_FLAC);
    REQUIRE(info.channels == 2);
    std::array<short, 10> actual{};
    REQUIRE(sf_readf_short(decoded.get(), actual.data(), 5) == 5);
    REQUIRE(actual == samples);
}

TEST_CASE("Plugin encoder validates lifecycle and recovers from a failed Open", "[encoder][plugin]")
{
    auto stream = MakeMemoryBufStream(4096, false);
    CPluginEncoder encoder(StreamFormatWav);
    REQUIRE(encoder.GetName() == CPluginEncoder::Name());
    REQUIRE_THROWS_AS(encoder.Init(), std::runtime_error);
    REQUIRE_THROWS_AS(encoder.AttachModule(nullptr), std::runtime_error);
    REQUIRE_FALSE(encoder.Open({}));
    REQUIRE_FALSE(encoder.Stop());
    REQUIRE_FALSE(encoder.Start(nullptr));
    REQUIRE(encoder.Encode(nullptr, 1) == 0);
    encoder.AttachModule(MakeEncoderWrapper(EncoderPath()));
    encoder.Init();
    REQUIRE_FALSE(encoder.Open({})); // 尚未 Attach 数据流。
    encoder.Attach(stream.get());
    const std::vector<EncoderParamter> invalid{
        { EncoderParamName::Channels, EncoderParamType::UnsignedInt, uint32_t(0) }
    };
    REQUIRE_THROWS_AS(encoder.Open(invalid), std::runtime_error);
    REQUIRE(encoder.GetTransFormat().format == AudioDataFormat::UNKNOWN);
    REQUIRE(encoder.Open({}));
    REQUIRE_FALSE(encoder.Open({}));
    auto format = encoder.GetTransFormat();
    auto mismatch = format;
    mismatch.sampleRate += 1;
    REQUIRE_THROWS_AS(encoder.Start(&mismatch), std::runtime_error);
    REQUIRE(encoder.Start(&format));
    REQUIRE_FALSE(encoder.Start(&format));
    const short samples[]{ 1234, -1234 };
    REQUIRE(encoder.Encode(samples, 1) == 1);
    encoder.Close();
    encoder.Close();
    REQUIRE(encoder.GetTransFormat().format == AudioDataFormat::UNKNOWN);
    SF_INFO info{};
    auto decoded = ReadOutput(*stream, info);
    REQUIRE(decoded != nullptr);
    REQUIRE(info.frames == 1);
    decoded.reset();
    auto another = MakeMemoryBufStream(4096, false);
    encoder.Attach(another.get());
    encoder.Init();
    REQUIRE(encoder.Open({}));
    format = encoder.GetTransFormat();
    REQUIRE(encoder.Start(&format));
    REQUIRE(encoder.Encode(samples, 1) == 1);
    REQUIRE(encoder.Stop());
}

TEST_CASE("Audio target passes cached metadata and negotiated input to an encoder plugin", "[encoder][target][plugin]")
{
    auto stream = std::make_unique<CMemoryBufStream>(false);
    REQUIRE(stream->Open(4096));
    auto* output = stream.get();
    auto encoder = std::make_unique<CPluginEncoder>(StreamFormatWav);
    encoder->AttachModule(MakeEncoderWrapper(EncoderPath()));
    CAudioTarget target(std::move(stream), std::move(encoder));
    CMediaTag tags;
    tags.AddTagString(MediaTag_Title, "Host metadata before Open");
    target.SetMetaInformation(tags);
    REQUIRE(target.Open({}));
    tags.AddTagString(MediaTag_Title, "Host metadata after Open");
    target.SetMetaInformation(tags);
    auto input = target.GetInputFormat();
    InitAudioFormat(&input, AudioDataFormat::Float32, input.numChannels, input.sampleRate);
    REQUIRE(target.Start(input));
    REQUIRE(input.numChannels == 2);
    const float samples[]{ -0.5f, 0.5f, 0, 0, 0.5f, -0.5f };
    REQUIRE(target.WriteBuffer(reinterpret_cast<const uint8_t*>(samples), 3, input) == 3);
    REQUIRE(target.FlushBuffer() == 1);
    target.Close();
    SF_INFO info{};
    auto decoded = ReadOutput(*output, info);
    REQUIRE(decoded != nullptr);
    REQUIRE(info.frames == 3);
    const auto* title = sf_get_string(decoded.get(), SF_STR_TITLE);
    REQUIRE(title != nullptr);
    REQUIRE(std::string(title) == "Host metadata after Open");
    short actual[6]{};
    REQUIRE(sf_readf_short(decoded.get(), actual, 3) == 3);
    REQUIRE(actual[0] == -16384);
    REQUIRE(actual[1] == 16384);
    REQUIRE(actual[2] == 0);
    REQUIRE(actual[3] == 0);
    REQUIRE(actual[4] == 16384);
    REQUIRE(actual[5] == -16384);
}

TEST_CASE("Plugin encoder reports output failure during Stop and prevents restarting", "[encoder][plugin]")
{
    CFailingOutputStream stream;
    REQUIRE(stream.Open(65536));
    CPluginEncoder encoder(StreamFormatFlac);
    encoder.AttachModule(MakeEncoderWrapper(EncoderPath()));
    encoder.Init();
    encoder.Attach(&stream);
    REQUIRE(encoder.Open({}));
    const auto format = encoder.GetTransFormat();
    REQUIRE(encoder.Start(&format));
    const short samples[]{ -123, 123, 0, 0, 456, -456 };
    REQUIRE(encoder.Encode(samples, 3) == 3);
    stream.FailWrites();
    REQUIRE_THROWS_AS(encoder.Stop(), std::runtime_error);
    REQUIRE_FALSE(encoder.Stop());
    REQUIRE_FALSE(encoder.Start(&format));
    REQUIRE(encoder.Encode(samples, 3) == 0);
    encoder.Close();
    REQUIRE(encoder.GetTransFormat().format == AudioDataFormat::UNKNOWN);
}

TEST_CASE("Host scans plugins and converts with explicit or negotiated format options", "[encoder][integration]")
{
    const auto sourceBin = GetApplicationBasePath();
    const auto sandbox = std::filesystem::temp_directory_path() /
        std::format(L"iplayer_encoder_host_{}_{}", ::GetCurrentProcessId(), ::GetTickCount64());
    REQUIRE(std::filesystem::create_directory(sandbox));
    auto cleanup = MakeGuard([&] {
        std::error_code error;
        std::filesystem::remove_all(sandbox, error);
    });
    for (const auto& entry : std::filesystem::directory_iterator(sourceBin)) {
        if (entry.is_regular_file() && (IsDllPath(entry.path()) || entry.path().filename() == "imPlayer.exe")) {
            std::filesystem::copy_file(entry.path(), sandbox / entry.path().filename());
        }
    }
    std::filesystem::create_directory(sandbox / "plugins");
    for (const auto& entry : std::filesystem::directory_iterator(sourceBin / "plugins")) {
        if (entry.is_regular_file() && IsDllPath(entry.path())) {
            std::filesystem::copy_file(entry.path(), sandbox / "plugins" / entry.path().filename());
        }
    }
    std::filesystem::copy_file(EncoderPath(), sandbox / "plugins" / "sndfile_encoder.opeplug");
    std::filesystem::copy_file(sourceBin / "plugins" / "libsnd_decoder.ipdplus", sandbox / "plugins" / "libsnd_decoder.ipdplus");
    REQUIRE(RunPlayer(sandbox / "imPlayer.exe", L"--rd", sandbox) == 0);
    std::vector<PluginConfig> plugins;
    REQUIRE(LoadPluginConfigFile(Utf16LeToLocalMBCS((sandbox / "config" / "plugin.config").wstring()), plugins));
    REQUIRE(plugins.size() == 2);
    REQUIRE(std::any_of(plugins.begin(), plugins.end(), [](const PluginConfig& plugin) {
        return plugin.type == "Encoder" && plugin.name == "Libsndfile encoder" && plugin.hostfile == "sndfile_encoder.opeplug";
    }));
    REQUIRE(std::any_of(plugins.begin(), plugins.end(), [](const PluginConfig& plugin) { return plugin.type == "Decoder"; }));

    const auto input = sandbox / "input.wav";
    const std::array<short, 8> pattern{ -32768, 32767, -1000, 1000, 0, 0, -123, 456 };
    std::vector<short> samples(4410 * 2);
    for (std::size_t i = 0; i < samples.size(); ++i) samples[i] = pattern[i % pattern.size()];
    {
        SF_INFO info{};
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        info.channels = 2;
        info.samplerate = 44100;
        SndfilePtr file(sf_wchar_open(input.c_str(), SFM_WRITE, &info), &sf_close);
        REQUIRE(file != nullptr);
        REQUIRE(sf_writef_short(file.get(), samples.data(), 4410) == 4410);
    }
    const auto output = sandbox / "output.audio";
    std::wstring format;
    int expectedType = 0;
    int expectedRate = 44100;
    int expectedChannels = 2;
    int expectedSubtype = SF_FORMAT_PCM_16;
    bool exactSamples = true;
    std::wstring options = L"--srate=44100 --cfmt=S16 --channel=2";
    SECTION("Case insensitive FLAC extension") { format = L".FLAC"; expectedType = SF_FORMAT_FLAC; }
    SECTION("AIFF format") { format = L"aiff"; expectedType = SF_FORMAT_AIFF; }
    SECTION("WVE keeps codec and rate defaults with explicit mono channels")
    {
        format = L"wve";
        expectedType = SF_FORMAT_WVE;
        expectedSubtype = SF_FORMAT_ALAW;
        expectedRate = 8000;
        expectedChannels = 1;
        exactSamples = false;
        options = L"--channel=1";
    }
    SECTION("FLAC defaults to stereo when all options are omitted")
    {
        format = L"flac";
        expectedType = SF_FORMAT_FLAC;
        options.clear();
    }
    SECTION("Only sample rate is overridden")
    {
        format = L"flac";
        expectedType = SF_FORMAT_FLAC;
        expectedRate = 48000;
        exactSamples = false;
        options = L"--srate=48000";
    }
    SECTION("Only sample data format is overridden")
    {
        format = L"aiff";
        expectedType = SF_FORMAT_AIFF;
        expectedSubtype = SF_FORMAT_FLOAT;
        exactSamples = false;
        options = L"--cfmt=F32";
    }
    SECTION("Only channel count is overridden")
    {
        format = L"flac";
        expectedType = SF_FORMAT_FLAC;
        expectedChannels = 1;
        exactSamples = false;
        options = L"--channel=1";
    }
    const auto arguments = std::format(L"--convert --filename=\"{}\" --out=\"{}\" --ffmt={} {}",
        input.wstring(), output.wstring(), format, options);
    REQUIRE(RunPlayer(sandbox / "imPlayer.exe", arguments, sandbox) == 0);
    SF_INFO info{};
    SndfilePtr file(sf_wchar_open(output.c_str(), SFM_READ, &info), &sf_close);
    REQUIRE(file != nullptr);
    REQUIRE((info.format & SF_FORMAT_TYPEMASK) == expectedType);
    REQUIRE((info.format & SF_FORMAT_SUBMASK) == expectedSubtype);
    REQUIRE(info.frames == expectedRate / 10);
    REQUIRE(info.channels == expectedChannels);
    REQUIRE(info.samplerate == expectedRate);
    if (exactSamples) {
        std::vector<short> actual(samples.size());
        REQUIRE(sf_readf_short(file.get(), actual.data(), info.frames) == info.frames);
        REQUIRE(actual == samples);
    }
}
