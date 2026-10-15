#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "AudioInfo.h"
#include "DataStream.h"
#include "EncoderParamterDefine.h"
#include "EncodingParams.h"
#include "plugsdk.h"

// Host and plugin must use compatible MSVC/STL and runtime configurations.
#pragma pack(push, 4)
typedef struct tagEncodingFormatDesc
{
    uint32_t streamFmt;
    std::string desc;
    std::string extname;
}EncodingFormatDesc;

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
    CDataStream* pStream; // Host-owned output stream; the plugin must not delete it.
    std::vector<EncoderParamter> params;
}EncoderPluginOpen;

typedef struct tagEncoderPluginClose
{
    uint32_t size;
}EncoderPluginClose;
#pragma pack(pop)
