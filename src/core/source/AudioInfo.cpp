#include <fstream>
#include <stdexcept>
#include <format>
#include <cassert>
#include "AudioInfo.h"
#include "nlohmann/json.hpp" 

using json = nlohmann::json;



static TypeNameFormat s_TypenameFormat[] = {
    {"MPEG-1 Layer III", StreamFormatMp3, ".mp3" },
    {"WAV format", StreamFormatWav, ".wav" },
    {"RF64 WAV", StreamFormatWav64, ".wav" },
    {"AU", StreamFormatAu, ".au" },
    {"Audio Visual Research", StreamFormatAvr, ".avr" },
    {"CAF(Core Audio Format)", StreamFormatCaf, ".caf" },
    {"PARIS Audio Format", StreamFormatPaf, ".paf" },
    {"Amiga IFF SVX Audio", StreamFormatSvx, ".svx" },
    {"Sphere NIST Format", StreamFormatNist, ".sph" },
    {"IRCAM/CARL Sound Format", StreamFormatIrcam, ".sf" },
    {"Creative Labs(VOC)", StreamFormatVoc, ".voc" },
    {"Sonic Foundry's 64 bit RIFF/WAV", StreamFormatW64, ".w64" },
    //{"Matlab (tm) V4.2/GNU Octave 2.0", StreamFormatMat5, ".mat" },
    {"Matlab (tm) V5.0/GNU Octave 2.1", StreamFormatMat, ".mat" },
    {"Portable Voice Format", StreamFormatPvf, ".pvf" },
    {"Fasttracker 2 Extended Instrument", StreamFormatXi, ".xi" },
    {"Midi Sample Dump Standard", StreamFormatMidSds, ".sds" },
    //{"Sound Designer 2", StreamFormatSd2, ".sd2" },
    {"Psion WVE format", StreamFormatWve, ".wve" },
    {"AIFF", StreamFormatAiff, ".aiff" },
    {"FLAC", StreamFormatFlac, ".flac" },
    {"Xiph OGG container", StreamFormatOgg, ".ogg" },
    {"WV (Wavepack format)", StreamFormatWv, ".wv" },
    {"Musepack Compressed Audio File", StreamFormatMpc, ".mpc" },
    {"Sound/AKAI MPC", StreamFormatMPC2k, ".snd" },
    {"WMA (Windows mwdia audio)", StreamFormatWma, ".wma" },
    {nullptr, StreamFormatUnknown }
};

uint32_t GetBitsPerSampleByFormat(AudioDataFormat format)
{
    long bitsPerSample = 0;
    switch (format)
    {
    case AudioDataFormat::ImaAdpcm:
    case AudioDataFormat::MsAdpcm:
        bitsPerSample = 4;
        break;
    case AudioDataFormat::PCM_S8:
    case AudioDataFormat::PCM_U8:
    case AudioDataFormat::Ulaw:
    case AudioDataFormat::Alaw:
        bitsPerSample = 8;
        break;
    case AudioDataFormat::PCM_S16:
        bitsPerSample = 16;
        break;
    case AudioDataFormat::PCM_S24:
        bitsPerSample = 24;
        break;
    case AudioDataFormat::PCM_S24_32:
    case AudioDataFormat::PCM_S32:
    case AudioDataFormat::Float32:
        bitsPerSample = 32;
        break;
    case AudioDataFormat::PCM_64:
    case AudioDataFormat::Float64:
        bitsPerSample = 64;
        break;

    default:
        break;
    }

    return bitsPerSample;
}

std::string StringFromAudioFormat(AudioDataFormat format)
{
    std::string result = "unknown";
    switch (format)
    {
    case AudioDataFormat::ImaAdpcm: result = "ImaAdpcm"; break;
    case AudioDataFormat::MsAdpcm: result = "MsAdpcm"; break;
    case AudioDataFormat::PCM_S8: result = "PCM-S8"; break;
    case AudioDataFormat::PCM_U8: result = "PCM-U8"; break;
    case AudioDataFormat::Ulaw: result = "Ulaw"; break;
    case AudioDataFormat::Alaw: result = "Alaw"; break;
    case AudioDataFormat::PCM_S16: result = "PCM-S16"; break;
    case AudioDataFormat::PCM_S24: result = "PCM-S24"; break;
    case AudioDataFormat::PCM_S24_32: result = "PCM-S24_in_32"; break;
    case AudioDataFormat::PCM_S32: result = "PCM-S32"; break;
    case AudioDataFormat::Float32: result = "PCM-F32"; break;
    case AudioDataFormat::PCM_64: result = "PCM-S64"; break;
    case AudioDataFormat::Float64: result = "PCM-F64"; break;
    case AudioDataFormat::MpegLayer3: result = "MP3"; break;
    case AudioDataFormat::Alac16: result = "Alac16"; break;
    case AudioDataFormat::Alac20: result = "Alac20"; break;
    case AudioDataFormat::Alac24: result = "Alac24"; break;
    case AudioDataFormat::Alac32: result = "Alac32"; break;
    case AudioDataFormat::Dpcm_8: result = "Dpcm_8"; break;
    case AudioDataFormat::Dpcm_16: result = "Dpcm_16"; break;
    case AudioDataFormat::Vorbis: result = "Vorbis"; break;
    case AudioDataFormat::Opus: result = "Opus"; break;
    default: break;
    }

    return result;
}


std::string BitsPerSampleStringFromAudioFormat(AudioDataFormat wavAudioFormat)
{
    if ((wavAudioFormat == AudioDataFormat::ImaAdpcm) || (wavAudioFormat == AudioDataFormat::MsAdpcm))
        return "4 bits";
    if ((wavAudioFormat == AudioDataFormat::Ulaw) || (wavAudioFormat == AudioDataFormat::Alaw))
        return "8 bits";
    if (wavAudioFormat == AudioDataFormat::PCM_S8)
        return "s8 bits";
    if (wavAudioFormat == AudioDataFormat::PCM_U8)
        return "u8 bits";
    else if (wavAudioFormat == AudioDataFormat::PCM_S16)
        return "s16 bits";
    if (wavAudioFormat == AudioDataFormat::PCM_S24)
        return "s24 bits";
    if (wavAudioFormat == AudioDataFormat::PCM_S32)
        return "s32 bits";
    if (wavAudioFormat == AudioDataFormat::Float32)
        return "32 bits";
    if (wavAudioFormat == AudioDataFormat::Float64)
        return "64 bits";
    else
        return "4 bits";
}

//只用于解码输出的 PCM 编码格式说明
std::string PcmDescriptionFromFormat(AudioDataFormat wavAudioFormat)
{
    if (wavAudioFormat == AudioDataFormat::PCM_S8)
        return "s8 i";
    if (wavAudioFormat == AudioDataFormat::PCM_U8)
        return "u8 i";
    else if (wavAudioFormat == AudioDataFormat::PCM_S16)
        return "s16 i";
    if (wavAudioFormat == AudioDataFormat::PCM_S24)
        return "s24 i";
    if (wavAudioFormat == AudioDataFormat::PCM_S24_32)
        return "s24_32 i";
    if (wavAudioFormat == AudioDataFormat::PCM_S32)
        return "s32 i";
    if (wavAudioFormat == AudioDataFormat::Float32)
        return "f32 i";
    if (wavAudioFormat == AudioDataFormat::Float64)
        return "f64 i";
    if (wavAudioFormat == AudioDataFormat::PCM_S8P)
        return "s8 p";
    if (wavAudioFormat == AudioDataFormat::PCM_U8P)
        return "u8 p";
    else if (wavAudioFormat == AudioDataFormat::PCM_S16P)
        return "s16 p";
    if (wavAudioFormat == AudioDataFormat::PCM_S24P)
        return "s24 p";
    if (wavAudioFormat == AudioDataFormat::PCM_S24_32P)
        return "s24_32 p";
    if (wavAudioFormat == AudioDataFormat::PCM_S32P)
        return "s32 p";
    if (wavAudioFormat == AudioDataFormat::Float32P)
        return "f32 p";
    if (wavAudioFormat == AudioDataFormat::Float64P)
        return "f64 p";
    if (wavAudioFormat == AudioDataFormat::MpegLayer3)
        return "MP3";
    if (wavAudioFormat == AudioDataFormat::Vorbis)
        return "Vorbis";
    if (wavAudioFormat == AudioDataFormat::Opus)
        return "Opus";
    else
        return "unknown";
}

std::string SampleRateBrifStr(uint32_t rate)
{
    if (rate == 8000)
        return "8KHz";
    else if (rate == 16000)
        return "16KHz";
    else if (rate == 11025)
        return "11025Hz";
    else if (rate == 22050)
        return "22.05KHz";
    else if (rate == 32000)
        return "32KHz";
    else if (rate == 44100)
        return "44.1KHz";
    else if (rate == 48000)
        return "48KHz";
    else if (rate == 88200)
        return "88.2KHz";
    else if (rate == 96000)
        return "96KHz";
    else if (rate == 176400)
        return "176.4KHz";
    else if (rate == 192000)
        return "192KHz";
    else if (rate == 352800)
        return "352.8KHz";
    else if (rate == 384000)
        return "384KHz";
    else if (rate == 705600)
        return "705.6KHz";
    else if (rate == 1411200)
        return "1.411MHz";
    else if (rate >= 2822000)
    {
        float fra = rate / 1000000.0f;
        return std::format("{:.3f}MHz", fra);
    }
    else
        return std::format("{}Hz", rate);
}

std::string AudioFormatBrifStr(const AudioFormat* fmt)
{
    std::string strSampleRate = SampleRateBrifStr(fmt->sampleRate);
    std::string strChannels = ChannelBrifName(fmt->numChannels, fmt->chLayout);
    std::string strFormat = PcmDescriptionFromFormat(fmt->format);

    return std::format("{}, {}, {} channels", strFormat, strSampleRate, strChannels);
}

/*
1	Mono
2	FL, FR
3	FL, FR, FC
4	FL, FR, SL, SR
5	FL, FR, FC, SL, SR
6	FL, FR, FC, LFE, SL, SR 
8	FL, FR, FC, LFE, BL, BR, SL, SR
*/
//根据工程经验
uint32_t StandLayoutByChannelsCount(uint32_t channels)
{
    uint32_t layout = 0;
    switch (channels)
    {
    case 1: 
        layout = CHANNEL_FRONT_CENTER;
        break;
    case 2:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT;
        break;
    case 3:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_LOW_FREQUENCY; // CHANNEL_FRONT_CENTER;
        break;
    case 4:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_BACK_LEFT | CHANNEL_BACK_RIGHT;
        break;
    case 5:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_FRONT_CENTER | 
            CHANNEL_BACK_LEFT | CHANNEL_BACK_RIGHT;
        break;
    case 6: // FFmpeg / WAV EXT 默认：5.1 Side
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_FRONT_CENTER |
            CHANNEL_SIDE_LEFT | CHANNEL_SIDE_RIGHT | CHANNEL_LOW_FREQUENCY;
        break;
    case 7:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_FRONT_CENTER |
            CHANNEL_SIDE_LEFT | CHANNEL_SIDE_RIGHT | CHANNEL_BACK_CENTER | CHANNEL_LOW_FREQUENCY;
        break;
    case 8:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_FRONT_CENTER |
            CHANNEL_SIDE_LEFT | CHANNEL_SIDE_RIGHT | CHANNEL_BACK_LEFT | CHANNEL_BACK_RIGHT | CHANNEL_LOW_FREQUENCY;
        break;
    default:
        layout = 0;
        break;
    }

    return layout;
}

/*
* SACD 的通道顺序是固定的：FL, FR, C, SL, SR, (LFE)
2	FL, FR
3	FL, FR, C
4	FL, FR, SL, SR
5	FL, FR, C, SL, SR
6	FL, FR, C, SL, SR, LFE
*/
uint32_t SACDLayoutByChannelsCount(uint32_t channels)
{
    uint32_t layout = 0;
    switch (channels)
    {
    case 2:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT;
        break;
    case 3:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_FRONT_CENTER;
        break;
    case 4:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_SIDE_LEFT | CHANNEL_SIDE_RIGHT;
        break;
    case 5:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_FRONT_CENTER | 
            CHANNEL_SIDE_LEFT | CHANNEL_SIDE_RIGHT;
        break;
    case 6:
        layout = CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT | CHANNEL_FRONT_CENTER |
            CHANNEL_SIDE_LEFT | CHANNEL_SIDE_RIGHT | CHANNEL_LOW_FREQUENCY;
        break;
    default:
        layout = 0;
        break;
    }

    return layout;
}

std::string ChannelNamesFromLayout(uint32_t chLayout)
{
    std::string names;

    if (chLayout & CHANNEL_FRONT_LEFT) names += "FL ";
    if (chLayout & CHANNEL_FRONT_RIGHT) names += "FR ";
    if (chLayout & CHANNEL_FRONT_CENTER) names += "FC ";
    if (chLayout & CHANNEL_LOW_FREQUENCY) names += "LFE ";
    if (chLayout & CHANNEL_BACK_LEFT) names += "BL ";
    if (chLayout & CHANNEL_BACK_RIGHT) names += "BR ";
    if (chLayout & CHANNEL_FRONT_LEFT_OF_CENTER) names += "FLC ";
    if (chLayout & CHANNEL_FRONT_RIGHT_OF_CENTER) names += "FRC ";
    if (chLayout & CHANNEL_BACK_CENTER) names += "BC ";
    if (chLayout & CHANNEL_SIDE_LEFT) names += "SL ";
    if (chLayout & CHANNEL_SIDE_RIGHT) names += "SR ";

    if (chLayout & CHANNEL_TOP_CENTER) names += "TC ";
    if (chLayout & CHANNEL_TOP_FRONT_LEFT) names += "TFL ";
    if (chLayout & CHANNEL_TOP_FRONT_CENTER) names += "TFC ";
    if (chLayout & CHANNEL_TOP_FRONT_RIGHT) names += "TFR ";
    if (chLayout & CHANNEL_TOP_BACK_LEFT) names += "TBL ";
    if (chLayout & CHANNEL_TOP_BACK_CENTER) names += "TBC ";
    if (chLayout & CHANNEL_TOP_BACK_RIGHT) names += "TBR ";

    return names;
}

std::string ChannelBrifName(uint32_t channels, uint32_t chLayout)
{
    std::string chName;

    switch (channels)
    {
    case 1: chName = "1"; break;
    case 2: chName = "2.0"; break;
    case 3: 
        if(chLayout == 0x07)
            chName = "2.1 (Front)"; 
        else
            chName = "2.1 (LFE)";
        break;
    case 4:
        if (chLayout == 0x0F)
            chName = "3.1";
        else if (chLayout == 0x107)
            chName = "4.0 (Surround)";
        else if (chLayout == 0x603)
            chName = "4.0 (Side)";
        else
            chName = "4.0 (Quad)";  //主流 0x33
        break;
    case 5:
        if (chLayout == 0x3B)
            chName = "4.1";
        else if (chLayout == 0x37)
            chName = "5.0 (Back)";
        else
            chName = "5.0 (Side)";
        break;
    case 6:
        if (chLayout == 0x707)
            chName = "6.0";
        else if (chLayout == 0x3F)
            chName = "5.1 (Back)";
        else
            chName = "5.1 (Side)";
        break;
    case 7:
        if (chLayout == 0x637)
            chName = "7.0";
        else
            chName = "6.1 (Side)";
        break;
    case 8:
        if (chLayout == 0xFF)
            chName = "7.1 (Front Wide)";
        else
            chName = "7.1 (Side Back)";  //0x63F 标准布局
        break;
    default: chName = std::to_string(channels); break;
    }

    return chName;
}

AudioDataFormat AudioFormatFromString(const std::string& fmtStr, uint32_t bitsPerSample)
{
    AudioDataFormat audioFmt;
    if (fmtStr == "Signed")
    {
        switch (bitsPerSample)
        {
        case 8: audioFmt = AudioDataFormat::PCM_S8; break;
        case 16: audioFmt = AudioDataFormat::PCM_S16; break;
        case 24: audioFmt = AudioDataFormat::PCM_S24; break;
        case 32: audioFmt = AudioDataFormat::PCM_S32; break;
        default: audioFmt = AudioDataFormat::PCM_S32; break;
        }
    }
    else if (fmtStr == "Unsigned")
    {
        switch (bitsPerSample)
        {
        case 8: audioFmt = AudioDataFormat::PCM_U8; break;
        default: audioFmt = AudioDataFormat::PCM_U8; break;
        }
    }
    else if (fmtStr == "Float")
    {
        if (bitsPerSample == 64)
            audioFmt = AudioDataFormat::Float64;
        else
            audioFmt = AudioDataFormat::Float32;
    }
    else if (fmtStr == "G.711-ULaw")
        audioFmt = AudioDataFormat::Ulaw;
    else if (fmtStr == "G.711-ALaw")
        audioFmt = AudioDataFormat::Alaw;
    else if (fmtStr == "IMA-ADPCM")
        audioFmt = AudioDataFormat::ImaAdpcm;
    else if (fmtStr == "MS-ADPCM")
        audioFmt = AudioDataFormat::MsAdpcm;
    else if (fmtStr == "GSM610")
        audioFmt = AudioDataFormat::Gsm610;
    else if (fmtStr == "G.721-ADPCM")
        audioFmt = AudioDataFormat::G721Adpcm;
    else if (fmtStr == "G.723-24-ADPCM")
        audioFmt = AudioDataFormat::G723Adpcm_24;
    else if (fmtStr == "G.723-40-ADPCM")
        audioFmt = AudioDataFormat::G723Adpcm_40;
    else if (fmtStr == "ALAC-16")
        audioFmt = AudioDataFormat::Alac16;
    else if (fmtStr == "ALAC-20")
        audioFmt = AudioDataFormat::Alac20;
    else if (fmtStr == "ALAC-24")
        audioFmt = AudioDataFormat::Alac24;
    else if (fmtStr == "ALAC-32")
        audioFmt = AudioDataFormat::Alac32;
    else if (fmtStr == "MPEG-LayerIII")
        audioFmt = AudioDataFormat::MpegLayer3;
    else if (fmtStr == "DPCM-8")
        audioFmt = AudioDataFormat::Dpcm_8;
    else if (fmtStr == "DPCM-16")
        audioFmt = AudioDataFormat::Dpcm_16;
    else
        audioFmt = AudioDataFormat::UNKNOWN;

    return audioFmt;
}

AudioDataFormat AudioFormatByBitsPerSample(uint32_t bitsPerSample)
{
    AudioDataFormat audioFmt;

    switch (bitsPerSample)
    {
    case 8: audioFmt = AudioDataFormat::PCM_S8; break;
    case 16: audioFmt = AudioDataFormat::PCM_S16; break;
    case 24: audioFmt = AudioDataFormat::PCM_S24; break;
    case 32: audioFmt = AudioDataFormat::PCM_S32; break;
    default: audioFmt = AudioDataFormat::PCM_S32; break;
    }

    return audioFmt;
}

bool IsSameAudioFormat(const AudioFormat* fmt1, const AudioFormat* fmt2)
{
    assert(fmt1 != nullptr);
    assert(fmt2 != nullptr);

    if (fmt1->format != fmt2->format)
        return false;
    if (fmt1->bitsPerSample != fmt2->bitsPerSample)
        return false;
    if (fmt1->numChannels != fmt2->numChannels)
        return false;
    if (fmt1->sampleRate != fmt2->sampleRate)
        return false;
    if (fmt1->chLayout != fmt2->chLayout)
        return false;
    if (fmt1->blockAlign != fmt2->blockAlign)
        return false;

    return true;
}

void InitAudioFormat(AudioFormat* fmt, AudioDataFormat dataFmt, uint32_t channels, uint32_t sampleRate, uint32_t bitsPerSample, int blockAlign, uint32_t chLayout)
{
    fmt->format = dataFmt;
    fmt->numChannels = channels;
    fmt->sampleRate = sampleRate;
    if(bitsPerSample != 0)
        fmt->bitsPerSample = bitsPerSample;
    else
        fmt->bitsPerSample = GetBitsPerSampleByFormat(dataFmt);

    if (fmt->bitsPerSample == 0)
        fmt->bitsPerSample = 8;

    if(blockAlign == 0)
        fmt->blockAlign = (fmt->bitsPerSample / 8) * fmt->numChannels;
    else
        fmt->blockAlign = blockAlign;

    if (chLayout != 0)
        fmt->chLayout = chLayout;
    else
        fmt->chLayout = StandLayoutByChannelsCount(channels);
}

void InitEmptyAudioFormat(AudioFormat* fmt)
{
    fmt->format = AudioDataFormat::UNKNOWN;
    fmt->numChannels = 0;
    fmt->chLayout = 0;
    fmt->sampleRate = 0;
    fmt->bitsPerSample = 0;
    fmt->blockAlign = 0;
}

const TypeNameFormat* GetTypenameFormat()
{
    return s_TypenameFormat;
}
