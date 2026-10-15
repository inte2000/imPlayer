#pragma once

#include <cstdint>

#include "MediaTag.h"

enum class PluginType
{
    Decoder = 0,
    Encoder = 1,
    Filter = 2,
    Device = 3
};

const uint32_t PLUG_NAME_MAX_LIMIT = 64;
const uint32_t PLUG_PUBLISHER_MAX_LIMIT = 128;
const uint32_t PLUG_FORMAT_MAX_LIMIT = 128;

#pragma pack(push, 4)
typedef struct tagApplicationConfig
{
    uint32_t major_ver;
    uint32_t minor_ver;
    uint32_t FileTypeIdBegin;
}ApplicationConfig;

typedef struct tagPluginInfo
{
    uint32_t size;
    PluginType plug_type;
    uint32_t ver_major;
    uint32_t ver_minor;
    char name[PLUG_NAME_MAX_LIMIT]; //utf8
    char publisher[PLUG_PUBLISHER_MAX_LIMIT]; //utf8
}PluginInfo;

typedef struct tagFileFormatReg
{
    uint32_t id;
    char desc[128];
    char ext_list[128]; //utf8
}FileFormatReg;

typedef struct tagPluginRegister
{
    uint32_t size;
    uint32_t format_count;
    FileFormatReg fmt_reg[PLUG_FORMAT_MAX_LIMIT];
}PluginRegister;

typedef struct tagAudioMetaTags
{
    uint32_t size;
    uint32_t streamIdx;
    CMediaTag tags;
}AudioMetaTags;
#pragma pack(pop)
