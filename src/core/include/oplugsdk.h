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

// Host and plugin must use compatible MSVC/STL and runtime configurations.
#pragma pack(push, 4)
typedef struct tagPluginEncodingFormatsDefine
{
    uint32_t size;
    std::vector<EncodingFormatDesc> fmtDef;
}PluginEncodingFormatsDefine;

typedef struct tagPluginEncodingParamtersDefine
{
    uint32_t size;
    std::vector<EncoderParamterDefine> paramDef;
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
    std::vector<EncoderParamter> params;
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
