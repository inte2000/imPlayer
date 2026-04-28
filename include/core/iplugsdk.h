#pragma once

#include <cstdint>
#include "DataStream.h"
#include "MediaTag.h"
#include "MediaTagNames.h"
#include "AudioInfo.h"

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

#pragma pack(4)
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
    char publisher[PLUG_PUBLISHER_MAX_LIMIT];// utf8
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

typedef struct tagAudioContextHeader
{
    uint32_t size;
	unsigned long long filesize;
    long long m_totalFrames;
    float durations;
    uint32_t streamIndex;
    uint32_t streamCount;
}AudioContextHeader;

typedef struct tagPluginInitialize
{
    CDataStream* pStream;
    uint32_t streamFmt; // filetype, result of Plus_ParseFileTypeID()
    uint32_t mediaStreamIdx; //default open media stream index
    wchar_t hostpath[260];
}PluginInitialize;

typedef struct tagAudioMetaTags
{
    uint32_t size;
    uint32_t streamIdx;
    CMediaTag tags;
}AudioMetaTags;

typedef struct tagPluginStart
{
    uint32_t streamFmt; // filetype, result of Plus_ParseFileTypeID()
    uint32_t mediaStreamIdx;
}PluginStart;

const uint32_t AudioInfoFlagFormat = 0x00000001;
const uint32_t AudioInfoFlagActiveStream = 0x00000002;
const uint32_t AudioInfoFlagStreamCount = 0x00000004;
const uint32_t AudioInfoFlagCurrentFrames = 0x00000008;
const uint32_t AudioInfoFlagTotalFrames = 0x00000010;

const uint32_t AudioInfoFlagAll = 0x0000001F;

typedef struct tagPluginAudioInfo
{
    uint32_t size;
    uint32_t flags;
    AudioFormat audioFmt;
    uint32_t mediaStreamIdx;
    uint32_t mediaStreamCount;
    std::size_t currentFrames;
    std::size_t totalFrames;
}PluginAudioInfo;

#pragma pack()

