/*
20260424 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_15.txt

20260424 修改 inline，引入 core.lib
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_16.txt
*/
#include <algorithm>
#include <limits>
#include <format>
#include <string>
#include "MediaTagNames.h"
#include "UnicodeConvert.h"
#include "LibsndFunc.h"
#include "LibSndPlayCtrl.h"

//Human action
static std::string AdaptiveSfString(const char* sfString)
{
    std::string result;

    if (sfString != nullptr)
    {
        result = AdaptiveToUtf8(sfString);
    }

    return result;
}

LibSndPlayCtrl::LibSndPlayCtrl()
    : m_stream(nullptr)
    , m_file(nullptr)
    , m_sfInfo({})
    , m_vio({})
    , m_srcAudioFmt({})
    , m_totalFrames(0)
    , m_curFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
{
    InitEmptyAudioFormat(&m_srcAudioFmt);
}

LibSndPlayCtrl::~LibSndPlayCtrl()
{
    Release();
}

bool LibSndPlayCtrl::Init(CDataStream* stream, uint32_t streamFmt)
{
    Release();
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;

    m_vio.get_filelen = GetLengthCb;
    m_vio.seek = SeekCb;
    m_vio.read = ReadCb;
    m_vio.write = WriteCb;
    m_vio.tell = TellCb;

    m_file = sf_open_virtual(&m_vio, SFM_READ, &m_sfInfo, m_stream);
    if (m_file == nullptr) {
        Release();
        return false;
    }

    if (m_streamFmt == StreamFormatUnknown) {
        m_streamFmt = StreamFormatFromLibsndfileFormat(m_sfInfo.format);
    }

    m_srcAudioFmt.format = SndfileTransSubType(m_sfInfo.format & SF_FORMAT_SUBMASK);
    m_srcAudioFmt.numChannels = static_cast<uint32_t>(std::max(1, m_sfInfo.channels));
    m_srcAudioFmt.chLayout = StandLayoutByChannelsCount(m_srcAudioFmt.numChannels);
    m_srcAudioFmt.sampleRate = static_cast<uint32_t>(std::max(1, m_sfInfo.samplerate));
    m_srcAudioFmt.bitsPerSample = BitsPerSampleFromSndSubtype(m_sfInfo.format);
    if (m_srcAudioFmt.bitsPerSample == 0) {
        m_srcAudioFmt.bitsPerSample = GetBitsPerSampleByFormat(m_srcAudioFmt.format);
    }
    if (m_srcAudioFmt.bitsPerSample == 0) {
        m_srcAudioFmt.bitsPerSample = 16;
    }
    m_srcAudioFmt.blockAlign = (m_srcAudioFmt.bitsPerSample / 8) * m_srcAudioFmt.numChannels;

    if (m_sfInfo.frames > 0) {
        m_totalFrames = static_cast<std::size_t>(m_sfInfo.frames);
    }
    else {
        m_totalFrames = 0;
    }

    m_curFrames = 0;
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    return true;
}

void LibSndPlayCtrl::Release()
{
    if (m_file != nullptr) {
        sf_close(m_file);
        m_file = nullptr;
    }

    m_stream = nullptr;
    m_sfInfo = {};
    m_vio = {};
    InitEmptyAudioFormat(&m_srcAudioFmt);
    m_totalFrames = 0;
    m_curFrames = 0;
    m_streamFmt = StreamFormatUnknown;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
}

bool LibSndPlayCtrl::OpenStream(uint32_t streamIndex)
{
    if (m_file == nullptr) {
        return false;
    }
    if ((streamIndex != 0) && (streamIndex != static_cast<uint32_t>(-1))) {
        return false;
    }

    sf_count_t pos = sf_seek(m_file, 0, SEEK_SET);
    if (pos < 0) {
        return false;
    }

    m_curFrames = 0;
    m_opened = true;
    m_activeStreamIdx = 0;
    return true;
}

void LibSndPlayCtrl::StopStream()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool LibSndPlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
{
    if ((m_file == nullptr) || (audioFmt == nullptr)) {
        return false;
    }
    if (audioFmt->sampleRate != m_srcAudioFmt.sampleRate) {
        return false;
    }
    if (audioFmt->numChannels != m_srcAudioFmt.numChannels) {
        return false;
    }

    switch (audioFmt->format) {
    case AudioDataFormat::PCM_S16:
    case AudioDataFormat::PCM_S32:
    case AudioDataFormat::Float32:
    case AudioDataFormat::Float64:
        return true;
    default:
        return false;
    }
}

bool LibSndPlayCtrl::IsCanSeeking() const
{
    if (m_stream == nullptr) {
        return false;
    }
    if ((m_stream->GetType() & dsTypeSeekable) == 0) {
        return false;
    }

    return (m_sfInfo.seekable != 0);
}

uint32_t LibSndPlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((m_file == nullptr) || (!m_opened) || (pBuf == nullptr) || (frames == 0) || !IsSupportOutput(audioFmt)) {
        return 0;
    }

    sf_count_t readFrames = 0;
    switch (audioFmt->format) {
    case AudioDataFormat::PCM_S16:
        readFrames = sf_readf_short(m_file, static_cast<short*>(pBuf), static_cast<sf_count_t>(frames));
        break;
    case AudioDataFormat::PCM_S32:
        readFrames = sf_readf_int(m_file, static_cast<int*>(pBuf), static_cast<sf_count_t>(frames));
        break;
    case AudioDataFormat::Float32:
        readFrames = sf_readf_float(m_file, static_cast<float*>(pBuf), static_cast<sf_count_t>(frames));
        break;
    case AudioDataFormat::Float64:
        readFrames = sf_readf_double(m_file, static_cast<double*>(pBuf), static_cast<sf_count_t>(frames));
        break;
    default:
        readFrames = 0;
        break;
    }

    if (readFrames < 0) {
        return 0;
    }

    m_curFrames += static_cast<std::size_t>(readFrames);
    if (m_totalFrames > 0) {
        m_curFrames = std::min(m_curFrames, m_totalFrames);
    }

    return static_cast<uint32_t>(readFrames);
}

void LibSndPlayCtrl::Seek(std::size_t frames)
{
    if ((m_file == nullptr) || !IsCanSeeking()) {
        return;
    }

    sf_count_t target = static_cast<sf_count_t>(frames);
    if (m_totalFrames > 0) {
        target = static_cast<sf_count_t>(std::min(frames, m_totalFrames));
    }

    sf_count_t pos = sf_seek(m_file, target, SEEK_SET);
    if (pos >= 0) {
        m_curFrames = static_cast<std::size_t>(pos);
    }
}

float LibSndPlayCtrl::GetDurationSeconds() const
{
    if (m_srcAudioFmt.sampleRate == 0) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(m_totalFrames) / m_srcAudioFmt.sampleRate);
}

void LibSndPlayCtrl::FillMetaTags(CMediaTag& tags) const
{
    tags.Clear();

    int NativeType = m_sfInfo.format & SF_FORMAT_TYPEMASK;
    std::string type, brief;
    if ((m_srcAudioFmt.format == AudioDataFormat::MpegLayer3)
        || (m_srcAudioFmt.format == AudioDataFormat::Vorbis)
        || (m_srcAudioFmt.format == AudioDataFormat::Opus))
    {
        std::string strtype = SndfileGetFileTypeName(NativeType);
        std::string vodename = StringFromAudioFormat(m_srcAudioFmt.format);
        type = std::format("{}-{}", strtype, vodename);
        brief = std::format("{}: {} {} channels",
            type, SampleRateBrifStr(m_srcAudioFmt.sampleRate),  ChannelBrifName(m_srcAudioFmt.numChannels, m_srcAudioFmt.chLayout));
    }
    else
    {
        type = SndfileGetFileTypeName(NativeType).data();
        brief = std::format("{}: {} {} {} channels",
            type, SampleRateBrifStr(m_srcAudioFmt.sampleRate), BitsPerSampleStringFromAudioFormat(m_srcAudioFmt.format), ChannelBrifName(m_srcAudioFmt.numChannels, m_srcAudioFmt.chLayout));
    }

    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());

    const char* pTitle = sf_get_string(m_file, SF_STR_TITLE);
    const char* pArtist = sf_get_string(m_file, SF_STR_ARTIST);
    const char* pGener = sf_get_string(m_file, SF_STR_GENRE);
    const char* pAlbum = sf_get_string(m_file, SF_STR_ALBUM);
    const char* pDate = sf_get_string(m_file, SF_STR_DATE);
    const char* pTracks = sf_get_string(m_file, SF_STR_TRACKNUMBER);
    const char* pComment = sf_get_string(m_file, SF_STR_COMMENT);

    tags.AddTagString(MediaTag_Title, AdaptiveSfString(pTitle));
    tags.AddTagString(MediaTag_Artists, AdaptiveSfString(pArtist));
    tags.AddTagString(MediaTag_Album, AdaptiveSfString(pAlbum));
    tags.AddTagString(MediaTag_Genre, AdaptiveSfString(pGener));
    tags.AddTagString(MediaTag_Comment, AdaptiveSfString(pComment));
    tags.AddTagString(MediaTag_Date, AdaptiveSfString(pDate));
    if(pTracks && (strlen(pTracks) > 0))
        tags.AddTagInteger(MediaTag_Tracks, std::stoi(pTracks));

    tags.AddTagInteger(MediaTag_SamplesRate, static_cast<int32_t>(m_srcAudioFmt.sampleRate));
    tags.AddTagInteger(MediaTag_Channels, static_cast<int32_t>(m_srcAudioFmt.numChannels));
    tags.AddTagInteger(MediaTag_ChannelsLayout, static_cast<int32_t>(m_srcAudioFmt.chLayout));
    tags.AddTagInteger(MediaTag_BitsPerSample, static_cast<int32_t>(m_srcAudioFmt.bitsPerSample));
    tags.AddTagString(MediaTag_PcmFormat, StringFromAudioFormat(m_srcAudioFmt.format));
    uint32_t bitrates = m_srcAudioFmt.bitsPerSample * m_srcAudioFmt.sampleRate * m_srcAudioFmt.numChannels; // float32 x 2 channels
    tags.AddTagInteger(MediaTag_BitsRate, bitrates);
}

sf_count_t LibSndPlayCtrl::GetLengthCb(void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    if (stream == nullptr) {
        return 0;
    }

    return static_cast<sf_count_t>(stream->GetLength());
}

sf_count_t LibSndPlayCtrl::SeekCb(sf_count_t offset, int whence, void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    if (stream == nullptr) {
        return -1;
    }

    SeekBase base;
    switch (whence) {
    case SEEK_SET: base = SeekBase::Begin; break;
    case SEEK_CUR: base = SeekBase::Cur; break;
    case SEEK_END: base = SeekBase::End; break;
    default: return -1;
    }

    try {
        stream->Seek(base, offset);
        return static_cast<sf_count_t>(stream->Tell());
    }
    catch (...) {
        return -1;
    }
}

sf_count_t LibSndPlayCtrl::ReadCb(void* ptr, sf_count_t count, void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    if ((stream == nullptr) || (ptr == nullptr) || (count <= 0)) {
        return 0;
    }

    uint32_t readed = stream->Read(ptr, (uint32_t)count);

    return readed;
}

sf_count_t LibSndPlayCtrl::WriteCb(const void* ptr, sf_count_t count, void* userData)
{
    (void)ptr;
    (void)count;
    (void)userData;
    return 0;
}

sf_count_t LibSndPlayCtrl::TellCb(void* userData)
{
    CDataStream* stream = static_cast<CDataStream*>(userData);
    if (stream == nullptr) {
        return 0;
    }

    return static_cast<sf_count_t>(stream->Tell());
}

uint32_t LibSndPlayCtrl::BitsPerSampleFromSndSubtype(int format)
{
    const int subtype = (format & SF_FORMAT_SUBMASK);
    switch (subtype) {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_ULAW:
    case SF_FORMAT_ALAW:
        return 8;
    case SF_FORMAT_PCM_16:
        return 16;
    case SF_FORMAT_PCM_24:
        return 24;
    case SF_FORMAT_PCM_32:
    case SF_FORMAT_FLOAT:
        return 32;
    case SF_FORMAT_DOUBLE:
        return 64;
    default:
        return 0;
    }
}
