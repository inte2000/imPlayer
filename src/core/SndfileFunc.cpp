#include <format>
#include "UnicodeConvert.h"
#include "SndfileFunc.h"

std::string_view SndfileGetFileTypeName(uint32_t type)
{
    std::string_view name;
    switch (type)
    {
    case SF_FORMAT_WAV: name = "WAV"; break;
    case SF_FORMAT_AIFF: name = "AIFF"; break;
    case SF_FORMAT_AU: name = "AU"; break;
    case SF_FORMAT_RAW: name = "RAW PCM"; break;
    case SF_FORMAT_PAF: name = "PARIS"; break;
    case SF_FORMAT_SVX: name = "SVX8/SV16"; break;
    case SF_FORMAT_NIST: name = "NIST"; break;
    case SF_FORMAT_VOC: name = "VOC"; break;
    case SF_FORMAT_IRCAM: name = "IRCAM/CARL"; break;
    case SF_FORMAT_W64: name = "RIFF/WAV 64"; break;
    case SF_FORMAT_MAT4: name = "Matlab 4"; break;
    case SF_FORMAT_MAT5: name = "Matlab 5"; break;
    case SF_FORMAT_PVF: name = "PVF"; break;
    case SF_FORMAT_XI: name = "XI"; break;
    case SF_FORMAT_HTK: name = "HMM"; break;
    case SF_FORMAT_SDS: name = "Midi SDS"; break;
    case SF_FORMAT_AVR: name = "AVR"; break;
    case SF_FORMAT_WAVEX: name = "WAVEX"; break;
    case SF_FORMAT_SD2: name = "SD2"; break;
    case SF_FORMAT_FLAC: name = "FLAC"; break;
    case SF_FORMAT_CAF: name = "CAF"; break;
    case SF_FORMAT_WVE: name = "WVE"; break;
    case SF_FORMAT_OGG: name = "OGG"; break;
    case SF_FORMAT_MPC2K: name = "MPC 2000"; break;
    case SF_FORMAT_RF64: name = "RF64 WAV"; break;
    case SF_FORMAT_MPEG: name = "MPEG-1/2"; break;
    default: name = "Unknown"; break;
    }

    return name;
}

//playback 只识别已知类型，其他的都返回 Native，playback 会要求 decoder 将
//其转码成识别类型，目前 playback 优先选择 Float32
AudioDataFormat SndfileTransSubType(int sndsubtype)
{
    AudioDataFormat fmt = AudioDataFormat::Native;
    switch (sndsubtype)
    {
    case SF_FORMAT_PCM_S8: fmt = AudioDataFormat::PCM_S8; break;
    case SF_FORMAT_PCM_16: fmt = AudioDataFormat::PCM_S16; break;
    case SF_FORMAT_PCM_24: fmt = AudioDataFormat::PCM_S24; break;
    case SF_FORMAT_PCM_32: fmt = AudioDataFormat::PCM_S32; break;
    case SF_FORMAT_PCM_U8: fmt = AudioDataFormat::PCM_U8; break;
    case SF_FORMAT_FLOAT: fmt = AudioDataFormat::Float32; break;
    case SF_FORMAT_DOUBLE: fmt = AudioDataFormat::Float64; break;
    case SF_FORMAT_ULAW: fmt = AudioDataFormat::Ulaw; break;
    case SF_FORMAT_ALAW: fmt = AudioDataFormat::Alaw; break;
    case SF_FORMAT_IMA_ADPCM: fmt = AudioDataFormat::ImaAdpcm; break;
    case SF_FORMAT_MS_ADPCM: fmt = AudioDataFormat::MsAdpcm; break;
    case SF_FORMAT_GSM610: fmt = AudioDataFormat::Gsm610; break;
    case SF_FORMAT_G721_32: fmt = AudioDataFormat::G721Adpcm; break;
    case SF_FORMAT_G723_24: fmt = AudioDataFormat::G723Adpcm_24; break;
    case SF_FORMAT_G723_40: fmt = AudioDataFormat::G723Adpcm_40; break;
    case SF_FORMAT_MPEG_LAYER_III: fmt = AudioDataFormat::MpegLayer3; break;
    case SF_FORMAT_ALAC_16: fmt = AudioDataFormat::Alac16; break;
    case SF_FORMAT_ALAC_20: fmt = AudioDataFormat::Alac20; break;
    case SF_FORMAT_ALAC_24: fmt = AudioDataFormat::Alac24; break;
    case SF_FORMAT_ALAC_32: fmt = AudioDataFormat::Alac32; break;
    case SF_FORMAT_DPCM_8: fmt = AudioDataFormat::Dpcm_8; break;
    case SF_FORMAT_DPCM_16: fmt = AudioDataFormat::Dpcm_16; break;
    case SF_FORMAT_VORBIS: fmt = AudioDataFormat::Vorbis; break;
    case SF_FORMAT_OPUS: fmt = AudioDataFormat::Opus; break;
    default: break;
    }

    return fmt;
}

int SndfileGetSubType(AudioDataFormat format)
{
    switch (format) {
    case AudioDataFormat::PCM_U8:    return SF_FORMAT_PCM_U8;
    case AudioDataFormat::PCM_S8:    return SF_FORMAT_PCM_S8;
    case AudioDataFormat::PCM_S16:   return SF_FORMAT_PCM_16;
    case AudioDataFormat::PCM_S24:   return SF_FORMAT_PCM_24;
    case AudioDataFormat::PCM_S32:   return SF_FORMAT_PCM_32;
    case AudioDataFormat::Float32:   return SF_FORMAT_FLOAT;
    case AudioDataFormat::Float64:   return SF_FORMAT_DOUBLE;
    case AudioDataFormat::Ulaw:      return SF_FORMAT_ULAW;
    case AudioDataFormat::Alaw:      return SF_FORMAT_ALAW;
    case AudioDataFormat::ImaAdpcm:  return SF_FORMAT_IMA_ADPCM;
    case AudioDataFormat::MsAdpcm:   return SF_FORMAT_MS_ADPCM;
    case AudioDataFormat::Gsm610:   return SF_FORMAT_GSM610;
    case AudioDataFormat::G721Adpcm:   return SF_FORMAT_G721_32;
    case AudioDataFormat::G723Adpcm_24:   return SF_FORMAT_G723_24;
    case AudioDataFormat::G723Adpcm_40:   return SF_FORMAT_G723_40;
    case AudioDataFormat::MpegLayer3:   return SF_FORMAT_MPEG_LAYER_III;
    case AudioDataFormat::Alac16:   return SF_FORMAT_ALAC_16;
    case AudioDataFormat::Alac20:   return SF_FORMAT_ALAC_20;
    case AudioDataFormat::Alac24:   return SF_FORMAT_ALAC_24;
    case AudioDataFormat::Alac32:   return SF_FORMAT_ALAC_32;
    case AudioDataFormat::Dpcm_8:   return SF_FORMAT_DPCM_8;
    case AudioDataFormat::Dpcm_16:   return SF_FORMAT_DPCM_16;
    case AudioDataFormat::Vorbis:   return SF_FORMAT_VORBIS;
    case AudioDataFormat::Opus:   return SF_FORMAT_OPUS;
    case AudioDataFormat::Native:    return SF_FORMAT_PCM_16; // 默认使用16位
    default:                        return 0; // 表示不支持
    }
}

uint32_t SndfileStreamFmtToFileType(uint32_t streamFmt)
{
    switch (streamFmt) {
    case StreamFormatWav:    return SF_FORMAT_WAV;
    case StreamFormatAiff:    return SF_FORMAT_AIFF;/* Apple/SGI AIFF format (big endian). */
    case StreamFormatAu:    return SF_FORMAT_AU;/* Sun/NeXT AU format (big endian). */
    case StreamFormatRaw:    return SF_FORMAT_RAW;/* RAW PCM data. */
    case StreamFormatPaf:    return SF_FORMAT_PAF;/* Ensoniq PARIS file format. */
    case StreamFormatSvx:    return SF_FORMAT_SVX;/* Amiga IFF / SVX8 / SV16 format. */
    case StreamFormatNist:    return SF_FORMAT_NIST;/* Sphere NIST format. */
    case StreamFormatVoc:    return SF_FORMAT_VOC;/* VOC files. */
    case StreamFormatIrcam:    return SF_FORMAT_IRCAM;/* Berkeley/IRCAM/CARL */
    case StreamFormatW64:    return SF_FORMAT_W64;/* Sonic Foundry's 64 bit RIFF/WAV */
    case StreamFormatMat4:    return SF_FORMAT_MAT4;/* Matlab (tm) V4.2 / GNU Octave 2.0 */
    case StreamFormatMat5:    return SF_FORMAT_MAT5;/* Matlab (tm) V5.0 / GNU Octave 2.1 */
    case StreamFormatPvf:    return SF_FORMAT_PVF;/* Portable Voice Format */
    case StreamFormatXi:    return SF_FORMAT_XI;/* Fasttracker 2 Extended Instrument */
    case StreamFormatHtk:    return SF_FORMAT_HTK;/* HMM Tool Kit format */
    case StreamFormatMidSds:    return SF_FORMAT_SDS;/* Midi Sample Dump Standard */
    case StreamFormatAvr:    return SF_FORMAT_AVR;/* Audio Visual Research */
    case StreamFormatWavEx:    return SF_FORMAT_WAVEX;/* MS WAVE with WAVEFORMATEX */
    case StreamFormatSd2:    return SF_FORMAT_SD2;/* Sound Designer 2 */
    case StreamFormatFlac:    return SF_FORMAT_FLAC;/* FLAC lossless file format */
    case StreamFormatCaf:    return SF_FORMAT_CAF;/* Core Audio File format */
    case StreamFormatWve:    return SF_FORMAT_WVE;	/* Psion WVE format */
    case StreamFormatOgg:    return SF_FORMAT_OGG;/* Xiph OGG container */
    case StreamFormatMPC2k:    return SF_FORMAT_MPC2K;/* Akai MPC 2000 sampler */
    case StreamFormatWav64:    return SF_FORMAT_RF64;/* RF64 WAV file */
    case StreamFormatMp3:    return SF_FORMAT_MPEG;/* MPEG-1/2 audio stream */
    default:  return 0; // 表示不支持

    }

}

float LibsndFileGetSeconds(const std::wstring& filename)
{
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    std::string ansifilename = Utf16LeToLocalMBCS(filename);
    SNDFILE* file = sf_open(ansifilename.c_str(), SFM_READ, &sfinfo);
    if (!file)
        return 0.0f;

    // 检查格式是否为 FLAC
    //uint32_t subType = sfinfo.format & SF_FORMAT_TYPEMASK;
    //bool isFlac = ((sfinfo.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_FLAC);
    float seconds = float(sfinfo.frames) / float(sfinfo.samplerate);
    sf_close(file);

    return seconds;
}
