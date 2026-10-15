#include <cstring>

#include "PluginMain.h"

namespace
{
thread_local char errorMsg[256] = {};

int NotImplemented()
{
    strcpy_s(errorMsg, "Encoding is not implemented in this empty plugin.");
    return -1;
}

int InvalidParameter()
{
    strcpy_s(errorMsg, "Invalid plugin parameter or structure size.");
    return -1;
}
}

int WINAPI Plug_OnRegister(const ApplicationConfig* app, PluginRegister* regInfo)
{
    if (app == nullptr || regInfo == nullptr || regInfo->size != sizeof(PluginRegister)) {
        return InvalidParameter();
    }

    regInfo->format_count = 0;
    errorMsg[0] = '\0';
    return 0;
}

void WINAPI Plug_GetErrMessage(char* msgBuf, uint32_t bufSize)
{
    if (msgBuf == nullptr || bufSize == 0) {
        return;
    }

    strncpy_s(msgBuf, bufSize, errorMsg, _TRUNCATE);
}

int WINAPI Plug_GetPluginInformation(PluginInfo* info)
{
    if (info == nullptr || info->size != sizeof(PluginInfo)) {
        return InvalidParameter();
    }

    info->plug_type = PluginType::Encoder;
    info->ver_major = 0;
    info->ver_minor = 1;
    strcpy_s(info->name, "Libsndfile encoder");
    strcpy_s(info->publisher, "imPlayer Group");
    errorMsg[0] = '\0';
    return 0;
}

int WINAPI Plug_GetEncodingFormatsDefine(PluginEncodingFormatsDefine* fmt)
{
    if (fmt == nullptr || fmt->size != sizeof(PluginEncodingFormatsDefine)) {
        return InvalidParameter();
    }

    fmt->fmtDef.clear();
    errorMsg[0] = '\0';
    return 0;
}

int WINAPI Plug_GetEncodingParamtersDefine(uint32_t streamFmt, PluginEncodingParamtersDefine* param)
{
    if (param == nullptr || param->size != sizeof(PluginEncodingParamtersDefine)) {
        return InvalidParameter();
    }

    param->paramDef.clear();
    return NotImplemented();
}

void* WINAPI Plug_OnInitialize(const EncoderPluginInitialize* init)
{
    NotImplemented();
    return nullptr;
}

int WINAPI Plug_OpenEncoder(void* ctxhdr, const EncoderPluginOpen* open)
{
    return NotImplemented();
}

void WINAPI Plug_CloseEncoder(void* ctxhdr, const EncoderPluginClose* close)
{
    // The empty plugin creates no context and owns no output stream.
}

int WINAPI Plug_SetMetaInfo(void* ctxhdr, const AudioMetaTags* metaTags)
{
    return NotImplemented();
}

int WINAPI Plug_StartEncoding(void* ctxhdr, const AudioFormat* audioFmt)
{
    return NotImplemented();
}

uint32_t WINAPI Plug_EncodeFrames(void* ctx, const void* pBuf, uint32_t frames)
{
    NotImplemented();
    return 0;
}

int WINAPI Plug_StopEncoding(void* ctxhdr)
{
    return NotImplemented();
}
