#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>

#include "PluginMain.h"
#include "EncodeControl.h"

typedef struct tagEncoderContext
{
    AudioEncodeContextHeader hdr;
    CMediaTag tags;
    std::unique_ptr<CEncodeControl> encodeCtrl;
}EncoderContext;

namespace
{
thread_local char errorMsg[256] = {};

int Error(const char* message)
{
    strncpy_s(errorMsg, message, _TRUNCATE);
    return -1;
}

template <typename TAction>
int Invoke(TAction&& action)
{
    errorMsg[0] = '\0';
    try {
        action();
        return 0;
    } catch (const std::exception& e) {
        return Error(e.what());
    } catch (...) {
        return Error("Unexpected encoder plugin error.");
    }
}

template <typename TStruct>
void CheckParameter(const TStruct* value)
{
    if (!value || value->size != sizeof(TStruct)) {
        throw std::runtime_error("Invalid plugin parameter or structure size.");
    }
}

EncoderContext& Context(void* ctxhdr)
{
    if (!ctxhdr) throw std::runtime_error("Encoder context is null.");
    auto& ctx = *static_cast<EncoderContext*>(ctxhdr);
    if (ctx.hdr.size != sizeof(AudioEncodeContextHeader)) throw std::runtime_error("Invalid encoder context header.");
    return ctx;
}

CEncodeControl& Control(void* ctxhdr)
{
    auto& ctx = Context(ctxhdr);
    if (!ctx.encodeCtrl) throw std::runtime_error("Encoder is not open.");
    return *ctx.encodeCtrl;
}

void CheckStream(uint32_t streamIdx)
{
    if (streamIdx != 0) throw std::runtime_error("libsndfile supports only media stream zero.");
}

void CheckResult(bool result, const CEncodeControl& control)
{
    if (!result) throw std::runtime_error(control.GetErrorMessage());
}
}

int WINAPI Plug_OnRegister(const ApplicationConfig* app, PluginRegister* regInfo)
{
    return Invoke([&] {
        if (!app) throw std::runtime_error("Application configuration is null.");
        CheckParameter(regInfo);
        std::vector<EncoderFormatDefine> formats;
        CEncodeControl::QueryFormatsDefine(formats);
        regInfo->format_count = static_cast<uint32_t>((std::min)(formats.size(), static_cast<std::size_t>(PLUG_FORMAT_MAX_LIMIT)));
        for (uint32_t i = 0; i < regInfo->format_count; ++i) {
            regInfo->fmt_reg[i].id = formats[i].streamFmt;
            strncpy_s(regInfo->fmt_reg[i].desc, formats[i].desc.c_str(), _TRUNCATE);
            strncpy_s(regInfo->fmt_reg[i].ext_list, formats[i].extname.c_str(), _TRUNCATE);
        }
    });
}

void WINAPI Plug_GetErrMessage(char* msgBuf, uint32_t bufSize)
{
    if (msgBuf && bufSize) strncpy_s(msgBuf, bufSize, errorMsg, _TRUNCATE);
}

int WINAPI Plug_GetPluginInformation(PluginInfo* info)
{
    return Invoke([&] {
        CheckParameter(info);
        info->plug_type = PluginType::Encoder;
        info->ver_major = 0;
        info->ver_minor = 2;
        strcpy_s(info->name, "Libsndfile encoder");
        strcpy_s(info->publisher, "imPlayer Group");
    });
}

int WINAPI Plug_GetEncodingFormatsDefine(PluginEncodingFormatsDefine* fmt)
{
    return Invoke([&] {
        CheckParameter(fmt);
        CEncodeControl::QueryFormatsDefine(fmt->fmtDef);
    });
}

int WINAPI Plug_GetEncodingParamtersDefine(uint32_t streamFmt, PluginEncodingParamtersDefine* param)
{
    return Invoke([&] {
        CheckParameter(param);
        CEncodeControl::QueryParamtersDefine(streamFmt, param->paramDef);
    });
}

void* WINAPI Plug_OnInitialize(const EncoderPluginInitialize* init)
{
    std::unique_ptr<EncoderContext> ctx;
    const int result = Invoke([&] {
        CheckParameter(init);
        ctx = std::make_unique<EncoderContext>();
        ctx->hdr.size = sizeof(AudioEncodeContextHeader);
    });
    return result == 0 ? ctx.release() : nullptr;
}

void WINAPI Plug_OnRelease(void* ctxhdr)
{
    if (!ctxhdr) return;
    Invoke([&] {
        std::unique_ptr<EncoderContext> ctx(&Context(ctxhdr));
        if (ctx->encodeCtrl) {
            ctx->encodeCtrl->Close();
            if (!ctx->encodeCtrl->GetErrorMessage().empty()) Error(ctx->encodeCtrl->GetErrorMessage().c_str());
        }
    });
}

int WINAPI Plug_OpenEncoder(void* ctxhdr, const EncoderPluginOpen* open)
{
    return Invoke([&] {
        auto& ctx = Context(ctxhdr);
        CheckParameter(open);
        CheckStream(open->mediaStreamIdx);
        if (ctx.encodeCtrl) throw std::runtime_error("Encoder is already open; close it before reopening.");
        auto control = std::make_unique<CEncodeControl>(open->streamFmt);
        control->SetMetaInfo(0, ctx.tags);
        CheckResult(control->Open(open->pStream, open->params), *control);
        ctx.hdr.transFmt = control->GetTransFormat();
        ctx.encodeCtrl = std::move(control);
    });
}

void WINAPI Plug_CloseEncoder(void* ctxhdr, const EncoderPluginClose* close)
{
    Invoke([&] {
        auto& ctx = Context(ctxhdr);
        CheckParameter(close);
        CheckStream(close->mediaStreamIdx);
        if (ctx.encodeCtrl) {
            auto control = std::move(ctx.encodeCtrl);
            control->Close();
            ctx.hdr.transFmt = {};
            if (!control->GetErrorMessage().empty()) Error(control->GetErrorMessage().c_str());
        }
    });
}

int WINAPI Plug_SetMetaInfo(void* ctxhdr, const AudioMetaTags* metaTags)
{
    return Invoke([&] {
        auto& ctx = Context(ctxhdr);
        CheckParameter(metaTags);
        CheckStream(metaTags->streamIdx);
        if (ctx.encodeCtrl) ctx.encodeCtrl->SetMetaInfo(metaTags->streamIdx, metaTags->tags);
        ctx.tags = metaTags->tags;
    });
}

int WINAPI Plug_StartEncoding(void* ctxhdr, const AudioFormat* audioFmt)
{
    return Invoke([&] {
        auto& control = Control(ctxhdr);
        CheckResult(control.Start(audioFmt), control);
    });
}

uint32_t WINAPI Plug_EncodeFrames(void* ctx, const void* pBuf, uint32_t frames)
{
    uint32_t written = 0;
    Invoke([&] {
        auto& control = Control(ctx);
        written = control.Encode(pBuf, frames);
        if (!control.GetErrorMessage().empty()) Error(control.GetErrorMessage().c_str());
    });
    return written;
}

int WINAPI Plug_StopEncoding(void* ctxhdr)
{
    return Invoke([&] {
        auto& control = Control(ctxhdr);
        CheckResult(control.Stop(), control);
    });
}
