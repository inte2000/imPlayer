/*
20260526 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_74.txt

�޸ļ�¼��
��ģ�ͣ�ChatGPT 5.3 Codex
todo_task_75.txt
todo_task_77.txt
*/
#include <algorithm>
#include <format>
#include <string>

#include "UnicodeConvert.h"
#include "MediaTagNames.h"
#include "Mpg123PlayCtrl.h"

std::string StringFromMp3Mode(Mp3Mode mode)
{
    if (mode == Mp3Mode_Stereo) {
        return "Stereo";
    }
    if (mode == Mp3Mode_JointStero) {
        return "Joint Stero";
    }
    if (mode == Mp3Mode_DualChannel) {
        return "Dual Channel";
    }

    return "Mono";
}

std::string BitsRateStringFromMp3Info(const Mp3ExtraInfo& extraInfo)
{
    if ((extraInfo.vbr == Mp3VbrMode_CBR) || (extraInfo.vbr == Mp3VbrMode_VBR) || (extraInfo.vbr == Mp3VbrMode_ABR)) {
        return std::to_string(extraInfo.bitrate) + "kbps";
    }

    return "";
}

Mpg123PlayCtrl::Mpg123PlayCtrl()
    : m_stream(nullptr)
    , m_mp3()
    , m_extraInfo({})
    , m_curFrames(0)
    , m_totalFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
{
    InitEmptyAudioFormat(&m_srcAudioFmt);
}

Mpg123PlayCtrl::~Mpg123PlayCtrl()
{
    Release();
}

bool Mpg123PlayCtrl::Init(CDataStream* stream, uint32_t streamFmt)
{
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;
    if (!m_mp3.OpenStream(stream)) {
        Release();
        return false;
    }

    m_srcAudioFmt.format = m_mp3.GetAudioFormat();
    m_srcAudioFmt.numChannels = m_mp3.GetChannels();
    m_srcAudioFmt.chLayout = StandLayoutByChannelsCount(m_srcAudioFmt.numChannels);
    m_srcAudioFmt.sampleRate = m_mp3.GetSampleRate();
    m_srcAudioFmt.bitsPerSample = GetBitsPerSampleByFormat(m_srcAudioFmt.format);
    m_srcAudioFmt.blockAlign = (m_srcAudioFmt.bitsPerSample / 8) * m_srcAudioFmt.numChannels;

    m_totalFrames = m_mp3.GetTotalFrames();
    m_curFrames = 0;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;

    m_mp3.GetExtraInfo(m_extraInfo);
    DecideBitsRate(m_extraInfo);

    m_tempBufSize = m_srcAudioFmt.sampleRate * m_srcAudioFmt.blockAlign * 2; 
    m_tempBuf = std::make_unique<uint8_t[]>(m_tempBufSize);

    return true;
}

void Mpg123PlayCtrl::Release()
{
    m_mp3.Close();
    m_stream = nullptr;
    InitEmptyAudioFormat(&m_srcAudioFmt);
    m_extraInfo = {};
    m_curFrames = 0;
    m_totalFrames = 0;
    m_streamFmt = StreamFormatUnknown;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
}

bool Mpg123PlayCtrl::OpenStream(uint32_t streamIndex)
{
    if (!m_mp3) {
        return false;
    }
    if ((streamIndex != 0) && (streamIndex != static_cast<uint32_t>(-1))) {
        return false;
    }

    m_curFrames = 0;
    m_totalFrames = m_mp3.GetTotalFrames();
    m_mp3.SeekSamples(0, SEEK_SET);
    m_activeStreamIdx = 0;
    m_opened = true;
    return true;
}

void Mpg123PlayCtrl::StopStream()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool Mpg123PlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
{
    if (audioFmt == nullptr) {
        return false;
    }
    if (audioFmt->sampleRate != m_srcAudioFmt.sampleRate) {
        return false;
    }
    if (audioFmt->numChannels != m_srcAudioFmt.numChannels) {
        return false;
    }

    return (audioFmt->format == AudioDataFormat::PCM_S16)
        || (audioFmt->format == AudioDataFormat::PCM_S32)
        || (audioFmt->format == AudioDataFormat::Float32)
        || (audioFmt->format == AudioDataFormat::Float64);
}

bool Mpg123PlayCtrl::IsCanSeeking() const
{
    if (m_stream == nullptr) {
        return false;
    }

    return (m_stream->GetStyle() & dsStyleSeekable) != 0;
}

uint32_t Mpg123PlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((!m_mp3) || (!m_opened) || (pBuf == nullptr) || (frames == 0) || !IsSupportOutput(audioFmt)) {
        return 0;
    }
    if (m_curFrames >= m_totalFrames) {
        return 0;
    }

    uint32_t bufSize = frames * audioFmt->blockAlign;
    if (m_mp3.GetAudioFormat() == audioFmt->format)
    {
        uint32_t frameSize = m_srcAudioFmt.blockAlign;
        uint32_t readed = m_mp3.Read(pBuf, bufSize);
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
        uint32_t frameSize = m_srcAudioFmt.blockAlign;
        uint32_t needsize = frames * frameSize;
        if (needsize > m_tempBufSize)
            needsize = m_tempBufSize;

        uint32_t readed = m_mp3.Read(m_tempBuf.get(), needsize);
        if (readed == 0)
        {
            return 0;
        }

        uint32_t readFrames = readed / frameSize;
        uint32_t readSamples = readFrames * m_mp3.GetChannels();

        if (bufSize < readFrames * audioFmt->blockAlign)
            return 0;

        bool bSuccess = false;
        if (audioFmt->format == AudioDataFormat::Float32)
            bSuccess = m_converter->ToFloat32(m_tempBuf.get(), m_mp3.GetAudioFormat(), (float *)pBuf, readSamples);
        else if (audioFmt->format == AudioDataFormat::PCM_S16)
            bSuccess = m_converter->ToInt16S(m_tempBuf.get(), m_mp3.GetAudioFormat(), (int16_t *)pBuf, readSamples);
        else if (audioFmt->format == AudioDataFormat::PCM_S32)
            bSuccess = m_converter->ToInt32S(m_tempBuf.get(), m_mp3.GetAudioFormat(), (int32_t *)pBuf, readSamples);
        else if (audioFmt->format == AudioDataFormat::Float64)
            bSuccess = m_converter->ToFloat64(m_tempBuf.get(), m_mp3.GetAudioFormat(), (double *)pBuf, readSamples);
        else
            bSuccess = false;

        if (!bSuccess)
            return 0;

        m_curFrames += readFrames;
        return readFrames;
    }
}

void Mpg123PlayCtrl::Seek(std::size_t frames)
{
    if (!m_mp3) {
        return;
    }

    m_curFrames = (frames <= m_totalFrames) ? frames : m_totalFrames;
    m_mp3.SeekSamples(static_cast<long long>(m_curFrames), SEEK_SET);
}

void Mpg123PlayCtrl::Reset()
{
    m_converter.reset();
}

float Mpg123PlayCtrl::GetDurationSeconds() const
{
    if (m_srcAudioFmt.sampleRate == 0) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(m_totalFrames) / m_srcAudioFmt.sampleRate);
}

void Mpg123PlayCtrl::FillMetaTags(CMediaTag& tags) const
{
    tags.Clear();

    const std::string type = "MP3 Audio";
    const std::string brief = std::format("MP{} {} {} {}",
        m_extraInfo.layer,
        SampleRateBrifStr(m_extraInfo.rate),
        BitsRateStringFromMp3Info(m_extraInfo),
        StringFromMp3Mode(m_extraInfo.mode));

    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagString(MediaTag_Type, LocalMBCSToUtf8(type));
    tags.AddTagString(MediaTag_Brief, LocalMBCSToUtf8(brief));
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());
    tags.AddTagString(MediaTag_Title, m_extraInfo.title);
    tags.AddTagString(MediaTag_Artists, m_extraInfo.artist);
    tags.AddTagString(MediaTag_Album, m_extraInfo.album);
    tags.AddTagString(MediaTag_Genre, m_extraInfo.genre);
    tags.AddTagString(MediaTag_Comment, m_extraInfo.comment);
    tags.AddTagString(MediaTag_Date, m_extraInfo.year);

    tags.AddTagInteger(MediaTag_BitsPerSample, m_srcAudioFmt.bitsPerSample);
    tags.AddTagInteger(MediaTag_SamplesRate, m_srcAudioFmt.sampleRate);
    tags.AddTagInteger(MediaTag_Channels, m_srcAudioFmt.numChannels);
    tags.AddTagInteger(MediaTag_ChannelsLayout, m_srcAudioFmt.chLayout);
    tags.AddTagInteger(MediaTag_BitsRate, m_extraInfo.bitrate * 1000);

    tags.AddTagInteger(MediaTag_Mp3Ver, m_extraInfo.version);
    tags.AddTagInteger(MediaTag_Mp3Layer, m_extraInfo.layer);
    tags.AddTagInteger(MediaTag_Mp3Mode, m_extraInfo.mode);
    tags.AddTagInteger(MediaTag_Mp3FrameSize, m_extraInfo.framesize);
    tags.AddTagInteger(MediaTag_Mp3CopyFlags, m_extraInfo.flags);
    tags.AddTagInteger(MediaTag_Mp3AbrRate, m_extraInfo.abr_rate);
    tags.AddTagInteger(MediaTag_Mp3VbrMode, m_extraInfo.vbr);
}

void Mpg123PlayCtrl::DecideBitsRate(Mp3ExtraInfo& extraInfo)
{
    if (extraInfo.vbr == Mp3VbrMode_CBR) {
        return;
    }
    if ((m_stream == nullptr) || (m_srcAudioFmt.sampleRate == 0)) {
        return;
    }

    const double totalBits = static_cast<double>(m_stream->GetLength()) * 8.0;
    const double duration = static_cast<double>(m_totalFrames) / static_cast<double>(m_srcAudioFmt.sampleRate);
    if (duration > 0.0) {
        extraInfo.bitrate = static_cast<int>((totalBits / duration) / 1000.0);
    }
}
