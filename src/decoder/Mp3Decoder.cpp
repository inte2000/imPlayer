#include <fstream>
#include <stdexcept>
#include <format>
#include <cassert>
#include "Mp3Decoder.h"
#include "Mp3Handle.h"
#include "ConvertFormat.h"
#include "UnicodeConvert.h"
//#include "Utf8String.h"


static std::string StringFromMp3Version(Mp3Version ver)
{
    if (ver == Mp3Ver_V2_5)
        return "MPEG 2.5";
    else if (ver == Mp3Ver_V2)
        return "MPEG 2";
    else
        return "MPEG 1";
}

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

static const char* sDecoderName = "Mpg123 Decoder";

std::string CMp3Decoder::Name()
{
    return sDecoderName;
}

CMp3Decoder::CMp3Decoder(uint32_t streamFmt)
{
    m_streamFmt = streamFmt;
    m_curFrames = 0;
    InitEmptyAudioFormat(&m_AudioFmt);
    m_totalFrames = 0;
    m_dataOffset = 0;
    m_name = sDecoderName;
    m_type = DECODE_TYPE_NATIVE;
}

bool CMp3Decoder::InitDecode(const CDecodeInitCtx* decodeInit)
{
    assert(m_pStream != nullptr);

    if (!m_hMp3.OpenStream(m_pStream))
        throw std::runtime_error("Mp3 handle fail to open mp3 data stream!");

    m_AudioFmt.format = m_hMp3.GetAudioFormat();
    m_AudioFmt.numChannels = m_hMp3.GetChannels();
    m_AudioFmt.chLayout = StandLayoutByChannelsCount(m_AudioFmt.numChannels);
    m_AudioFmt.sampleRate = m_hMp3.GetSampleRate();
    m_AudioFmt.bitsPerSample = GetBitsPerSampleByFormat(m_AudioFmt.format);
    m_AudioFmt.blockAlign = (m_AudioFmt.bitsPerSample / 8) * m_AudioFmt.numChannels;
   
    //uint32_t frameSize = audioInfo.m_audioFmt.bytesPerSample * audioInfo.m_audioFmt.numChannels;

    m_totalFrames = m_hMp3.GetTotalFrames();

    m_curStreamIdx = 0;
    m_StreamCount = 1;
    m_hMp3.GetExtraInfo(m_extraInfo);
    DecideBitsRate(m_extraInfo);

    m_tempBufSize = m_AudioFmt.sampleRate * m_AudioFmt.blockAlign * 2; 
    m_tempBuf = std::make_unique<uint8_t[]>(m_tempBufSize);

    return true;
}

bool CMp3Decoder::StartStream(uint32_t streamIdx, std::size_t begin, std::size_t end, uint32_t loop)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (!m_hMp3)
        return false;

    m_curStreamIdx = streamIdx;

    m_curFrames = 0;
    m_totalFrames = m_hMp3.GetTotalFrames();

    if (begin > 0)
    {
        m_curFrames = (begin <= m_totalFrames) ? begin : m_totalFrames;
    }
    if (end < m_totalFrames)
    {
        m_totalFrames = end;
    }

    m_hMp3.SeekSamples(m_curFrames, SEEK_SET);

    return true;
}

void CMp3Decoder::StopStream(uint32_t streamIdx)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);
}

CMediaTag CMp3Decoder::GetTags(uint32_t streamIdx)
{
    CMediaTag tags;
    MakeMediaTags(tags);

    return tags;
}

uint32_t CMp3Decoder::Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    assert(m_pStream != nullptr);

    if (m_curFrames >= m_totalFrames)
        return 0;

    if (m_hMp3.GetAudioFormat() == audioFmt->format)
    {
        if (bufSize < frames * audioFmt->blockAlign)
            return 0;

        uint32_t frameSize = m_AudioFmt.blockAlign;
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
        uint32_t frameSize = m_AudioFmt.blockAlign;
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

        if (bufSize < readFrames * audioFmt->blockAlign)
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
    if ((audioFmt->format != AudioDataFormat::PCM_S16) && (audioFmt->format != AudioDataFormat::PCM_S32)
        && (audioFmt->format != AudioDataFormat::Float32) && (audioFmt->format != AudioDataFormat::Float64))
    {
        return false;
    }

    if (audioFmt->sampleRate != m_AudioFmt.sampleRate)
        return false;
    if (audioFmt->numChannels != m_AudioFmt.numChannels)
        return false;

    return true;
}

bool CMp3Decoder::IsCanSeeking(uint32_t streamIdx) const
{
    if ((m_pStream->GetType() & dsTypeSeekable) == 0)
        return false;

    return true;
}

void CMp3Decoder::Reset()
{
    m_converter.reset();
}

void CMp3Decoder::DecideBitsRate(Mp3ExtraInfo& ei)
{
    if (ei.vbr == Mp3VbrMode_CBR)
        return; 

    double totalBits = m_pStream->GetLength() * 8.0;
    double duration = static_cast<double>(m_hMp3.GetTotalFrames()) / m_hMp3.GetSampleRate();
    ei.bitrate = int((totalBits / duration) / 1000.0);
}

void CMp3Decoder::MakeMediaTags(CMediaTag& tags)
{
    tags.Clear();

    std::string type = "MP3 Audio";
    std::string brief = std::format("MP{} {} {} {}",
        m_extraInfo.layer, SampleRateBrifStr(m_extraInfo.rate), BitsRateStringFromMp3Info(m_extraInfo),
        StringFromMp3Mode(m_extraInfo.mode));

    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagString(MediaTag_Type, LocalMBCSToUtf8(type));
    tags.AddTagString(MediaTag_Brief, LocalMBCSToUtf8(brief));
    double totalSeconds = double(m_totalFrames) / m_AudioFmt.sampleRate;
    tags.AddTagDecimal(MediaTag_Duration, totalSeconds);
    tags.AddTagString(MediaTag_Title, m_extraInfo.title);
    tags.AddTagString(MediaTag_Artists, m_extraInfo.artist);
    tags.AddTagString(MediaTag_Album, m_extraInfo.album);
    tags.AddTagString(MediaTag_Genre, m_extraInfo.genre);
    tags.AddTagString(MediaTag_Comment, m_extraInfo.comment);
    tags.AddTagString(MediaTag_Date, m_extraInfo.year);
    //tags.AddTagInteger(MediaTag_Tracks, 0);

    tags.AddTagInteger(MediaTag_BitsPerSample, m_AudioFmt.bitsPerSample);
    tags.AddTagInteger(MediaTag_SamplesRate, m_AudioFmt.sampleRate);
    tags.AddTagInteger(MediaTag_Channels, m_AudioFmt.numChannels);
    tags.AddTagInteger(MediaTag_ChannelsLayout, m_AudioFmt.chLayout);
    tags.AddTagInteger(MediaTag_BitsRate, m_extraInfo.bitrate * 1000);

    tags.AddTagInteger(MediaTag_Mp3Ver, m_extraInfo.version);
    tags.AddTagInteger(MediaTag_Mp3Layer, m_extraInfo.layer);
    tags.AddTagInteger(MediaTag_Mp3Mode, m_extraInfo.mode);
    tags.AddTagInteger(MediaTag_Mp3FrameSize, m_extraInfo.framesize);
    tags.AddTagInteger(MediaTag_Mp3CopyFlags, m_extraInfo.flags);
    tags.AddTagInteger(MediaTag_Mp3AbrRate, m_extraInfo.abr_rate);
    tags.AddTagInteger(MediaTag_Mp3VbrMode, m_extraInfo.vbr);
}

uint32_t Mpg123QueryFileType(const std::wstring& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return StreamFormatUnknown;

    unsigned char header[10];
    file.read((char*)header, 10);
    uint32_t readcount = static_cast<uint32_t>(file.gcount());
    if (readcount < sizeof(header))
        return StreamFormatUnknown;

    if (memcmp(header, "ID3", 3) == 0)
        return StreamFormatMp3;

    if ((header[0] == 0xFF) && ((header[1] & 0xE0) == 0xE0))
        return StreamFormatMp3;

    return StreamFormatUnknown;
}
