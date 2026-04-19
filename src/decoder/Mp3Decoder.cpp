#include <fstream>
#include <stdexcept>
#include <format>
#include <cassert>
#include "Mp3Decoder.h"
#include "Mp3Handle.h"
#include "ConvertFormat.h"
#include <nlohmann/json.hpp> 
#include "UnicodeConvert.h"
//#include "Utf8String.h"

using json = nlohmann::json;


static std::string StringFromMp3Mode(Mp3Mode mode)
{
    if (mode == Mp3Mode_Stereo)
        return "Stereo";
    else if (mode == Mp3Mode_JointStero)
        return "Joint Stero";
    else if (mode == Mp3Mode_DualChannel)
        return "Dual Channel";
    else
        return "Mono";
}

static std::string BitsRateStringFromMp3Info(const Mp3ExtraInfo& extraInfo)
{
    if (extraInfo.vbr == Mp3VbrMode_CBR)
        return std::to_string(extraInfo.bitrate) + "kbps";
    if (extraInfo.vbr == Mp3VbrMode_VBR)
        return std::to_string(extraInfo.bitrate) + "kbps";
    if (extraInfo.vbr == Mp3VbrMode_ABR)
        return std::to_string(extraInfo.bitrate) + "kbps";
    else
        return "";
}

std::string Mp3ExtraInfoToJson(const Mp3ExtraInfo& extraInfo, const AudioFormat& audioFmt, double totalSeconds)
{
    std::string brief = std::format("MP{} {} {} {}", 
        extraInfo.layer, SampleRateBrifStr(extraInfo.rate), BitsRateStringFromMp3Info(extraInfo), 
        StringFromMp3Mode(extraInfo.mode));

    std::string type = "MP3 Audio";

    json info = {
        {"type", LocalMBCSToUtf8(type)},
        {"sample_rate", extraInfo.rate },
        {"bits_per_sample", audioFmt.bytesPerSample * 8 },
        {"channels", audioFmt.numChannels },
        {"channels_layout", audioFmt.chLayout },
        {"bitrate", extraInfo.bitrate * 1000},
        {"length", totalSeconds },
        {"version", extraInfo.version},
        {"layer", extraInfo.layer},
        {"mode", extraInfo.mode},
        {"framesize", extraInfo.framesize},
        {"flags", extraInfo.flags},
        {"abr_rate", extraInfo.abr_rate},
        {"vbr", extraInfo.vbr},
        {"title", extraInfo.title },
        {"artist", extraInfo.artist },
        {"album", extraInfo.album },
        {"year", extraInfo.year },
        {"genre", extraInfo.genre },
        {"comment", extraInfo.comment },
        {"brief", LocalMBCSToUtf8(brief)}
    };

    return info.dump(4);
}

CMp3Decoder::CMp3Decoder(uint32_t streamFmt)
{
    m_streamFmt = streamFmt;
    m_curFrames = 0;
    InitEmptyAudioFormat(&m_AudioFmt);
    m_totalFrames = 0;
    m_dataOffset = 0;
    m_name = "Native Mpg123 Decoder";
    m_type = DECODE_TYPE_NATIVE;
}

AudioInfo CMp3Decoder::InitDecode(const CDecodeInitCtx* decodeInit)
{
    assert(m_pStream != nullptr);
    
    AudioInfo audioInfo;
    if (!m_hMp3.OpenStream(m_pStream))
        throw std::runtime_error("Mp3 handle fail to open mp3 data stream!");

    m_AudioFmt.format = m_hMp3.GetAudioFormat();
    m_AudioFmt.numChannels = m_hMp3.GetChannels();
    m_AudioFmt.chLayout = StandLayoutByChannelsCount(m_AudioFmt.numChannels);
    m_AudioFmt.sampleRate = m_hMp3.GetSampleRate();
    m_AudioFmt.bytesPerSample = GetBitsPerSampleByFormat(m_AudioFmt.format) / 8;
    m_AudioFmt.blockAlign = m_AudioFmt.bytesPerSample * m_AudioFmt.numChannels;
   
    audioInfo.m_audioFmt = m_AudioFmt;
    //uint32_t frameSize = audioInfo.m_audioFmt.bytesPerSample * audioInfo.m_audioFmt.numChannels;

    m_totalFrames = m_hMp3.GetTotalFrames();
    audioInfo.m_totalFrames = m_totalFrames;

    Mp3ExtraInfo extraInfo;
    m_hMp3.GetExtraInfo(extraInfo);
    DecideBitsRate(extraInfo);

    audioInfo.extraInfo = Mp3ExtraInfoToJson(extraInfo, m_AudioFmt, double(m_totalFrames) / m_AudioFmt.sampleRate);
    m_tempBufSize = m_AudioFmt.sampleRate * m_AudioFmt.bytesPerSample * m_AudioFmt.numChannels * 2; //准备一个 2s 的buffer
    m_tempBuf = std::make_unique<uint8_t[]>(m_tempBufSize);

    return audioInfo;
}

uint32_t CMp3Decoder::Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    assert(m_pStream != nullptr);

    if (m_curFrames >= m_totalFrames)
        return 0;

    //格式相同就用 pBuf 直出，避免多余的内存拷贝动作
    if (m_hMp3.GetAudioFormat() == audioFmt->format)
    {
        if (bufSize < frames * audioFmt->bytesPerSample * audioFmt->numChannels)
            return 0;

        uint32_t frameSize = m_AudioFmt.bytesPerSample * m_AudioFmt.numChannels;
        uint32_t readed = m_hMp3.Read(pBuf, bufSize);
        if (readed == 0)
        {
            return 0;
        }

        uint32_t readFrames = readed / frameSize;

        m_curFrames += readFrames;
        return readFrames;
    }
    else
    {
        if (!m_converter)
        {
            m_converter = MakeAudioConvert(DitherTypeT::Triangle); 
        }
        uint32_t frameSize = m_AudioFmt.bytesPerSample * m_AudioFmt.numChannels;
        uint32_t needsize = frames * frameSize;
        if (needsize > m_tempBufSize)
            needsize = m_tempBufSize;

        uint32_t readed = m_hMp3.Read(m_tempBuf.get(), needsize);
        if (readed == 0)
        {
            return 0;
        }

        uint32_t readFrames = readed / frameSize;
        uint32_t readSamples = readFrames * m_hMp3.GetChannels();

        if (bufSize < readFrames * audioFmt->bytesPerSample * audioFmt->numChannels)
            return 0;

        bool bSuccess = false;
        if (audioFmt->format == AudioDataFormat::Float32)
            bSuccess = m_converter->ToFloat32(m_tempBuf.get(), m_hMp3.GetAudioFormat(), (float *)pBuf, readSamples);
        else if (audioFmt->format == AudioDataFormat::PCM_S16)
            bSuccess = m_converter->ToInt16S(m_tempBuf.get(), m_hMp3.GetAudioFormat(), (int16_t *)pBuf, readSamples);
        else if (audioFmt->format == AudioDataFormat::PCM_S32)
            bSuccess = m_converter->ToInt32S(m_tempBuf.get(), m_hMp3.GetAudioFormat(), (int32_t *)pBuf, readSamples);
        else if (audioFmt->format == AudioDataFormat::Float64)
            bSuccess = m_converter->ToFloat64(m_tempBuf.get(), m_hMp3.GetAudioFormat(), (double *)pBuf, readSamples);
        else
            bSuccess = false;

        if (!bSuccess)
            return 0;

        m_curFrames += readFrames;
        return readFrames;
    }
}

void CMp3Decoder::SeekTo(std::size_t frames)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    assert(m_pStream != nullptr);

    m_curFrames = (frames <= m_totalFrames) ? frames : m_totalFrames;
    m_hMp3.SeekSamples(m_curFrames, SEEK_SET);
}

bool CMp3Decoder::IsSupportOutput(const AudioFormat* audioFmt) const
{
    //支持 PCM_S16、PCM_S32、Float32 和 Float64 输出格式
    if ((audioFmt->format != AudioDataFormat::PCM_S16) && (audioFmt->format != AudioDataFormat::PCM_S32)
        && (audioFmt->format != AudioDataFormat::Float32) && (audioFmt->format != AudioDataFormat::Float64))
    {
        return false;
    }

    //目前不支持采样率转换输出、声道转换
    if (audioFmt->sampleRate != m_AudioFmt.sampleRate)
        return false;
    if (audioFmt->numChannels != m_AudioFmt.numChannels)
        return false;

    return true;
}

void CMp3Decoder::DecideBitsRate(Mp3ExtraInfo& ei)
{
    if (ei.vbr == Mp3VbrMode_CBR) //恒定 bitrate
        return; 

    double totalBits = m_pStream->GetLength() * 8.0;
    double duration = static_cast<double>(m_hMp3.GetTotalFrames()) / m_hMp3.GetSampleRate();
    ei.bitrate = int((totalBits / duration) / 1000.0);
}

