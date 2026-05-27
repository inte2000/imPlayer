/*
20260508 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_35.txt

修改任务：
todo_task_42.txt
*/
#include "utils/DataLoader.h"
#include "utils/FileLoader.h"
#include "player/vgmplayer.hpp"
#include "player/droplayer.hpp"
#include "player/s98player.hpp"
#include "player/gymplayer.hpp"
#include "AudioInfo.h"
#include "LibvgmFunc.h"

namespace
{
constexpr uint32_t LIBVGM_CUSTOM_FMT_S98_OFFSET = 0;
constexpr uint32_t LIBVGM_CUSTOM_FMT_GYM_OFFSET = 1;

uint32_t g_formatIdBase = StreamFormatPlusBegin;

uint32_t ReadLE32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}

uint32_t DetectDroStreamType(DATA_LOADER* loader)
{
    DataLoader_ReadUntil(loader, 0x10);
    if (DataLoader_GetSize(loader) < 0x10) {
        return StreamFormatUnknown;
    }

    const uint8_t* data = DataLoader_GetData(loader);
    const uint32_t versionField = ReadLE32(&data[0x08]);
    if ((versionField & 0xFF00FF00) != 0) {
        return StreamFormatDro;
    }
    if ((versionField & 0x0000FFFF) == 0) {
        return StreamFormatDro;
    }
    return StreamFormatDro2;
}
}

void SetLibvgmCustomFormatBase(uint32_t formatIdBase)
{
    g_formatIdBase = formatIdBase;
}

uint32_t LibvgmFormatS98()
{
    return g_formatIdBase + LIBVGM_CUSTOM_FMT_S98_OFFSET;
}

uint32_t LibvgmFormatGym()
{
    return g_formatIdBase + LIBVGM_CUSTOM_FMT_GYM_OFFSET;
}

uint32_t ParseStreamFormatByLibvgm(const char* filenameUtf8)
{
    if (filenameUtf8 == nullptr || filenameUtf8[0] == '\0') {
        return StreamFormatUnknown;
    }

    DATA_LOADER* loader = FileLoader_Init(filenameUtf8);
    if (loader == nullptr) {
        return StreamFormatUnknown;
    }

    FileLoader_SetPreloadBytes(loader, 0x200);
    if (FileLoader_Load(loader) != 0x00) {
        FileLoader_Deinit(loader);
        return StreamFormatUnknown;
    }

    uint32_t streamFmt = StreamFormatUnknown;
    if (VGMPlayer::PlayerCanLoadFile(loader) == 0x00) {
        streamFmt = StreamFormatVgmVgz;
    }
    else if (S98Player::PlayerCanLoadFile(loader) == 0x00) {
        streamFmt = LibvgmFormatS98();
    }
    else if (DROPlayer::PlayerCanLoadFile(loader) == 0x00) {
        streamFmt = DetectDroStreamType(loader);
    }
    else if (GYMPlayer::PlayerCanLoadFile(loader) == 0x00) {
        streamFmt = LibvgmFormatGym();
    }

    FileLoader_Deinit(loader);
    return streamFmt;
}

const char* LibvgmFormatName(uint32_t streamFmt)
{
    switch (streamFmt)
    {
    case StreamFormatVgmVgz:
        return "VGM/VGZ";
    case StreamFormatDro:
        return "DRO";
    case StreamFormatDro2:
        return "DRO v2";
    default:
        if (streamFmt == LibvgmFormatS98()) {
            return "S98";
        }
        if (streamFmt == LibvgmFormatGym()) {
            return "GYM";
        }
        return "Video game music";
    }
}
