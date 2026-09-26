/*
20260508 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_35.txt

修改任务：
todo_task_42.txt
*/
#include "utils/DataLoader.h"
#include "utils/FileLoader.h"
#include "utils/MemoryLoader.h"
#include "player/vgmplayer.hpp"
#include "player/droplayer.hpp"
#include "player/s98player.hpp"
#include "player/gymplayer.hpp"
#include "AudioInfo.h"
#include "LibvgmFunc.h"

#include <vector>

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

uint32_t ParseLoader(DATA_LOADER* loader)
{
    if (loader == nullptr) {
        return StreamFormatUnknown;
    }

    if (VGMPlayer::PlayerCanLoadFile(loader) == 0x00) {
        return StreamFormatVgmVgz;
    }
    if (S98Player::PlayerCanLoadFile(loader) == 0x00) {
        return ::LibvgmFormatS98();
    }
    if (DROPlayer::PlayerCanLoadFile(loader) == 0x00) {
        return DetectDroStreamType(loader);
    }
    if (GYMPlayer::PlayerCanLoadFile(loader) == 0x00) {
        return ::LibvgmFormatGym();
    }

    return StreamFormatUnknown;
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

uint32_t ParseStreamFormatByLibvgm(const char* filenameUtf8, CDataStream* pStream)
{
    if ((filenameUtf8 != nullptr) && (filenameUtf8[0] != '\0')) {
        DATA_LOADER* loader = FileLoader_Init(filenameUtf8);
        if (loader == nullptr) {
            return StreamFormatUnknown;
        }

        FileLoader_SetPreloadBytes(loader, 0x200);
        if (FileLoader_Load(loader) != 0x00) {
            FileLoader_Deinit(loader);
            return StreamFormatUnknown;
        }

        const uint32_t streamFmt = ParseLoader(loader);
        FileLoader_Deinit(loader);
        return streamFmt;
    }

    if (pStream == nullptr) {
        return StreamFormatUnknown;
    }
    const DataStreamStyle style = pStream->GetStyle();
    if (((style & dsStyleSeekable) == 0) || ((style & dsStyleTellPos) == 0)) {
        return StreamFormatUnknown;
    }

    const std::size_t totalSize = pStream->GetLength();
    if ((totalSize == 0) || (totalSize > static_cast<std::size_t>(UINT32_MAX))) {
        return StreamFormatUnknown;
    }

    const std::size_t oldPos = pStream->Tell();
    pStream->Seek(SeekBase::Begin, 0);

    std::vector<uint8_t> fileData(totalSize);
    std::size_t done = 0;
    while (done < fileData.size())
    {
        const uint32_t once = pStream->Read(fileData.data() + done, static_cast<uint32_t>(fileData.size() - done));
        if (once == 0) {
            break;
        }
        done += once;
    }
    pStream->Seek(SeekBase::Begin, static_cast<long long>(oldPos));
    if (done != fileData.size()) {
        return StreamFormatUnknown;
    }

    DATA_LOADER* loader = MemoryLoader_Init(fileData.data(), static_cast<UINT32>(fileData.size()));
    if (loader == nullptr) {
        return StreamFormatUnknown;
    }

    DataLoader_SetPreloadBytes(loader, 0x200);
    if (DataLoader_Load(loader) != 0x00) {
        DataLoader_Deinit(loader);
        return StreamFormatUnknown;
    }

    const uint32_t streamFmt = ParseLoader(loader);
    DataLoader_Deinit(loader);
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
