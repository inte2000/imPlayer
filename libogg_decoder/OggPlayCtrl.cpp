/*
20260526 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_81.txt
*/
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <format>
#include <limits>

#include "MediaTagNames.h"
#include "OggPlayCtrl.h"

namespace {

constexpr uint32_t OGG_DECODE_CHUNK_FRAMES = 2048;

std::string ToUpperAscii(const std::string& text)
{
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

int32_t S16ToS32(int16_t sample)
{
    return static_cast<int32_t>(sample) << 16;
}

float S16ToF32(int16_t sample)
{
    return static_cast<float>(sample) / 32768.0f;
}

} // namespace

OggPlayCtrl::OggPlayCtrl()
    : m_stream(nullptr)
    , m_vf({})
    , m_inited(false)
    , m_srcAudioFmt({})
    , m_curFrames(0)
    , m_totalFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
    , m_tempS16()
    , m_tagTitle()
    , m_tagArtist()
    , m_tagAlbum()
    , m_tagGenre()
    , m_tagComment()
    , m_tagDate()
{
    InitEmptyAudioFormat(&m_srcAudioFmt);
}

OggPlayCtrl::~OggPlayCtrl()
{
    Release();
}

bool OggPlayCtrl::Init(CDataStream* stream, uint32_t streamFmt)
{
    Release();
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;

    ov_callbacks callbacks = {};
    callbacks.read_func = ReadFunc;
    callbacks.close_func = CloseFunc;
    if ((m_stream->GetType() & dsTypeSeekable) != 0) {
        callbacks.seek_func = SeekFunc;
        callbacks.tell_func = TellFunc;
    }

    if (ov_open_callbacks(m_stream, &m_vf, nullptr, 0, callbacks) != 0) {
        Release();
        return false;
    }
    m_inited = true;

    vorbis_info* vi = ov_info(&m_vf, -1);
    if ((vi == nullptr) || (vi->channels <= 0) || (vi->rate <= 0)) {
        Release();
        return false;
    }

    InitAudioFormat(&m_srcAudioFmt, AudioDataFormat::PCM_S16, static_cast<uint32_t>(vi->channels), static_cast<uint32_t>(vi->rate), 16);

    const ogg_int64_t total = ov_pcm_total(&m_vf, -1);
    m_totalFrames = (total > 0) ? static_cast<std::size_t>(total) : 0;
    m_curFrames = 0;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;

    ParseComments();
    return true;
}

void OggPlayCtrl::Release()
{
    if (m_inited) {
        ov_clear(&m_vf);
        m_inited = false;
    }

    m_stream = nullptr;
    m_vf = {};
    InitEmptyAudioFormat(&m_srcAudioFmt);
    m_curFrames = 0;
    m_totalFrames = 0;
    m_streamFmt = StreamFormatUnknown;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    m_tempS16.clear();
    m_tagTitle.clear();
    m_tagArtist.clear();
    m_tagAlbum.clear();
    m_tagGenre.clear();
    m_tagComment.clear();
    m_tagDate.clear();
}

bool OggPlayCtrl::OpenStream(uint32_t streamIndex)
{
    if (!m_inited) {
        return false;
    }
    if ((streamIndex != 0) && (streamIndex != static_cast<uint32_t>(-1))) {
        return false;
    }

    if (ov_pcm_seek(&m_vf, 0) != 0) {
        return false;
    }

    m_curFrames = 0;
    m_activeStreamIdx = 0;
    m_opened = true;
    return true;
}

void OggPlayCtrl::StopStream()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool OggPlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
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
        || (audioFmt->format == AudioDataFormat::Float32);
}

bool OggPlayCtrl::IsCanSeeking() const
{
    if (!m_inited) {
        return false;
    }

    return ov_seekable(const_cast<OggVorbis_File*>(&m_vf)) != 0;
}

uint32_t OggPlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((!m_inited) || (!m_opened) || (pBuf == nullptr) || (frames == 0) || !IsSupportOutput(audioFmt)) {
        return 0;
    }

    uint32_t remainFrames = frames;
    if (m_totalFrames > 0) {
        remainFrames = static_cast<uint32_t>(std::min<std::size_t>(remainFrames, m_totalFrames - std::min(m_curFrames, m_totalFrames)));
    }

    const uint32_t channels = m_srcAudioFmt.numChannels;
    uint32_t readFrames = 0;
    uint8_t* outBytes = static_cast<uint8_t*>(pBuf);

    while (remainFrames > 0)
    {
        const uint32_t chunkFrames = std::min(remainFrames, OGG_DECODE_CHUNK_FRAMES);
        const int chunkBytes = static_cast<int>(chunkFrames * channels * sizeof(int16_t));

        int bitstream = 0;
        long bytesRead = 0;
        if (audioFmt->format == AudioDataFormat::PCM_S16)
        {
            bytesRead = ov_read(&m_vf, reinterpret_cast<char*>(outBytes), chunkBytes, 0, 2, 1, &bitstream);
            if (bytesRead <= 0) {
                break;
            }
            outBytes += bytesRead;
        }
        else
        {
            m_tempS16.resize(static_cast<std::size_t>(chunkFrames) * channels);
            bytesRead = ov_read(&m_vf, reinterpret_cast<char*>(m_tempS16.data()), chunkBytes, 0, 2, 1, &bitstream);
            if (bytesRead <= 0) {
                break;
            }

            const uint32_t gotFrames = static_cast<uint32_t>(bytesRead / static_cast<long>(channels * sizeof(int16_t)));
            const uint32_t gotSamples = gotFrames * channels;
            if (audioFmt->format == AudioDataFormat::PCM_S32)
            {
                int32_t* out = reinterpret_cast<int32_t*>(outBytes);
                for (uint32_t i = 0; i < gotSamples; ++i)
                {
                    out[i] = S16ToS32(m_tempS16[i]);
                }
                outBytes += static_cast<std::size_t>(gotSamples) * sizeof(int32_t);
            }
            else
            {
                float* out = reinterpret_cast<float*>(outBytes);
                for (uint32_t i = 0; i < gotSamples; ++i)
                {
                    out[i] = S16ToF32(m_tempS16[i]);
                }
                outBytes += static_cast<std::size_t>(gotSamples) * sizeof(float);
            }
        }

        const uint32_t gotFrames = static_cast<uint32_t>(bytesRead / static_cast<long>(channels * sizeof(int16_t)));
        if (gotFrames == 0) {
            break;
        }
        readFrames += gotFrames;
        remainFrames -= gotFrames;
    }

    m_curFrames += readFrames;
    const ogg_int64_t pos = ov_pcm_tell(&m_vf);
    if (pos >= 0) {
        m_curFrames = static_cast<std::size_t>(pos);
    }

    return readFrames;
}

void OggPlayCtrl::Seek(std::size_t frames)
{
    if (!m_inited) {
        return;
    }

    std::size_t target = frames;
    if (m_totalFrames > 0) {
        target = std::min(target, m_totalFrames);
    }
    if (ov_pcm_seek(&m_vf, static_cast<ogg_int64_t>(target)) == 0) {
        m_curFrames = target;
    }
}

float OggPlayCtrl::GetDurationSeconds() const
{
    if ((m_srcAudioFmt.sampleRate == 0) || (m_totalFrames == 0)) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(m_totalFrames) / static_cast<double>(m_srcAudioFmt.sampleRate));
}

void OggPlayCtrl::FillMetaTags(CMediaTag& tags) const
{
    tags.Clear();

    const std::string type = "Ogg Vorbis";
    const std::string brief = std::format("{}: {}", type, AudioFormatBrifStr(&m_srcAudioFmt));

    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());

    tags.AddTagInteger(MediaTag_BitsPerSample, m_srcAudioFmt.bitsPerSample);
    tags.AddTagInteger(MediaTag_SamplesRate, m_srcAudioFmt.sampleRate);
    tags.AddTagInteger(MediaTag_Channels, m_srcAudioFmt.numChannels);
    tags.AddTagInteger(MediaTag_ChannelsLayout, m_srcAudioFmt.chLayout);
    tags.AddTagInteger(MediaTag_BitsRate, m_srcAudioFmt.bitsPerSample * m_srcAudioFmt.sampleRate * m_srcAudioFmt.numChannels);

    if (!m_tagTitle.empty()) {
        tags.AddTagString(MediaTag_Title, m_tagTitle);
    }
    if (!m_tagArtist.empty()) {
        tags.AddTagString(MediaTag_Artists, m_tagArtist);
    }
    if (!m_tagAlbum.empty()) {
        tags.AddTagString(MediaTag_Album, m_tagAlbum);
    }
    if (!m_tagGenre.empty()) {
        tags.AddTagString(MediaTag_Genre, m_tagGenre);
    }
    if (!m_tagComment.empty()) {
        tags.AddTagString(MediaTag_Comment, m_tagComment);
    }
    if (!m_tagDate.empty()) {
        tags.AddTagString(MediaTag_Date, m_tagDate);
    }
}

size_t OggPlayCtrl::ReadFunc(void* ptr, size_t size, size_t nmemb, void* datasource)
{
    CDataStream* stream = static_cast<CDataStream*>(datasource);
    if ((stream == nullptr) || (ptr == nullptr) || (size == 0) || (nmemb == 0)) {
        return 0;
    }

    const uint64_t bytesToRead64 = static_cast<uint64_t>(size) * static_cast<uint64_t>(nmemb);
    const uint32_t bytesToRead = (bytesToRead64 > UINT32_MAX) ? UINT32_MAX : static_cast<uint32_t>(bytesToRead64);
    const uint32_t bytesRead = stream->Read(ptr, bytesToRead);
    return bytesRead / size;
}

int OggPlayCtrl::SeekFunc(void* datasource, ogg_int64_t offset, int whence)
{
    CDataStream* stream = static_cast<CDataStream*>(datasource);
    if (stream == nullptr) {
        return -1;
    }

    SeekBase base = SeekBase::Begin;
    if (whence == SEEK_CUR) {
        base = SeekBase::Cur;
    }
    else if (whence == SEEK_END) {
        base = SeekBase::End;
    }

    try {
        stream->Seek(base, static_cast<long long>(offset));
    }
    catch (...) {
        return -1;
    }
    return 0;
}

int OggPlayCtrl::CloseFunc(void* datasource)
{
    (void)datasource;
    return 0;
}

long OggPlayCtrl::TellFunc(void* datasource)
{
    CDataStream* stream = static_cast<CDataStream*>(datasource);
    if (stream == nullptr) {
        return -1;
    }

    try {
        const std::size_t pos = stream->Tell();
        if (pos > static_cast<std::size_t>(std::numeric_limits<long>::max())) {
            return std::numeric_limits<long>::max();
        }
        return static_cast<long>(pos);
    }
    catch (...) {
        return -1;
    }
}

void OggPlayCtrl::ParseComments()
{
    vorbis_comment* vc = ov_comment(&m_vf, -1);
    if (vc == nullptr) {
        return;
    }

    for (int i = 0; i < vc->comments; ++i)
    {
        const char* c = vc->user_comments[i];
        if (c == nullptr) {
            continue;
        }

        const std::string kv(c);
        const std::size_t pos = kv.find('=');
        if ((pos == std::string::npos) || (pos == 0)) {
            continue;
        }

        const std::string key = ToUpperAscii(kv.substr(0, pos));
        const std::string value = kv.substr(pos + 1);
        if (value.empty()) {
            continue;
        }

        if (key == "TITLE") {
            m_tagTitle = value;
        }
        else if ((key == "ARTIST") || (key == "ALBUMARTIST")) {
            if (m_tagArtist.empty()) {
                m_tagArtist = value;
            }
        }
        else if (key == "ALBUM") {
            m_tagAlbum = value;
        }
        else if (key == "GENRE") {
            m_tagGenre = value;
        }
        else if (key == "COMMENT") {
            m_tagComment = value;
        }
        else if ((key == "DATE") || (key == "YEAR")) {
            if (m_tagDate.empty()) {
                m_tagDate = value;
            }
        }
    }
}
