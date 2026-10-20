#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>

#include "PluginMain.h"
#include "EncoderParamName.h"
#include "EncoderParamterDefineUtils.h"
#include "LibsndFunc.h"
#include "MemBufStream.h"
#include "iplugsdk.h"

static_assert(sizeof(ApplicationConfig) == 12);
static_assert(sizeof(PluginInfo) == 208);
static_assert(sizeof(FileFormatReg) == 260);
static_assert(sizeof(PluginRegister) == 33288);
static_assert(offsetof(AudioMetaTags, tags) == 8);
static_assert(alignof(AudioMetaTags) == 4);

namespace
{
class CEncoderPlugin
{
public:
    CEncoderPlugin() : m_module(LoadLibraryW(std::filesystem::path(SNDFILE_ENCODER_PATH).c_str()), &FreeLibrary)
    {
        REQUIRE(m_module != nullptr);
    }

    template <typename TFunction>
    TFunction Get(const char* name)
    {
        INFO(name);
        const auto function = GetProcAddress(m_module.get(), name);
        REQUIRE(function != nullptr);
        return reinterpret_cast<TFunction>(function);
    }

private:
    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)> m_module;
};
}

TEST_CASE("Sndfile encoder plugin exports metadata and format dependent parameters", "[encoder][plugin]")
{
    CEncoderPlugin plugin;
    const auto infoFn = plugin.Get<decltype(&Plug_GetPluginInformation)>("Plug_GetPluginInformation");
    PluginInfo info{};
    info.size = sizeof(info);
    REQUIRE(infoFn(&info) == 0);
    REQUIRE(info.plug_type == PluginType::Encoder);
    REQUIRE(std::string(info.name) == "Libsndfile encoder");

    ApplicationConfig app{};
    PluginRegister registration{};
    registration.size = sizeof(registration);
    REQUIRE(plugin.Get<decltype(&Plug_OnRegister)>("Plug_OnRegister")(&app, &registration) == 0);
    PluginEncodingFormatsDefine formats{};
    formats.size = sizeof(formats);
    REQUIRE(plugin.Get<decltype(&Plug_GetEncodingFormatsDefine)>("Plug_GetEncodingFormatsDefine")(&formats) == 0);
    REQUIRE(formats.fmtDef.size() > 20);
    REQUIRE(registration.format_count == formats.fmtDef.size());
    for (std::size_t i = 0; i < formats.fmtDef.size(); ++i) {
        REQUIRE(registration.fmt_reg[i].id == formats.fmtDef[i].streamFmt);
        REQUIRE(std::string(registration.fmt_reg[i].ext_list) == formats.fmtDef[i].extname);
    }
    PluginEncodingParamtersDefine parameters{};
    parameters.size = sizeof(parameters);
    const auto query = plugin.Get<decltype(&Plug_GetEncodingParamtersDefine)>("Plug_GetEncodingParamtersDefine");
    REQUIRE(query(StreamFormatFlac, &parameters) == 0);
    REQUIRE(HasEncoderParamterDefine(parameters.paramDef, EncoderParamName::CompressLevel));
    REQUIRE(query(StreamFormatOgg, &parameters) == 0);
    REQUIRE(HasEncoderParamterDefine(parameters.paramDef, EncoderParamName::Quality));
    REQUIRE_FALSE(HasEncoderParamterDefine(parameters.paramDef, EncoderParamName::CompressLevel));
    REQUIRE(query(StreamFormatUnknown, &parameters) < 0);
    REQUIRE(parameters.paramDef.empty());
}

TEST_CASE("Sndfile encoder plugin validates ABI parameters and bounds error messages", "[encoder][plugin]")
{
    CEncoderPlugin plugin;
    const auto info = plugin.Get<decltype(&Plug_GetPluginInformation)>("Plug_GetPluginInformation");
    const auto queryFormats = plugin.Get<decltype(&Plug_GetEncodingFormatsDefine)>("Plug_GetEncodingFormatsDefine");
    const auto queryParams = plugin.Get<decltype(&Plug_GetEncodingParamtersDefine)>("Plug_GetEncodingParamtersDefine");
    const auto onRegister = plugin.Get<decltype(&Plug_OnRegister)>("Plug_OnRegister");
    ApplicationConfig app{};
    PluginInfo badInfo{};
    PluginRegister badReg{};
    PluginEncodingFormatsDefine badFormats{};
    PluginEncodingParamtersDefine badParams{};
    REQUIRE(info(nullptr) < 0);
    REQUIRE(info(&badInfo) < 0);
    REQUIRE(onRegister(&app, nullptr) < 0);
    REQUIRE(onRegister(nullptr, &badReg) < 0);
    REQUIRE(onRegister(&app, &badReg) < 0);
    REQUIRE(queryFormats(nullptr) < 0);
    REQUIRE(queryFormats(&badFormats) < 0);
    REQUIRE(queryParams(StreamFormatWav, nullptr) < 0);
    REQUIRE(queryParams(StreamFormatWav, &badParams) < 0);
    const auto getError = plugin.Get<decltype(&Plug_GetErrMessage)>("Plug_GetErrMessage");
    char error[256]{};
    getError(error, sizeof(error));
    REQUIRE_FALSE(std::string(error).empty());
    char shortError[2]{};
    getError(shortError, sizeof(shortError));
    REQUIRE(shortError[0] != '\0');
    REQUIRE(shortError[1] == '\0');
    char untouched = 'x';
    getError(&untouched, 0);
    REQUIRE(untouched == 'x');
    getError(nullptr, 0);
    getError(nullptr, sizeof(error));
}

TEST_CASE("Sndfile encoder plugin owns context and finalizes actual PCM output", "[encoder][plugin]")
{
    CEncoderPlugin plugin;
    const auto initialize = plugin.Get<decltype(&Plug_OnInitialize)>("Plug_OnInitialize");
    const auto release = plugin.Get<decltype(&Plug_OnRelease)>("Plug_OnRelease");
    const auto open = plugin.Get<decltype(&Plug_OpenEncoder)>("Plug_OpenEncoder");
    const auto close = plugin.Get<decltype(&Plug_CloseEncoder)>("Plug_CloseEncoder");
    const auto setTags = plugin.Get<decltype(&Plug_SetMetaInfo)>("Plug_SetMetaInfo");
    const auto start = plugin.Get<decltype(&Plug_StartEncoding)>("Plug_StartEncoding");
    const auto encode = plugin.Get<decltype(&Plug_EncodeFrames)>("Plug_EncodeFrames");
    const auto stop = plugin.Get<decltype(&Plug_StopEncoding)>("Plug_StopEncoding");
    const auto error = plugin.Get<decltype(&Plug_GetErrMessage)>("Plug_GetErrMessage");
    REQUIRE(initialize(nullptr) == nullptr);
    EncoderPluginInitialize init{};
    REQUIRE(initialize(&init) == nullptr);
    init.size = sizeof(init);
    auto stream = MakeMemoryBufStream(4096, false);
    std::unique_ptr<void, decltype(release)> ctx(initialize(&init), release);
    REQUIRE(ctx != nullptr);
    const auto* header = static_cast<const AudioEncodeContextHeader*>(ctx.get());
    REQUIRE(header->size == sizeof(AudioEncodeContextHeader));
    REQUIRE(header->transFmt.format == AudioDataFormat::UNKNOWN);

    AudioMetaTags tags{};
    tags.size = sizeof(tags);
    tags.tags.AddTagString(MediaTag_Title, "Encoded through plugin ABI");
    tags.tags.AddTagInteger(MediaTag_Tracks, 7);
    REQUIRE(setTags(ctx.get(), &tags) == 0);
    EncoderPluginOpen openParams{};
    REQUIRE(openParams.mediaStreamIdx == 0);
    openParams.size = sizeof(openParams);
    openParams.streamFmt = StreamFormatWav;
    openParams.pStream = stream.get();
    openParams.mediaStreamIdx = 1;
    REQUIRE(open(ctx.get(), &openParams) < 0);
    openParams.mediaStreamIdx = 0;
    REQUIRE(open(ctx.get(), &openParams) == 0);
    REQUIRE(header->transFmt.format == AudioDataFormat::PCM_S16);
    REQUIRE(header->transFmt.numChannels == 2);
    REQUIRE(open(ctx.get(), &openParams) < 0);
    REQUIRE(encode(ctx.get(), nullptr, 1) == 0);
    REQUIRE(start(nullptr, &header->transFmt) < 0);
    REQUIRE(start(ctx.get(), nullptr) < 0);
    AudioFormat mismatch = header->transFmt;
    mismatch.sampleRate = 48000;
    REQUIRE(start(ctx.get(), &mismatch) < 0);
    REQUIRE(start(ctx.get(), &header->transFmt) == 0);
    tags.streamIdx = 1;
    REQUIRE(setTags(ctx.get(), &tags) < 0);
    const short samples[] = { -32768, 32767, -123, 123, 0, 0, 456, -456, 32767, -32768 };
    REQUIRE(encode(ctx.get(), samples, 5) == 5);

    SECTION("Stop and Close finalize the file, and the context can be reused")
    {
        REQUIRE(stop(ctx.get()) == 0);
        EncoderPluginClose closeParams{};
        REQUIRE(closeParams.mediaStreamIdx == 0);
        closeParams.size = sizeof(closeParams);
        closeParams.mediaStreamIdx = 1;
        close(ctx.get(), &closeParams);
        char message[256]{};
        error(message, sizeof(message));
        REQUIRE_FALSE(std::string(message).empty());
        closeParams.mediaStreamIdx = 0;
        close(ctx.get(), &closeParams);
        REQUIRE(header->transFmt.format == AudioDataFormat::UNKNOWN);
        REQUIRE(stop(ctx.get()) < 0);
        auto another = MakeMemoryBufStream(4096, false);
        openParams.pStream = another.get();
        REQUIRE(open(ctx.get(), &openParams) == 0);
        REQUIRE(start(ctx.get(), &header->transFmt) == 0);
        REQUIRE(encode(ctx.get(), samples, 5) == 5);
        close(ctx.get(), &closeParams); // Finalize without an explicit Stop.
        ctx.reset();
    }
    SECTION("Release finalizes an open encoder without Stop or Close")
    {
        ctx.reset();
    }

    stream->Seek(SeekBase::Begin, 0);
    SF_INFO info{};
    std::unique_ptr<SNDFILE, decltype(&sf_close)> file(SndfileOpenStream(stream.get(), info), &sf_close);
    REQUIRE(file != nullptr);
    REQUIRE(info.frames == 5);
    REQUIRE(info.channels == 2);
    REQUIRE(std::string(sf_get_string(file.get(), SF_STR_TITLE)) == "Encoded through plugin ABI");
    REQUIRE(std::string(sf_get_string(file.get(), SF_STR_TRACKNUMBER)) == "7");
    short decoded[10]{};
    REQUIRE(sf_readf_short(file.get(), decoded, 5) == 5);
    for (int i = 0; i < 10; ++i) REQUIRE(decoded[i] == samples[i]);
    release(nullptr);
}
