/*
原始函数声明是手工写的，extern “C” 和 WINAPI 声明部分是 AI 修改的。
大模型：GPT 5.3 Codex


*/
#include <string>
#include <windows.h>
#include "PlugMain.h"
#include "MediaTag.h"

const char8_t* plugname = u8"Libsndfile decoder";
const char8_t* plugpublisher = u8"iPlayer Group";

typedef struct tagDecoderContext
{
    AudioContextHeader hdr;
    std::wstring mediaName;
    void *Decoder;
}DecoderContext;

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

    return 0;
}

void WINAPI Plus_GetErrMessage(char* msgBuf, uint32_t bufSize)
{
}

uint32_t WINAPI Plus_ParseFileTypeID(const char* filename) //filename in utf8 codec
{
    return StreamFormatUnknown;
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
    info->ver_minor = 2;
    strcpy_s(info->name, 64, (const char*)plugname);
    strcpy_s(info->publisher, 128, (const char*)plugpublisher);

    return 0;
}

void* WINAPI Plus_OnInitialize(const PluginInitialize* init)
{
    //std::wstring srcname = pStream->GetName();
    DecoderContext* pCtx = new DecoderContext;
    if (pCtx == nullptr)
        return nullptr;

    std::memset(pCtx, 0, sizeof(DecoderContext));
    pCtx->hdr.size = sizeof(AudioContextHeader);
    pCtx->hdr.filesize = init->pStream->GetLength();
 
    return pCtx;
}

int WINAPI Plus_StartStream(void* ctxhdr, const PluginStart* param)
{
    DecoderContext* pCtx = (DecoderContext*)ctxhdr;
    if (!pCtx)
    {
        sprintf_s(errorMsg, "Failed to start media stream [%u]: plus not initialized!", param->mediaStreamIdx);
        return -1;
    }

    return 0;
}

int WINAPI Plus_StopStream(void* ctxhdr, uint32_t mediaStreamIdx)
{
    DecoderContext* pCtx = (DecoderContext*)ctxhdr;
    if (!pCtx || !pCtx->Decoder)
    {
        sprintf_s(errorMsg, "Failed to stop media stream [%u]: plus not initialized!", mediaStreamIdx);
        return -1;
    }

    pCtx->hdr.streamIndex = -1;
    return 0;
}

int WINAPI Plus_IsSupportOutput(void* ctxhdr, uint32_t mediaStreamIdx, const AudioFormat* audioFmt)
{
    DecoderContext* pCtx = (DecoderContext*)ctxhdr;
    if (!pCtx || !pCtx->Decoder)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return -1;
    }

    return 0;
}

int WINAPI Plus_IsCanSeeking(void* ctxhdr, uint32_t mediaStreamIdx)
{
    DecoderContext* pCtx = (DecoderContext*)ctxhdr;
    if (!pCtx || !pCtx->Decoder)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return 0;
    }

    return 1;
}

void WINAPI Plus_OnUninitialize(void* ctxhdr)
{
    DecoderContext* pCtx = (DecoderContext *)ctxhdr;
    if (!pCtx)
        return;

    delete pCtx;
}

uint32_t WINAPI Plus_DecodeFrames(void* ctx, void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    DecoderContext* pCtx = (DecoderContext*)ctx;
    if (!pCtx || !pCtx->Decoder)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return 0;
    }

    return 0;
}

void WINAPI Plus_SeekToFrame(void* ctx, std::size_t frames)
{
    DecoderContext* pCtx = (DecoderContext*)ctx;
    if (pCtx == nullptr)
        return;

}

int WINAPI Plus_QueryMetaInfo(void* ctxhdr, uint32_t streamIdx, AudioMetaTags* metaTags)
{
    DecoderContext* pCtx = (DecoderContext*)ctxhdr;
    if (!pCtx || !pCtx->Decoder)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return -1;
    }
    if (metaTags->size < sizeof(AudioMetaTags))
    {
        strcpy_s(errorMsg, "Failed to query meta info: metaTags size not match!");
        return -1;
    }

    if (streamIdx == -1)
        streamIdx = 0;

    metaTags->size = sizeof(AudioMetaTags);
    metaTags->streamIdx = streamIdx;
    metaTags->tags.Clear();

    return 0;
}

int WINAPI Plus_GetAudioStatusInfo(void* ctxhdr, PluginAudioInfo* info)
{
    DecoderContext* pCtx = (DecoderContext*)ctxhdr;
    if (!pCtx || !pCtx->Decoder)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return -1;
    }
    if (info->size < sizeof(PluginAudioInfo))
    {
        strcpy_s(errorMsg, "Failed to get audio status: info size not match!");
        return -1;
    }

    return 0;
}

void WINAPI Plus_ResetDecoder(void* ctxhdr)
{
    DecoderContext* pCtx = (DecoderContext*)ctxhdr;
    if (!pCtx || !pCtx->Decoder)
    {
        strcpy_s(errorMsg, "Current audio stream is closed!!");
        return;
    }
}

void WINAPI Plus_ConfigPlugin(HWND hWnd)
{
}
