/*
20260508 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_35.txt

20260519 修改配置加载方式
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_37.txt
任务描述：todo_task_38.txt
*/
#include <string>
#include <cstring>
#include <memory>
#include <optional>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <windows.h>
#include "PlugMain.h"
#include "LibvgmFunc.h"
#include "LibVgmPlayCtrl.h"
#include "PluginConfig.h"
#include "PluginConfigFile.h"

const char8_t* plugname = u8"Libvgm decoder";
const char8_t* plugpublisher = u8"imPlayer Group";
uint32_t FileTypeIdBegin = 0;

typedef struct tagFileExtRegItem
{
    uint32_t st;
    const char* desc;
    const char* extList;
} FileExtRegItem;

typedef struct tagDecoderContext
{
    AudioContextHeader hdr;
    PluginConfig pluginCfg;
    std::wstring mediaName;
    std::unique_ptr<LibVgmPlayCtrl> playCtrl;
} DecoderContext;

char errorMsg[256] = {};
static PluginConfig g_pluginCfg = PluginConfig{};
static bool g_pluginCfgLoaded = false;

static bool EnsurePluginConfigLoaded()
{
    if (g_pluginCfgLoaded) {
        return true;
    }

    g_pluginCfg = PluginConfig{};
    const std::filesystem::path cfgPath = GetVgmPluginConfigPath();
    if (!LoadVgmPluginConfigFile(cfgPath, g_pluginCfg)) {
        SaveVgmPluginConfigFile(cfgPath, g_pluginCfg);
    }

    g_pluginCfgLoaded = true;
    return true;
}

int WINAPI Plug_OnRegister(const ApplicationConfig* app, PluginRegister* regInfo)
{
    if (app->major_ver > 5)
    {
        strcpy_s(errorMsg, "This plugin module only tested for version below 5.0");
        return -1;
    }
    if (regInfo->size != sizeof(PluginRegister))
    {
        strcpy_s(errorMsg, "This plugin module version not match with host app");
        return -2;
    }

    FileTypeIdBegin = app->FileTypeIdBegin;
    SetLibvgmCustomFormatBase(FileTypeIdBegin);

    const FileExtRegItem fmtMap[] = {
        {StreamFormatVgmVgz, "Video Game Music", ".vgm;.vgz"},
        {StreamFormatDro, "DOSBox Raw OPL", ".dro"},
        {StreamFormatDro2, "DOSBox Raw OPL v2", ".dro2"},
        {LibvgmFormatS98(), "S98 Music Log", ".s98"},
        {LibvgmFormatGym(), "Genesis YM2612 Log", ".gym;.gymx"},
    };

    uint32_t formatCount = static_cast<uint32_t>(sizeof(fmtMap) / sizeof(fmtMap[0]));
    if (formatCount > PLUG_FORMAT_MAX_LIMIT)
        formatCount = PLUG_FORMAT_MAX_LIMIT;

    for (uint32_t i = 0; i < formatCount; ++i)
    {
        regInfo->fmt_reg[i].id = fmtMap[i].st;
        strcpy_s(regInfo->fmt_reg[i].desc, fmtMap[i].desc);
        strcpy_s(regInfo->fmt_reg[i].ext_list, fmtMap[i].extList);
    }
    regInfo->format_count = formatCount;

    return 0;
}

void WINAPI Plug_GetErrMessage(char* msgBuf, uint32_t bufSize)
{
    if ((msgBuf == nullptr) || (bufSize == 0))
        return;

    strcpy_s(msgBuf, bufSize, errorMsg);
}

uint32_t WINAPI Plug_ParseFileTypeID(const char* filename)
{
    return ParseStreamFormatByLibvgm(filename);
}

int WINAPI Plug_GetPluginInformation(PluginInfo* info)
{
    if (info->size != sizeof(PluginInfo))
    {
        strcpy_s(errorMsg, "This plugin module version not match with host app");
        return -2;
    }

    info->plug_type = PluginType::Decoder;
    info->ver_major = 1;
    info->ver_minor = 0;
    strcpy_s(info->name, 64, reinterpret_cast<const char*>(plugname));
    strcpy_s(info->publisher, 128, reinterpret_cast<const char*>(plugpublisher));

    return 0;
}

void* WINAPI Plug_OnInitialize(const PluginInitialize* init)
{
    if ((init == nullptr) || (init->pStream == nullptr))
    {
        strcpy_s(errorMsg, "Invalid initialize parameter.");
        return nullptr;
    }

    DecoderContext* pCtx = new DecoderContext;
    if (pCtx == nullptr)
        return nullptr;

    pCtx->hdr.size = sizeof(AudioContextHeader);
    pCtx->hdr.filesize = init->pStream->GetLength();
    pCtx->hdr.streamCount = 1;
    pCtx->hdr.streamIndex = static_cast<uint32_t>(-1);
    EnsurePluginConfigLoaded();
    pCtx->pluginCfg = g_pluginCfg;
    pCtx->mediaName = init->pStream->GetName();
    pCtx->playCtrl = std::make_unique<LibVgmPlayCtrl>();

    if (!pCtx->playCtrl->Init(init->pStream, init->streamFmt, pCtx->pluginCfg))
    {
        strcpy_s(errorMsg, "Failed to initialize libvgm stream controller.");
        delete pCtx;
        return nullptr;
    }

    pCtx->hdr.m_totalFrames = static_cast<long long>(pCtx->playCtrl->GetTotalFrame());
    pCtx->hdr.durations = pCtx->playCtrl->GetDurationSeconds();

    return pCtx;
}

int WINAPI Plug_StartStream(void* ctxhdr, const PluginStart* param)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    const uint32_t reqStreamIdx = (param == nullptr) ? static_cast<uint32_t>(-1) : param->mediaStreamIdx;
    if (!pCtx)
    {
        sprintf_s(errorMsg, "Failed to start media stream [%u]: plus not initialized!", reqStreamIdx);
        return -1;
    }
    if ((param == nullptr) || (pCtx->playCtrl == nullptr))
    {
        strcpy_s(errorMsg, "Failed to start media stream: invalid start parameter.");
        return -1;
    }
    if (!pCtx->playCtrl->OpenStream(param->mediaStreamIdx))
    {
        sprintf_s(errorMsg, "Failed to start media stream [%u]: unsupported stream index.", param->mediaStreamIdx);
        return -1;
    }

    pCtx->hdr.streamIndex = 0;
    pCtx->hdr.streamCount = pCtx->playCtrl->GetStreamCount();
    pCtx->hdr.m_totalFrames = static_cast<long long>(pCtx->playCtrl->GetTotalFrame());
    pCtx->hdr.durations = pCtx->playCtrl->GetDurationSeconds();

    return 0;
}

int WINAPI Plug_StopStream(void* ctxhdr, uint32_t mediaStreamIdx)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        sprintf_s(errorMsg, "Failed to stop media stream [%u]: plus not initialized!", mediaStreamIdx);
        return -1;
    }

    pCtx->playCtrl->StopStream();
    pCtx->hdr.streamIndex = static_cast<uint32_t>(-1);
    return 0;
}

int WINAPI Plug_IsSupportOutput(void* ctxhdr, uint32_t mediaStreamIdx, const AudioFormat* audioFmt)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return -1;
    }

    if ((mediaStreamIdx != 0) && (mediaStreamIdx != static_cast<uint32_t>(-1)))
    {
        sprintf_s(errorMsg, "Unsupported media stream index: %u", mediaStreamIdx);
        return 0;
    }

    return pCtx->playCtrl->IsSupportOutput(audioFmt) ? 1 : 0;
}

int WINAPI Plug_IsCanSeeking(void* ctxhdr, uint32_t mediaStreamIdx)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return 0;
    }

    if ((mediaStreamIdx != 0) && (mediaStreamIdx != static_cast<uint32_t>(-1)))
    {
        sprintf_s(errorMsg, "Unsupported media stream index: %u", mediaStreamIdx);
        return 0;
    }

    return pCtx->playCtrl->IsCanSeeking() ? 1 : 0;
}

void WINAPI Plug_OnUninitialize(void* ctxhdr)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx)
        return;

    if (pCtx->playCtrl)
        pCtx->playCtrl->Release();

    delete pCtx;
}

uint32_t WINAPI Plug_DecodeFrames(void* ctx, void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctx);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return 0;
    }

    uint32_t readFrames = pCtx->playCtrl->DecodeFrames(pBuf, frames, audioFmt);
    pCtx->hdr.streamIndex = pCtx->playCtrl->GetActiveStreamIndex();
    return readFrames;
}

void WINAPI Plug_SeekToFrame(void* ctx, std::size_t frames)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctx);
    if (pCtx == nullptr)
        return;

    if (pCtx->playCtrl)
    {
        pCtx->playCtrl->Seek(frames);
        pCtx->hdr.streamIndex = pCtx->playCtrl->GetActiveStreamIndex();
    }
}

int WINAPI Plug_QueryMetaInfo(void* ctxhdr, uint32_t streamIdx, AudioMetaTags* metaTags)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return -1;
    }
    if (metaTags->size < sizeof(AudioMetaTags))
    {
        strcpy_s(errorMsg, "Failed to query meta info: metaTags size not match!");
        return -1;
    }

    if (streamIdx == static_cast<uint32_t>(-1))
        streamIdx = 0;
    if (streamIdx != 0)
    {
        sprintf_s(errorMsg, "Unsupported media stream index: %u", streamIdx);
        return -1;
    }

    metaTags->size = sizeof(AudioMetaTags);
    metaTags->streamIdx = streamIdx;
    pCtx->playCtrl->FillMetaTags(metaTags->tags);

    return 0;
}

int WINAPI Plug_GetAudioStatusInfo(void* ctxhdr, PluginAudioInfo* info)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return -1;
    }
    if (info->size < sizeof(PluginAudioInfo))
    {
        strcpy_s(errorMsg, "Failed to get audio status: info size not match!");
        return -1;
    }

    info->size = sizeof(PluginAudioInfo);
    if ((info->flags & AudioInfoFlagFormat) != 0)
        info->audioFmt = pCtx->playCtrl->GetSourceAudioFormat();
    if ((info->flags & AudioInfoFlagActiveStream) != 0)
        info->mediaStreamIdx = pCtx->playCtrl->GetActiveStreamIndex();
    if ((info->flags & AudioInfoFlagStreamCount) != 0)
        info->mediaStreamCount = pCtx->playCtrl->GetStreamCount();
    if ((info->flags & AudioInfoFlagCurrentFrames) != 0)
        info->currentFrames = pCtx->playCtrl->GetCurrentFrame();
    if ((info->flags & AudioInfoFlagTotalFrames) != 0)
        info->totalFrames = pCtx->playCtrl->GetTotalFrame();

    return 0;
}

void WINAPI Plug_ResetDecoder(void* ctxhdr)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return;
    }

    pCtx->playCtrl->Seek(0);
}

void WINAPI Plug_ConfigPlugin(HWND hWnd)
{
    (void)hWnd;

    EnsurePluginConfigLoaded();

    using namespace ftxui;

    std::string sampleRateText = std::to_string(g_pluginCfg.SampleRate);
    std::string loopsText = std::to_string(g_pluginCfg.Loops);
    std::string fadeLenText = std::to_string(g_pluginCfg.FadeLen);
    std::string errorText;

    int bitsSelect = 0;
    switch (g_pluginCfg.BitsPerSample)
    {
    case 16:
        bitsSelect = 0;
        break;
    case 24:
        bitsSelect = 1;
        break;
    case 32:
        bitsSelect = 2;
        break;
    default:
        bitsSelect = 2;
        break;
    }

    std::vector<std::string> bitsItems = { "16", "24", "32" };

    ScreenInteractive screen = ScreenInteractive::TerminalOutput();
    bool shouldSave = false;

    auto sampleRateInput = Input(&sampleRateText, "sample rate");
    auto loopsInput = Input(&loopsText, "loops");
    auto fadeInput = Input(&fadeLenText, "fade seconds");
    auto bitsRadio = Radiobox(&bitsItems, &bitsSelect);

    auto onOk = [&]() {
        PluginConfig nextConfig = g_pluginCfg;

        try {
            const unsigned long sampleRate = std::stoul(sampleRateText);
            const unsigned long loops = std::stoul(loopsText);
            const float fadeLen = std::stof(fadeLenText);

            if ((sampleRate == 0) || (loops == 0) || (fadeLen < 0.0f)) {
                errorText = "SampleRate/Loops must > 0, FadeLen >= 0.";
                return;
            }

            nextConfig.SampleRate = static_cast<uint32_t>(sampleRate);
            nextConfig.Loops = static_cast<uint32_t>(loops);
            nextConfig.FadeLen = fadeLen;
            nextConfig.BitsPerSample = static_cast<uint32_t>(std::stoi(bitsItems[bitsSelect]));
        }
        catch (...) {
            errorText = "Invalid number format.";
            return;
        }

        if (!SaveVgmPluginConfigFile(GetVgmPluginConfigPath(), nextConfig)) {
            errorText = "Save config failed.";
            return;
        }

        g_pluginCfg = nextConfig;
        shouldSave = true;
        screen.ExitLoopClosure()();
    };

    auto onCancel = [&]() {
        screen.ExitLoopClosure()();
    };

    auto okButton = Button("OK", onOk);
    auto cancelButton = Button("Cancel", onCancel);

    auto layout = Container::Vertical({
        sampleRateInput,
        bitsRadio,
        loopsInput,
        fadeInput,
        Container::Horizontal({ okButton, cancelButton }),
    });

    auto renderer = Renderer(layout, [&] {
        return vbox({
            text("libvgm decoder config") | bold,
            separator(),
            hbox({ text("SampleRate: "), sampleRateInput->Render() }),
            hbox({ text("BitsPerSample: "), bitsRadio->Render() }),
            hbox({ text("Loops: "), loopsInput->Render() }),
            hbox({ text("FadeLen: "), fadeInput->Render(), text(" seconds") }),
            separator(),
            hbox({ okButton->Render(), text("  "), cancelButton->Render() }),
            separator(),
            text(errorText) | color(Color::Red),
        }) | border;
    });

    screen.Loop(renderer);

    if (!shouldSave && !errorText.empty()) {
        strcpy_s(errorMsg, "Failed to save plugin config.");
    }
}
