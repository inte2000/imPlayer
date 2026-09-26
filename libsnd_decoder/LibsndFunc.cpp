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

namespace {

sf_count_t GetLengthCb(void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    return (stream == nullptr) ? 0 : static_cast<sf_count_t>(stream->GetLength());
}

sf_count_t SeekCb(sf_count_t offset, int whence, void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    if (stream == nullptr) {
        return -1;
    }

    SeekBase base = SeekBase::Begin;
    switch (whence) {
    case SEEK_SET: base = SeekBase::Begin; break;
    case SEEK_CUR: base = SeekBase::Cur; break;
    case SEEK_END: base = SeekBase::End; break;
    default: return -1;
    }

    stream->Seek(base, offset);
    return static_cast<sf_count_t>(stream->Tell());
}

sf_count_t ReadCb(void* ptr, sf_count_t count, void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    if ((stream == nullptr) || (ptr == nullptr) || (count <= 0)) {
        return 0;
    }

    return stream->Read(ptr, static_cast<uint32_t>(count));
}

sf_count_t WriteCb(const void* ptr, sf_count_t count, void* userData)
{
    (void)ptr;
    (void)count;
    (void)userData;
    return 0;
}

sf_count_t TellCb(void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    return (stream == nullptr) ? 0 : static_cast<sf_count_t>(stream->Tell());
}

}

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

SNDFILE* SndfileOpenStream(CDataStream* stream, SF_INFO& sfInfo)
{
    if (stream == nullptr) {
        return nullptr;
    }

    SF_VIRTUAL_IO vio = {};
    vio.get_filelen = GetLengthCb;
    vio.seek = SeekCb;
    vio.read = ReadCb;
    vio.write = WriteCb;
    vio.tell = TellCb;

    return sf_open_virtual(&vio, SFM_READ, &sfInfo, stream);
}

uint32_t ParseStreamFormatByLibsndfile(const char* filenameUtf8)
{
    SF_INFO info = {};
    if ((filenameUtf8 == nullptr) || (filenameUtf8[0] == '\0')) {
        return StreamFormatUnknown;
    }

    SNDFILE* handle = sf_open(filenameUtf8, SFM_READ, &info);
    if (handle == nullptr) {
        return StreamFormatUnknown;
    }

    const uint32_t streamFormat = StreamFormatFromLibsndfileFormat(info.format);
    sf_close(handle);
    return streamFormat;
}

uint32_t ParseStreamFormatByLibsndStream(CDataStream* pStream)
{
    SF_INFO info = {};
    if (pStream == nullptr) {
        return StreamFormatUnknown;
    }
    const DataStreamStyle style = pStream->GetStyle();
    if (((style & dsStyleSeekable) == 0) || ((style & dsStyleTellPos) == 0)) {
        return StreamFormatUnknown;
    }

    const std::size_t oldPos = pStream->Tell();
    pStream->Seek(SeekBase::Begin, 0);
    SNDFILE* handle = SndfileOpenStream(pStream, info);
    uint32_t streamFormat = StreamFormatUnknown;
    if (handle != nullptr) {
        streamFormat = StreamFormatFromLibsndfileFormat(info.format);
        sf_close(handle);
    }
    pStream->Seek(SeekBase::Begin, static_cast<long long>(oldPos));
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
