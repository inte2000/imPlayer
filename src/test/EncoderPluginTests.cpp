#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>

#include "PluginMain.h"
#include "iplugsdk.h"

// Preserve the existing decoder SDK layout after extracting shared definitions.
static_assert(sizeof(ApplicationConfig) == 12);
static_assert(sizeof(PluginInfo) == 208);
static_assert(sizeof(FileFormatReg) == 260);
static_assert(sizeof(PluginRegister) == 33288);
static_assert(offsetof(AudioMetaTags, tags) == 8);
static_assert(alignof(AudioMetaTags) == 4);

namespace
{
template <typename TFunction>
TFunction ResolvePluginFunction(HMODULE module, const char* name)
{
    const auto function = GetProcAddress(module, name);
    INFO(name);
    REQUIRE(function != nullptr);
    return reinterpret_cast<TFunction>(function);
}
}

TEST_CASE("Empty encoder plugin exposes its SDK entry points", "[encoder][plugin]")
{
    const std::filesystem::path pluginPath(SNDFILE_ENCODER_PATH);
    REQUIRE(pluginPath.extension() == ".opeplug");
    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)> module(
        LoadLibraryW(pluginPath.c_str()), &FreeLibrary);
    REQUIRE(module != nullptr);

    const char* exports[] = {
        "Plug_OnRegister", "Plug_GetErrMessage", "Plug_GetPluginInformation",
        "Plug_GetEncodingFormatsDefine", "Plug_GetEncodingParamtersDefine",
        "Plug_OnInitialize", "Plug_OpenEncoder", "Plug_CloseEncoder",
        "Plug_SetMetaInfo", "Plug_StartEncoding", "Plug_EncodeFrames", "Plug_StopEncoding"
    };
    for (const auto* name : exports) {
        INFO(name);
        REQUIRE(GetProcAddress(module.get(), name) != nullptr);
    }

    const auto getError = ResolvePluginFunction<decltype(&Plug_GetErrMessage)>(module.get(), "Plug_GetErrMessage");
    const auto getInfo = ResolvePluginFunction<decltype(&Plug_GetPluginInformation)>(module.get(), "Plug_GetPluginInformation");
    const auto onRegister = ResolvePluginFunction<decltype(&Plug_OnRegister)>(module.get(), "Plug_OnRegister");
    const auto getFormats = ResolvePluginFunction<decltype(&Plug_GetEncodingFormatsDefine)>(module.get(), "Plug_GetEncodingFormatsDefine");
    const auto getParameters = ResolvePluginFunction<decltype(&Plug_GetEncodingParamtersDefine)>(module.get(), "Plug_GetEncodingParamtersDefine");

    SECTION("Metadata identifies an encoder with no supported formats")
    {
        PluginInfo info{};
        info.size = sizeof(info);
        REQUIRE(getInfo(&info) == 0);
        REQUIRE(info.plug_type == PluginType::Encoder);
        REQUIRE(std::string(info.name) == "Libsndfile encoder");
        REQUIRE(std::string(info.publisher) == "imPlayer Group");

        ApplicationConfig app{};
        PluginRegister registration{};
        registration.size = sizeof(registration);
        registration.format_count = 1;
        REQUIRE(onRegister(&app, &registration) == 0);
        REQUIRE(registration.format_count == 0);

        PluginEncodingFormatsDefine formats{};
        formats.size = sizeof(formats);
        formats.fmtDef.push_back({ StreamFormatWav, "stale", ".wav" });
        REQUIRE(getFormats(&formats) == 0);
        REQUIRE(formats.fmtDef.empty());

        PluginEncodingParamtersDefine parameters{};
        parameters.size = sizeof(parameters);
        parameters.paramDef.emplace_back("stale", EncoderParamType::UnsignedInt, uint32_t(16));
        REQUIRE(getParameters(StreamFormatWav, &parameters) < 0);
        REQUIRE(parameters.paramDef.empty());
    }

    SECTION("Queries reject null pointers and incompatible structure sizes")
    {
        ApplicationConfig app{};
        PluginRegister registration{};
        PluginInfo info{};
        PluginEncodingFormatsDefine formats{};
        PluginEncodingParamtersDefine parameters{};
        REQUIRE(onRegister(&app, nullptr) < 0);
        REQUIRE(onRegister(nullptr, &registration) < 0);
        REQUIRE(onRegister(&app, &registration) < 0);
        REQUIRE(getInfo(nullptr) < 0);
        REQUIRE(getInfo(&info) < 0);
        REQUIRE(getFormats(nullptr) < 0);
        REQUIRE(getFormats(&formats) < 0);
        REQUIRE(getParameters(StreamFormatWav, nullptr) < 0);
        REQUIRE(getParameters(StreamFormatWav, &parameters) < 0);

        char error[256] = {};
        getError(error, sizeof(error));
        REQUIRE_FALSE(std::string(error).empty());
        char shortError[2] = {};
        getError(shortError, sizeof(shortError));
        REQUIRE(shortError[0] != '\0');
        REQUIRE(shortError[1] == '\0');
        char untouched = 'x';
        getError(&untouched, 0);
        REQUIRE(untouched == 'x');
        getError(nullptr, 0);
        getError(nullptr, sizeof(error));
    }

    SECTION("Empty encoding entry points report unsupported operations")
    {
        const auto initialize = ResolvePluginFunction<decltype(&Plug_OnInitialize)>(module.get(), "Plug_OnInitialize");
        const auto open = ResolvePluginFunction<decltype(&Plug_OpenEncoder)>(module.get(), "Plug_OpenEncoder");
        const auto close = ResolvePluginFunction<decltype(&Plug_CloseEncoder)>(module.get(), "Plug_CloseEncoder");
        const auto setMetaInfo = ResolvePluginFunction<decltype(&Plug_SetMetaInfo)>(module.get(), "Plug_SetMetaInfo");
        const auto start = ResolvePluginFunction<decltype(&Plug_StartEncoding)>(module.get(), "Plug_StartEncoding");
        const auto encode = ResolvePluginFunction<decltype(&Plug_EncodeFrames)>(module.get(), "Plug_EncodeFrames");
        const auto stop = ResolvePluginFunction<decltype(&Plug_StopEncoding)>(module.get(), "Plug_StopEncoding");

        EncoderPluginInitialize init{};
        init.size = sizeof(init);
        REQUIRE(initialize(&init) == nullptr);
        REQUIRE(initialize(nullptr) == nullptr);
        EncoderPluginOpen openParams{};
        openParams.size = sizeof(openParams);
        REQUIRE(open(nullptr, &openParams) < 0);
        AudioMetaTags tags{};
        tags.size = sizeof(tags);
        REQUIRE(setMetaInfo(nullptr, &tags) < 0);
        AudioFormat audioFmt{};
        REQUIRE(start(nullptr, &audioFmt) < 0);
        const float samples[2] = {};
        REQUIRE(encode(nullptr, samples, 1) == 0);
        REQUIRE(stop(nullptr) < 0);
        EncoderPluginClose closeParams{};
        closeParams.size = sizeof(closeParams);
        close(nullptr, &closeParams);
        close(nullptr, nullptr);

        char error[256] = {};
        getError(error, sizeof(error));
        REQUIRE(std::string(error).find("not implemented") != std::string::npos);
    }
}
