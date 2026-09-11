/*
20260523 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_53.txt

修改记录：
大模型：ChatGPT 5.3 Codex
todo_task_54.txt
todo_task_57.txt
todo_task_58.txt
todo_task_59.txt
*/
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include "FfmpegFunc.h"

namespace {

std::string ToLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool NameContains(const std::string& names, const char* key)
{
    return names.find(key) != std::string::npos;
}

uint32_t StreamFormatFromCodecId(int codecId)
{
    switch (codecId)
    {
    case AV_CODEC_ID_MP3: return StreamFormatMp3;
    case AV_CODEC_ID_MP2: return StreamFormatMp2;
    case AV_CODEC_ID_MP1: return StreamFormatMp1;
    case AV_CODEC_ID_FLAC: return StreamFormatFlac;
    case AV_CODEC_ID_VORBIS:
    case AV_CODEC_ID_OPUS: return StreamFormatOgg;
    case AV_CODEC_ID_AAC: return StreamFormatAac;
    case AV_CODEC_ID_AC3: return StreamFormatAc3_4;
    case AV_CODEC_ID_DTS: return StreamFormatDts;
    case AV_CODEC_ID_APE: return StreamFormatApe;
    case AV_CODEC_ID_AMR_NB:
    case AV_CODEC_ID_AMR_WB: return StreamFormatAmr;
    case AV_CODEC_ID_DSD_LSBF:
    case AV_CODEC_ID_DSD_MSBF:
    case AV_CODEC_ID_DSD_LSBF_PLANAR:
    case AV_CODEC_ID_DSD_MSBF_PLANAR: return StreamFormatDsf;
    case AV_CODEC_ID_WMAV1:
    case AV_CODEC_ID_WMAV2:
    case AV_CODEC_ID_WMAPRO:
    case AV_CODEC_ID_WMALOSSLESS: return StreamFormatWma;
    case AV_CODEC_ID_ALAC: return StreamFormatM4a;
    default:
        return StreamFormatUnknown;
    }
}

uint32_t StreamFormatFromInputFormatName(const char* inputFmtName)
{
    if (inputFmtName == nullptr) {
        return StreamFormatUnknown;
    }

    const std::string names = ToLower(inputFmtName);
    if (NameContains(names, "mp3")) return StreamFormatMp3;
    if (NameContains(names, "flac")) return StreamFormatFlac;
    if (NameContains(names, "ogg") || NameContains(names, "opus")) return StreamFormatOgg;
    if (NameContains(names, "wav")) return StreamFormatWav;
    if (NameContains(names, "aiff")) return StreamFormatAiff;
    if (NameContains(names, "caf")) return StreamFormatCaf;
    if (NameContains(names, "aac")) return StreamFormatAac;
    if (NameContains(names, "ac3")) return StreamFormatAc3_4;
    if (NameContains(names, "dts")) return StreamFormatDts;
    if (NameContains(names, "dsf")) return StreamFormatDsf;
    if (NameContains(names, "dff") || NameContains(names, "dsdiff")) return StreamFormatDff;
    if (NameContains(names, "amr")) return StreamFormatAmr;
    if (NameContains(names, "asf")) return StreamFormatAsf;
    if (NameContains(names, "mov") || NameContains(names, "mp4") || NameContains(names, "3gp") || NameContains(names, "m4a")) return StreamFormatMp4;
    if (NameContains(names, "wmv")) return StreamFormatWmv;
    if (NameContains(names, "ape")) return StreamFormatApe;
    if (NameContains(names, "tta")) return StreamFormatTta;
    return StreamFormatUnknown;
}

uint32_t StreamFormatFromExtension(const char* filenameUtf8)
{
    if (filenameUtf8 == nullptr || filenameUtf8[0] == '\0') {
        return StreamFormatUnknown;
    }

    const std::string ext = ToLower(std::filesystem::path(filenameUtf8).extension().string());
    if (ext == ".mp3") return StreamFormatMp3;
    if (ext == ".mp2") return StreamFormatMp2;
    if (ext == ".mp1") return StreamFormatMp1;
    if (ext == ".flac") return StreamFormatFlac;
    if ((ext == ".ogg") || (ext == ".oga") || (ext == ".opus")) return StreamFormatOgg;
    if (ext == ".wav") return StreamFormatWav;
    if ((ext == ".aiff") || (ext == ".aif")) return StreamFormatAiff;
    if (ext == ".m4a") return StreamFormatM4a;
    if (ext == ".aac") return StreamFormatAac;
    if ((ext == ".mp4") || (ext == ".mov") || (ext == ".m4v") || (ext == ".3gp")) return StreamFormatMp4;
    if ((ext == ".asf") || (ext == ".wma")) return StreamFormatAsf;
    if (ext == ".wmv") return StreamFormatWmv;
    if ((ext == ".dts") || (ext == ".dtshd")) return StreamFormatDts;
    if (ext == ".dsf") return StreamFormatDsf;
    if ((ext == ".dff") || (ext == ".dsdiff")) return StreamFormatDff;
    if (ext == ".ape") return StreamFormatApe;
    if ((ext == ".amr") || (ext == ".awb")) return StreamFormatAmr;
    return StreamFormatUnknown;
}

} // namespace

uint32_t StreamFormatFromFfmpeg(const char* inputFmtName, const char* filenameUtf8, int audioCodecId)
{
    uint32_t streamFmt = StreamFormatFromInputFormatName(inputFmtName);
    if (streamFmt != StreamFormatUnknown) {
        return streamFmt;
    }

    streamFmt = StreamFormatFromCodecId(audioCodecId);
    if (streamFmt != StreamFormatUnknown) {
        return streamFmt;
    }

    return StreamFormatFromExtension(filenameUtf8);
}

uint32_t ParseStreamFormatByFfmpeg(const char* filenameUtf8)
{
    if (filenameUtf8 == nullptr || filenameUtf8[0] == '\0') {
        return StreamFormatUnknown;
    }

    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, filenameUtf8, nullptr, nullptr) < 0) {
        return StreamFormatUnknown;
    }

    avformat_find_stream_info(fmtCtx, nullptr);

    int codecId = AV_CODEC_ID_NONE;
    for (unsigned int i = 0; i < fmtCtx->nb_streams; ++i)
    {
        const AVStream* stream = fmtCtx->streams[i];
        if (stream && stream->codecpar && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            codecId = stream->codecpar->codec_id;
            break;
        }
    }

    const char* fmtName = (fmtCtx->iformat == nullptr) ? nullptr : fmtCtx->iformat->name;
    const uint32_t streamFmt = StreamFormatFromFfmpeg(fmtName, filenameUtf8, codecId);

    avformat_close_input(&fmtCtx);
    return streamFmt;
}

AudioDataFormat AudioDataFormatFromFfmpegCodec(int codecId)
{
    switch (codecId)
    {
    case AV_CODEC_ID_MP3:
    case AV_CODEC_ID_MP2:
    case AV_CODEC_ID_MP1:
        return AudioDataFormat::MpegLayer3;
    case AV_CODEC_ID_VORBIS:
        return AudioDataFormat::Vorbis;
    case AV_CODEC_ID_OPUS:
        return AudioDataFormat::Opus;
    case AV_CODEC_ID_PCM_U8:
        return AudioDataFormat::PCM_U8;
    case AV_CODEC_ID_PCM_S8:
        return AudioDataFormat::PCM_S8;
    case AV_CODEC_ID_PCM_S16LE:
    case AV_CODEC_ID_PCM_S16BE:
        return AudioDataFormat::PCM_S16;
    case AV_CODEC_ID_PCM_S24LE:
    case AV_CODEC_ID_PCM_S24BE:
        return AudioDataFormat::PCM_S24;
    case AV_CODEC_ID_PCM_S32LE:
    case AV_CODEC_ID_PCM_S32BE:
        return AudioDataFormat::PCM_S32;
    case AV_CODEC_ID_PCM_F32LE:
    case AV_CODEC_ID_PCM_F32BE:
        return AudioDataFormat::Float32;
    case AV_CODEC_ID_PCM_F64LE:
    case AV_CODEC_ID_PCM_F64BE:
        return AudioDataFormat::Float64;
    default:
        return AudioDataFormat::UNKNOWN;
    }
}

const char* FfmpegFormatName(uint32_t streamFmt)
{
    switch (streamFmt)
    {
    case StreamFormatMp3: return "MP3";
    case StreamFormatMp2: return "MP2";
    case StreamFormatMp1: return "MP1";
    case StreamFormatFlac: return "FLAC";
    case StreamFormatOgg: return "OGG";
    case StreamFormatWav: return "WAV";
    case StreamFormatAiff: return "AIFF";
    case StreamFormatM4a: return "M4A";
    case StreamFormatMp4: return "MP4";
    case StreamFormatAsf: return "ASF";
    case StreamFormatWmv: return "WMV";
    case StreamFormatDts: return "DTS";
    case StreamFormatDsf: return "DSF";
    case StreamFormatDff: return "DFF";
    case StreamFormatApe: return "APE";
    case StreamFormatAmr: return "AMR";
    default:
        return "FFmpeg";
    }
}
