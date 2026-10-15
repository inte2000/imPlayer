#pragma once

#include <cstdint>
#include "plugsdk.h"
#include "DataStream.h"
#include "MediaTag.h"
#include "MediaTagNames.h"
#include "AudioInfo.h"

#pragma pack(4)
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
    uint32_t streamFmt; // filetype, result of Plug_ParseFileTypeID()
    uint32_t mediaStreamIdx; //default open media stream index
    wchar_t hostpath[260];
}PluginInitialize;

typedef struct tagPluginStart
{
    uint32_t streamFmt; // filetype, result of Plug_ParseFileTypeID()
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

