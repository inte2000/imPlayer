#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "AudioInfo.h"
#include "DataStream.h"
#include "EncoderParamterDefine.h"
#include "EncoderFormatDefine.h"
#include "EncodingParams.h"
#include "plugsdk.h"

using EncodingFormatDesc = EncoderFormatDefine;
constexpr uint32_t EncoderFormatItemsLimit = 256;

// Fixed-layout encoder payloads. CDataStream/CMediaTag remain C++ interfaces.
#pragma pack(push, 4)
typedef struct tagPluginEncodingFormatsDefine
{
    uint32_t size;
    EncodingFormatDesc fmtDef[EncoderFormatItemsLimit];
    uint32_t count;
}PluginEncodingFormatsDefine;

typedef struct tagPluginEncodingParamtersDefine
{
    uint32_t size;
    EncoderParamterDefine paramDef[EncoderParamItemsLimit];
    uint32_t count;
}PluginEncodingParamtersDefine;

typedef struct tagEncoderPluginInitialize
{
    uint32_t size;
    wchar_t hostpath[260];
}EncoderPluginInitialize;

typedef struct tagEncoderPluginOpen
{
    uint32_t size;
    uint32_t streamFmt;
    uint32_t mediaStreamIdx = 0;
    CDataStream* pStream; // Host-owned output stream; the plugin must not delete it.
    EncoderParamter params[EncoderParamItemsLimit];
    uint32_t count;
}EncoderPluginOpen;

typedef struct tagEncoderPluginClose
{
    uint32_t size;
    uint32_t mediaStreamIdx = 0;
}EncoderPluginClose;

typedef struct tagAudioEncodeContextHeader
{
    uint32_t size;
    AudioFormat transFmt;
}AudioEncodeContextHeader;
#pragma pack(pop)
