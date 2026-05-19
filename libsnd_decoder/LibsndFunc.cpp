/*
20260424 AI 生成内容：
uint32_t StreamFormatFromLibsndfileFormat(int format);
uint32_t ParseStreamFormatByLibsndfile(const char* filenameUtf8);

大模型：GPT 5.3 Codex
任务：todo_task_12.txt


20260425 AI 生成内容：
std::string SndfileGetFileTypeName(uint32_t type);
AudioDataFormat SndfileTransSubType(int sndsubtype);

大模型：GPT 5.3 Codex
任务：todo_task_19.txt
*/
#include <sndfile.h>
#include "AudioInfo.h"
#include "LibsndFunc.h"

uint32_t StreamFormatFromLibsndfileFormat(int format)
{
    const int type = format & SF_FORMAT_TYPEMASK;

    switch (type) {
    case SF_FORMAT_WAV:
        return StreamFormatWav;
    case SF_FORMAT_AIFF:
        return StreamFormatAiff;
    case SF_FORMAT_AU:
        return StreamFormatAu;
    case SF_FORMAT_RAW:
        return StreamFormatRaw;
    case SF_FORMAT_PAF:
        return StreamFormatPaf;
    case SF_FORMAT_SVX:
        return StreamFormatSvx;
    case SF_FORMAT_NIST:
        return StreamFormatNist;
    case SF_FORMAT_VOC:
        return StreamFormatVoc;
    case SF_FORMAT_IRCAM:
        return StreamFormatIrcam;
    case SF_FORMAT_W64:
        return StreamFormatW64;
    case SF_FORMAT_MAT4:
    case SF_FORMAT_MAT5:
        return StreamFormatMat;
    case SF_FORMAT_PVF:
        return StreamFormatPvf;
    case SF_FORMAT_XI:
        return StreamFormatXi;
    case SF_FORMAT_HTK:
        return StreamFormatHtk;
    case SF_FORMAT_SDS:
        return StreamFormatMidSds;
    case SF_FORMAT_AVR:
        return StreamFormatAvr;
    case SF_FORMAT_WAVEX:
        return StreamFormatWavEx;
    case SF_FORMAT_SD2:
        return StreamFormatSd2;
    case SF_FORMAT_FLAC:
        return StreamFormatFlac;
    case SF_FORMAT_CAF:
        return StreamFormatCaf;
    case SF_FORMAT_WVE:
        return StreamFormatWve;
    case SF_FORMAT_OGG:
        return StreamFormatOgg;
    case SF_FORMAT_MPC2K:
        return StreamFormatMPC2k;
    case SF_FORMAT_RF64:
        return StreamFormatWav64;
#ifdef SF_FORMAT_MP3
    case SF_FORMAT_MP3:
        return StreamFormatMp3;
#endif
#ifdef SF_FORMAT_MPEG
    case SF_FORMAT_MPEG:
    {
        const int subType = format & SF_FORMAT_SUBMASK;
#ifdef SF_FORMAT_MPEG_LAYER_I
        if (subType == SF_FORMAT_MPEG_LAYER_I) {
            return StreamFormatMp1;
        }
#endif
#ifdef SF_FORMAT_MPEG_LAYER_II
        if (subType == SF_FORMAT_MPEG_LAYER_II) {
            return StreamFormatMp2;
        }
#endif
#ifdef SF_FORMAT_MPEG_LAYER_III
        if (subType == SF_FORMAT_MPEG_LAYER_III) {
            return StreamFormatMp3;
        }
#endif
        return StreamFormatUnknown;
    }
#endif
    default:
        return StreamFormatUnknown;
    }
}

uint32_t ParseStreamFormatByLibsndfile(const char* filenameUtf8)
{
    if (filenameUtf8 == nullptr || filenameUtf8[0] == '\0') {
        return StreamFormatUnknown;
    }

    SF_INFO info = {};
    SNDFILE* handle = sf_open(filenameUtf8, SFM_READ, &info);
    if (handle == nullptr) {
        return StreamFormatUnknown;
    }

    const uint32_t streamFormat = StreamFormatFromLibsndfileFormat(info.format);
    sf_close(handle);
    return streamFormat;
}

std::string SndfileGetFileTypeName(uint32_t type)
{
    switch (type)
    {
    case SF_FORMAT_WAV:
        return "WAV";
    case SF_FORMAT_AIFF:
        return "AIFF";
    case SF_FORMAT_AU:
        return "AU";
    case SF_FORMAT_RAW:
        return "RAW PCM";
    case SF_FORMAT_PAF:
        return "PAF";
    case SF_FORMAT_SVX:
        return "SVX8/SV16"; // Human action
    case SF_FORMAT_NIST:
        return "NIST";
    case SF_FORMAT_VOC:
        return "VOC";
    case SF_FORMAT_IRCAM:
        return "IRCAM/CARL"; // Human action
    case SF_FORMAT_W64:
        return "RIFF/WAV 64"; // Human action
    case SF_FORMAT_MAT4:
    case SF_FORMAT_MAT5:
        return "MAT";
    case SF_FORMAT_PVF:
        return "PVF";
    case SF_FORMAT_XI:
        return "XI";
    case SF_FORMAT_HTK:
        return "HTK";
    case SF_FORMAT_SDS:
        return "SDS";
    case SF_FORMAT_AVR:
        return "AVR";
    case SF_FORMAT_WAVEX:
        return "WAVEX";
    case SF_FORMAT_SD2:
        return "SD2";
    case SF_FORMAT_FLAC:
        return "FLAC";
    case SF_FORMAT_CAF:
        return "CAF";
    case SF_FORMAT_WVE:
        return "WVE";
    case SF_FORMAT_OGG:
        return "OGG";
    case SF_FORMAT_MPC2K:
        return "MPC2K";
    case SF_FORMAT_RF64:
        return "RF64 WAV"; // Human action
#ifdef SF_FORMAT_MP3
    case SF_FORMAT_MP3:
        return "MP3";
#endif
#ifdef SF_FORMAT_MPEG
    case SF_FORMAT_MPEG:
        return "MPEG-1/2";  // Human action
#endif
    default:
        return "Unknown";  // Human action
    }
}

AudioDataFormat SndfileTransSubType(int sndsubtype)
{
    switch (sndsubtype)
    {
    case SF_FORMAT_PCM_U8:
        return AudioDataFormat::PCM_U8;
    case SF_FORMAT_PCM_S8:
        return AudioDataFormat::PCM_S8;
    case SF_FORMAT_PCM_16:
        return AudioDataFormat::PCM_S16;
    case SF_FORMAT_PCM_24:
        return AudioDataFormat::PCM_S24;
    case SF_FORMAT_PCM_32:
        return AudioDataFormat::PCM_S32;
    case SF_FORMAT_FLOAT:
        return AudioDataFormat::Float32;
    case SF_FORMAT_DOUBLE:
        return AudioDataFormat::Float64;
    case SF_FORMAT_ULAW:
        return AudioDataFormat::Ulaw;
    case SF_FORMAT_ALAW:
        return AudioDataFormat::Alaw;
    case SF_FORMAT_IMA_ADPCM:
        return AudioDataFormat::ImaAdpcm;
    case SF_FORMAT_MS_ADPCM:
        return AudioDataFormat::MsAdpcm;
    case SF_FORMAT_GSM610:
        return AudioDataFormat::Gsm610;
    case SF_FORMAT_G721_32:
        return AudioDataFormat::G721Adpcm;
    case SF_FORMAT_G723_24:
        return AudioDataFormat::G723Adpcm_24;
    case SF_FORMAT_G723_40:
        return AudioDataFormat::G723Adpcm_40;
    case SF_FORMAT_VORBIS:
        return AudioDataFormat::Vorbis;
#ifdef SF_FORMAT_OPUS
    case SF_FORMAT_OPUS:
        return AudioDataFormat::Opus;
#endif
    case SF_FORMAT_ALAC_16:
        return AudioDataFormat::Alac16;
    case SF_FORMAT_ALAC_20:
        return AudioDataFormat::Alac20;
    case SF_FORMAT_ALAC_24:
        return AudioDataFormat::Alac24;
    case SF_FORMAT_ALAC_32:
        return AudioDataFormat::Alac32;
#ifdef SF_FORMAT_MPEG_LAYER_III
    case SF_FORMAT_MPEG_LAYER_III:
        return AudioDataFormat::MpegLayer3;
#endif
    default:
        return AudioDataFormat::UNKNOWN;
    }
}
