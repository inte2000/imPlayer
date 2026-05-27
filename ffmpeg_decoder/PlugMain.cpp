/*
20260523 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_53.txt

修改记录：
大模型：ChatGPT 5.3 Codex
todo_task_54.txt
todo_task_57.txt
todo_task_58.txt
todo_task_59.txt
*/
#include <cstring>
#include <memory>
#include <string>
#include <filesystem>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <windows.h>

#include "PlugMain.h"
#include "FfmpegFunc.h"
#include "FfmpegPlayCtrl.h"
#include "PluginConfig.h"
#include "PluginConfigFile.h"

const char8_t* plugname = u8"FFmpeg decoder";
const char8_t* plugpublisher = u8"imPlayer Group";

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
    std::unique_ptr<FfmpegPlayCtrl> playCtrl;
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
    const std::filesystem::path cfgPath = GetFfmpegPluginConfigPath();
    if (!LoadFfmpegPluginConfigFile(cfgPath, g_pluginCfg)) {
        SaveFfmpegPluginConfigFile(cfgPath, g_pluginCfg);
    }

    g_pluginCfgLoaded = true;
    return true;
}

int WINAPI Plus_OnRegister(const ApplicationConfig* app, PluginRegister* regInfo)
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

    const FileExtRegItem fmtMap[] = {
        {StreamFormatMp3, "MPEG Audio Layer III", ".mp3"},
        {StreamFormatMp2, "MPEG Audio Layer II", ".mp2"},
        {StreamFormatMp1, "MPEG Audio Layer I", ".mp1"},
        {StreamFormatFlac, "Free Lossless Audio Codec", ".flac"},
        {StreamFormatOgg, "Ogg Vorbis/Opus", ".ogg;.oga;.opus"},
        {StreamFormatWav, "Waveform Audio", ".wav"},
        {StreamFormatDts, "Digital Theater Systems", ".dts;.dtshd"},
        {StreamFormatDsf, "DSD Stream File", ".dsf"},
        {StreamFormatDff, "DSDIFF", ".dff;.dsdiff"},
        {StreamFormatApe, "Monkey's Audio", ".ape"},
        {StreamFormatAmr, "Adaptive Multi-Rate", ".amr;.awb"},
        {StreamFormatAiff, "Audio Interchange File Format", ".aiff;.aif"},
        {StreamFormatCaf, "Core Audio Format", ".caf"},
        {StreamFormatAac, "Advanced Audio Coding", ".aac"},
        {StreamFormatM4a, "MPEG-4 Audio", ".m4a"},
        {StreamFormatMp4, "MPEG-4 Media", ".mp4;.mov;.m4v;.3gp"},
        {StreamFormatAsf, "ASF / WMA", ".asf;.wma"},
        {StreamFormatWmv, "Windows Media Video", ".wmv"},
    };

    uint32_t formatCount = static_cast<uint32_t>(sizeof(fmtMap) / sizeof(fmtMap[0]));
    if (formatCount > PLUG_FORMAT_MAX_LIMIT) {
        formatCount = PLUG_FORMAT_MAX_LIMIT;
    }

    for (uint32_t i = 0; i < formatCount; ++i)
    {
        regInfo->fmt_reg[i].id = fmtMap[i].st;
        strcpy_s(regInfo->fmt_reg[i].desc, fmtMap[i].desc);
        strcpy_s(regInfo->fmt_reg[i].ext_list, fmtMap[i].extList);
    }
    regInfo->format_count = formatCount;

    return 0;
}

void WINAPI Plus_GetErrMessage(char* msgBuf, uint32_t bufSize)
{
    if ((msgBuf == nullptr) || (bufSize == 0))
        return;

    strcpy_s(msgBuf, bufSize, errorMsg);
}

uint32_t WINAPI Plus_ParseFileTypeID(const char* filename)
{
    return ParseStreamFormatByFfmpeg(filename);
}

int WINAPI Plus_GetPluginInformation(PluginInfo* info)
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

void* WINAPI Plus_OnInitialize(const PluginInitialize* init)
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
    pCtx->hdr.streamCount = 0;
    pCtx->hdr.streamIndex = static_cast<uint32_t>(-1);
    EnsurePluginConfigLoaded();
    pCtx->pluginCfg = g_pluginCfg;
    pCtx->mediaName = init->pStream->GetName();
    pCtx->playCtrl = std::make_unique<FfmpegPlayCtrl>();

    if (!pCtx->playCtrl->Init(init->pStream, init->streamFmt, pCtx->pluginCfg))
    {
        strcpy_s(errorMsg, "Failed to initialize ffmpeg stream controller.");
        delete pCtx;
        return nullptr;
    }

    pCtx->hdr.streamCount = pCtx->playCtrl->GetStreamCount();
    pCtx->hdr.m_totalFrames = static_cast<long long>(pCtx->playCtrl->GetTotalFrame());
    pCtx->hdr.durations = pCtx->playCtrl->GetDurationSeconds();
    return pCtx;
}

int WINAPI Plus_StartStream(void* ctxhdr, const PluginStart* param)
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

    pCtx->hdr.streamIndex = pCtx->playCtrl->GetActiveStreamIndex();
    pCtx->hdr.streamCount = pCtx->playCtrl->GetStreamCount();
    pCtx->hdr.m_totalFrames = static_cast<long long>(pCtx->playCtrl->GetTotalFrame());
    pCtx->hdr.durations = pCtx->playCtrl->GetDurationSeconds();
    return 0;
}

int WINAPI Plus_StopStream(void* ctxhdr, uint32_t mediaStreamIdx)
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

int WINAPI Plus_IsSupportOutput(void* ctxhdr, uint32_t mediaStreamIdx, const AudioFormat* audioFmt)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return -1;
    }

    const uint32_t maxStreams = pCtx->playCtrl->GetStreamCount();
    if ((mediaStreamIdx != static_cast<uint32_t>(-1)) && (mediaStreamIdx >= maxStreams))
    {
        sprintf_s(errorMsg, "Unsupported media stream index: %u", mediaStreamIdx);
        return 0;
    }

    return pCtx->playCtrl->IsSupportOutput(audioFmt) ? 1 : 0;
}

int WINAPI Plus_IsCanSeeking(void* ctxhdr, uint32_t mediaStreamIdx)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return 0;
    }

    const uint32_t maxStreams = pCtx->playCtrl->GetStreamCount();
    if ((mediaStreamIdx != static_cast<uint32_t>(-1)) && (mediaStreamIdx >= maxStreams))
    {
        sprintf_s(errorMsg, "Unsupported media stream index: %u", mediaStreamIdx);
        return 0;
    }

    return pCtx->playCtrl->IsCanSeeking() ? 1 : 0;
}

void WINAPI Plus_OnUninitialize(void* ctxhdr)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx)
        return;

    if (pCtx->playCtrl)
        pCtx->playCtrl->Release();

    delete pCtx;
}

uint32_t WINAPI Plus_DecodeFrames(void* ctx, void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
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

void WINAPI Plus_SeekToFrame(void* ctx, std::size_t frames)
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

int WINAPI Plus_QueryMetaInfo(void* ctxhdr, uint32_t streamIdx, AudioMetaTags* metaTags)
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

    if (streamIdx == static_cast<uint32_t>(-1)) {
        streamIdx = pCtx->playCtrl->GetActiveStreamIndex();
    }
    if (streamIdx >= pCtx->playCtrl->GetStreamCount())
    {
        sprintf_s(errorMsg, "Unsupported media stream index: %u", streamIdx);
        return -1;
    }

    metaTags->size = sizeof(AudioMetaTags);
    metaTags->streamIdx = streamIdx;
    pCtx->playCtrl->FillMetaTags(metaTags->tags);
    return 0;
}

int WINAPI Plus_GetAudioStatusInfo(void* ctxhdr, PluginAudioInfo* info)
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

void WINAPI Plus_ResetDecoder(void* ctxhdr)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx || !pCtx->playCtrl)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return;
    }

    pCtx->playCtrl->Seek(0);
}

void WINAPI Plus_ConfigPlugin(HWND hWnd)
{
    (void)hWnd;
    EnsurePluginConfigLoaded();

    using namespace ftxui;

    std::string sampleRateText = std::to_string(g_pluginCfg.DefaultSampleRate);
    std::string bitsText = std::to_string(g_pluginCfg.DefaultBitsPerSample);
    std::string channelsText = std::to_string(g_pluginCfg.DefaultChannels);
    std::string errorText;

    int pcmFmtSelect = 0;
    std::vector<std::string> pcmFmtItems = { "Float32", "PCM_S16", "PCM_S32", "PCM_S24_32" };
    AudioDataFormat fmt = static_cast<AudioDataFormat>(g_pluginCfg.DefaultPcmFormat);
    if (fmt == AudioDataFormat::PCM_S16) pcmFmtSelect = 1;
    else if (fmt == AudioDataFormat::PCM_S32) pcmFmtSelect = 2;
    else if (fmt == AudioDataFormat::PCM_S24_32) pcmFmtSelect = 3;

    ScreenInteractive screen = ScreenInteractive::TerminalOutput();

    auto sampleRateInput = Input(&sampleRateText, "default sample rate");
    auto bitsInput = Input(&bitsText, "default bits per sample");
    auto channelsInput = Input(&channelsText, "default channels");
    auto fmtRadio = Radiobox(&pcmFmtItems, &pcmFmtSelect);

    auto onOk = [&]() {
        PluginConfig next = g_pluginCfg;
        try {
            const uint32_t sr = static_cast<uint32_t>(std::stoul(sampleRateText));
            const uint32_t bps = static_cast<uint32_t>(std::stoul(bitsText));
            const uint32_t ch = static_cast<uint32_t>(std::stoul(channelsText));
            if (sr == 0 || bps == 0 || ch == 0) {
                errorText = "SampleRate/Bits/Channels must > 0";
                return;
            }

            next.DefaultSampleRate = sr;
            next.DefaultBitsPerSample = bps;
            next.DefaultChannels = ch;
            switch (pcmFmtSelect)
            {
            case 1: next.DefaultPcmFormat = static_cast<uint32_t>(AudioDataFormat::PCM_S16); break;
            case 2: next.DefaultPcmFormat = static_cast<uint32_t>(AudioDataFormat::PCM_S32); break;
            case 3: next.DefaultPcmFormat = static_cast<uint32_t>(AudioDataFormat::PCM_S24_32); break;
            default: next.DefaultPcmFormat = static_cast<uint32_t>(AudioDataFormat::Float32); break;
            }
        }
        catch (...) {
            errorText = "Invalid number format.";
            return;
        }

        if (!SaveFfmpegPluginConfigFile(GetFfmpegPluginConfigPath(), next)) {
            errorText = "Save config failed.";
            return;
        }

        g_pluginCfg = next;
        screen.ExitLoopClosure()();
    };

    auto onCancel = [&]() {
        screen.ExitLoopClosure()();
    };

    auto okButton = Button("OK", onOk);
    auto cancelButton = Button("Cancel", onCancel);

    auto layout = Container::Vertical({
        sampleRateInput,
        bitsInput,
        channelsInput,
        fmtRadio,
        Container::Horizontal({ okButton, cancelButton }),
    });

    auto renderer = Renderer(layout, [&] {
        return vbox({
            text("ffmpeg decoder config") | bold,
            separator(),
            hbox({ text("DefaultSampleRate: "), sampleRateInput->Render() }),
            hbox({ text("DefaultBitsPerSample: "), bitsInput->Render() }),
            hbox({ text("DefaultChannels: "), channelsInput->Render() }),
            hbox({ text("DefaultPcmFormat: "), fmtRadio->Render() }),
            separator(),
            hbox({ okButton->Render(), text("  "), cancelButton->Render() }),
            separator(),
            text(errorText) | color(Color::Red),
        }) | border;
    });

    screen.Loop(renderer);
}
