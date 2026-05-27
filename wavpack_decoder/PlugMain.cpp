/*
20260527 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_84.txt
*/
#include <cstring>
#include <memory>

#include <windows.h>

#include "PlugMain.h"
#include "WavpackFunc.h"
#include "WavpackPlayCtrl.h"

const char8_t* plugname = u8"WavPack decoder";
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
    std::wstring mediaName;
    std::unique_ptr<WavpackPlayCtrl> playCtrl;
} DecoderContext;

char errorMsg[256] = {};

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

    SetWavpackCustomFormatBase(app->FileTypeIdBegin);

    const FileExtRegItem fmtMap[] = {
        {WavpackFormatWv(), "WavPack Audio", ".wv"},
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
    if ((msgBuf == nullptr) || (bufSize == 0)) {
        return;
    }

    strcpy_s(msgBuf, bufSize, errorMsg);
}

uint32_t WINAPI Plus_ParseFileTypeID(const char* filename)
{
    return ParseStreamFormatByWavpack(filename);
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
    if (pCtx == nullptr) {
        return nullptr;
    }

    pCtx->hdr.size = sizeof(AudioContextHeader);
    pCtx->hdr.filesize = init->pStream->GetLength();
    pCtx->hdr.streamCount = 1;
    pCtx->hdr.streamIndex = static_cast<uint32_t>(-1);
    pCtx->mediaName = init->pStream->GetName();
    pCtx->playCtrl = std::make_unique<WavpackPlayCtrl>();

    if (!pCtx->playCtrl->Init(init->pStream, init->streamFmt))
    {
        strcpy_s(errorMsg, "Failed to initialize wavpack stream controller.");
        delete pCtx;
        return nullptr;
    }

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

    pCtx->hdr.streamIndex = 0;
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

    if ((mediaStreamIdx != 0) && (mediaStreamIdx != static_cast<uint32_t>(-1)))
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

    if ((mediaStreamIdx != 0) && (mediaStreamIdx != static_cast<uint32_t>(-1)))
    {
        sprintf_s(errorMsg, "Unsupported media stream index: %u", mediaStreamIdx);
        return 0;
    }

    return pCtx->playCtrl->IsCanSeeking() ? 1 : 0;
}

void WINAPI Plus_OnUninitialize(void* ctxhdr)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctxhdr);
    if (!pCtx) {
        return;
    }

    if (pCtx->playCtrl) {
        pCtx->playCtrl->Release();
    }

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

    const uint32_t readFrames = pCtx->playCtrl->DecodeFrames(pBuf, frames, audioFmt);
    pCtx->hdr.streamIndex = pCtx->playCtrl->GetActiveStreamIndex();
    return readFrames;
}

void WINAPI Plus_SeekToFrame(void* ctx, std::size_t frames)
{
    DecoderContext* pCtx = static_cast<DecoderContext*>(ctx);
    if (pCtx == nullptr) {
        return;
    }

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
        streamIdx = 0;
    }
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
    if ((info->flags & AudioInfoFlagFormat) != 0) {
        info->audioFmt = pCtx->playCtrl->GetSourceAudioFormat();
    }
    if ((info->flags & AudioInfoFlagActiveStream) != 0) {
        info->mediaStreamIdx = pCtx->playCtrl->GetActiveStreamIndex();
    }
    if ((info->flags & AudioInfoFlagStreamCount) != 0) {
        info->mediaStreamCount = pCtx->playCtrl->GetStreamCount();
    }
    if ((info->flags & AudioInfoFlagCurrentFrames) != 0) {
        info->currentFrames = pCtx->playCtrl->GetCurrentFrame();
    }
    if ((info->flags & AudioInfoFlagTotalFrames) != 0) {
        info->totalFrames = pCtx->playCtrl->GetTotalFrame();
    }

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
}
