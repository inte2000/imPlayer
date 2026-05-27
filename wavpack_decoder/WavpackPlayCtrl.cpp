/*
20260527 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_84.txt
*/
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include "MediaTagNames.h"
#include "WavpackPlayCtrl.h"

namespace {

int16_t S32ToS16(int32_t sample, uint32_t bitsPerSample)
{
    if (bitsPerSample == 0) {
        bitsPerSample = 16;
    }

    int32_t value = sample;
    if (bitsPerSample > 16) {
        value >>= static_cast<int>(bitsPerSample - 16);
    }
    else if (bitsPerSample < 16) {
        value <<= static_cast<int>(16 - bitsPerSample);
    }

    value = std::clamp(value, -0x8000, 0x7FFF);
    return static_cast<int16_t>(value);
}

int32_t S32ToS32(int32_t sample, uint32_t bitsPerSample)
{
    if (bitsPerSample == 0) {
        bitsPerSample = 16;
    }

    int64_t value = sample;
    if (bitsPerSample < 32) {
        value <<= static_cast<int>(32 - bitsPerSample);
    }
    else if (bitsPerSample > 32) {
        value >>= static_cast<int>(bitsPerSample - 32);
    }

    value = std::clamp<int64_t>(value, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
    return static_cast<int32_t>(value);
}

float S32ToF32(int32_t sample, uint32_t bitsPerSample)
{
    if (bitsPerSample == 0) {
        bitsPerSample = 16;
    }
    const double scale = std::ldexp(1.0, static_cast<int>(bitsPerSample - 1));
    if (scale <= 0.0) {
        return 0.0f;
    }

    const double v = static_cast<double>(sample) / scale;
    return static_cast<float>(std::clamp(v, -1.0, 0.9999999403953552));
}

} // namespace

WavpackPlayCtrl::WavpackPlayCtrl()
    : m_stream(nullptr)
    , m_ctx(nullptr)
    , m_reader({})
    , m_srcAudioFmt({})
    , m_curFrames(0)
    , m_totalFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
    , m_sourceBitsPerSample(0)
    , m_tempS32()
{
    InitEmptyAudioFormat(&m_srcAudioFmt);
}

WavpackPlayCtrl::~WavpackPlayCtrl()
{
    Release();
}

bool WavpackPlayCtrl::Init(CDataStream* stream, uint32_t streamFmt)
{
    Release();
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;

    m_reader.read_bytes = ReadBytes;
    m_reader.write_bytes = WriteBytes;
    m_reader.get_pos = GetPos;
    m_reader.set_pos_abs = SetPosAbs;
    m_reader.set_pos_rel = SetPosRel;
    m_reader.push_back_byte = PushBackByte;
    m_reader.get_length = GetLength;
    m_reader.can_seek = CanSeek;
    m_reader.truncate_here = TruncateHere;
    m_reader.close = CloseReader;

    char error[80] = {};
    m_ctx = WavpackOpenFileInputEx64(&m_reader, this, nullptr, error, OPEN_TAGS, 0);
    if (m_ctx == nullptr) {
        Release();
        return false;
    }

    const uint32_t channels = static_cast<uint32_t>(std::max(1, WavpackGetReducedChannels(m_ctx)));
    const uint32_t sampleRate = WavpackGetSampleRate(m_ctx);
    m_sourceBitsPerSample = static_cast<uint32_t>(std::max(1, WavpackGetBitsPerSample(m_ctx)));
    if ((sampleRate == 0) || (channels == 0)) {
        Release();
        return false;
    }

    uint32_t outBits = m_sourceBitsPerSample;
    if ((outBits != 16) && (outBits != 24) && (outBits != 32)) {
        outBits = 32;
    }
    InitAudioFormat(&m_srcAudioFmt, AudioFormatByBitsPerSample(outBits), channels, sampleRate, outBits);

    const int64_t totalSamples = WavpackGetNumSamples64(m_ctx);
    m_totalFrames = (totalSamples > 0) ? static_cast<std::size_t>(totalSamples) : 0;
    m_curFrames = 0;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    return true;
}

void WavpackPlayCtrl::Release()
{
    if (m_ctx != nullptr) {
        WavpackCloseFile(m_ctx);
        m_ctx = nullptr;
    }

    m_stream = nullptr;
    m_reader = {};
    InitEmptyAudioFormat(&m_srcAudioFmt);
    m_curFrames = 0;
    m_totalFrames = 0;
    m_streamFmt = StreamFormatUnknown;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    m_sourceBitsPerSample = 0;
    m_tempS32.clear();
}

bool WavpackPlayCtrl::OpenStream(uint32_t streamIndex)
{
    if (m_ctx == nullptr) {
        return false;
    }
    if ((streamIndex != 0) && (streamIndex != static_cast<uint32_t>(-1))) {
        return false;
    }

    if (WavpackSeekSample64(m_ctx, 0) == 0) {
        return false;
    }

    m_curFrames = 0;
    m_activeStreamIdx = 0;
    m_opened = true;
    return true;
}

void WavpackPlayCtrl::StopStream()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool WavpackPlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
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

bool WavpackPlayCtrl::IsCanSeeking() const
{
    if (m_stream == nullptr) {
        return false;
    }

    return (m_stream->GetType() & dsTypeSeekable) != 0;
}

uint32_t WavpackPlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((m_ctx == nullptr) || (!m_opened) || (pBuf == nullptr) || (frames == 0) || !IsSupportOutput(audioFmt)) {
        return 0;
    }

    uint32_t requestFrames = frames;
    if (m_totalFrames > 0) {
        requestFrames = static_cast<uint32_t>(std::min<std::size_t>(requestFrames, m_totalFrames - std::min(m_curFrames, m_totalFrames)));
    }
    if (requestFrames == 0) {
        return 0;
    }

    const uint32_t channels = m_srcAudioFmt.numChannels;
    m_tempS32.resize(static_cast<std::size_t>(requestFrames) * channels);
    const uint32_t gotFrames = WavpackUnpackSamples(m_ctx, m_tempS32.data(), requestFrames);
    if (gotFrames == 0) {
        return 0;
    }

    const uint32_t gotSamples = gotFrames * channels;
    if (audioFmt->format == AudioDataFormat::PCM_S16)
    {
        int16_t* out = static_cast<int16_t*>(pBuf);
        for (uint32_t i = 0; i < gotSamples; ++i)
        {
            out[i] = S32ToS16(m_tempS32[i], m_sourceBitsPerSample);
        }
    }
    else if (audioFmt->format == AudioDataFormat::PCM_S32)
    {
        int32_t* out = static_cast<int32_t*>(pBuf);
        for (uint32_t i = 0; i < gotSamples; ++i)
        {
            out[i] = S32ToS32(m_tempS32[i], m_sourceBitsPerSample);
        }
    }
    else
    {
        float* out = static_cast<float*>(pBuf);
        for (uint32_t i = 0; i < gotSamples; ++i)
        {
            out[i] = S32ToF32(m_tempS32[i], m_sourceBitsPerSample);
        }
    }

    m_curFrames += gotFrames;
    const int64_t cur = WavpackGetSampleIndex64(m_ctx);
    if (cur >= 0) {
        m_curFrames = static_cast<std::size_t>(cur);
    }

    return gotFrames;
}

void WavpackPlayCtrl::Seek(std::size_t frames)
{
    if (m_ctx == nullptr) {
        return;
    }

    std::size_t target = frames;
    if (m_totalFrames > 0) {
        target = std::min(target, m_totalFrames);
    }

    if (WavpackSeekSample64(m_ctx, static_cast<int64_t>(target)) != 0) {
        m_curFrames = target;
    }
}

float WavpackPlayCtrl::GetDurationSeconds() const
{
    if ((m_srcAudioFmt.sampleRate == 0) || (m_totalFrames == 0)) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(m_totalFrames) / static_cast<double>(m_srcAudioFmt.sampleRate));
}

void WavpackPlayCtrl::FillMetaTags(CMediaTag& tags) const
{
    tags.Clear();

    const std::string type = "WavPack";
    const std::string brief = std::format("{}: {}", type, AudioFormatBrifStr(&m_srcAudioFmt));

    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());
    tags.AddTagInteger(MediaTag_BitsPerSample, static_cast<int32_t>(m_srcAudioFmt.bitsPerSample));
    tags.AddTagInteger(MediaTag_SamplesRate, static_cast<int32_t>(m_srcAudioFmt.sampleRate));
    tags.AddTagInteger(MediaTag_Channels, static_cast<int32_t>(m_srcAudioFmt.numChannels));
    tags.AddTagInteger(MediaTag_ChannelsLayout, static_cast<int32_t>(m_srcAudioFmt.chLayout));
    tags.AddTagInteger(MediaTag_BitsRate, static_cast<int32_t>(m_srcAudioFmt.bitsPerSample * m_srcAudioFmt.sampleRate * m_srcAudioFmt.numChannels));

    const std::string title = ReadTagValue("Title");
    const std::string artist = ReadTagValue("Artist");
    const std::string album = ReadTagValue("Album");
    const std::string genre = ReadTagValue("Genre");
    const std::string comment = ReadTagValue("Comment");
    std::string date = ReadTagValue("Year");
    if (date.empty()) {
        date = ReadTagValue("Date");
    }

    if (!title.empty()) {
        tags.AddTagString(MediaTag_Title, title);
    }
    if (!artist.empty()) {
        tags.AddTagString(MediaTag_Artists, artist);
    }
    if (!album.empty()) {
        tags.AddTagString(MediaTag_Album, album);
    }
    if (!genre.empty()) {
        tags.AddTagString(MediaTag_Genre, genre);
    }
    if (!comment.empty()) {
        tags.AddTagString(MediaTag_Comment, comment);
    }
    if (!date.empty()) {
        tags.AddTagString(MediaTag_Date, date);
    }
}

int32_t WavpackPlayCtrl::ReadBytes(void* id, void* data, int32_t bcount)
{
    WavpackPlayCtrl* self = static_cast<WavpackPlayCtrl*>(id);
    if ((self == nullptr) || (self->m_stream == nullptr) || (data == nullptr) || (bcount <= 0)) {
        return 0;
    }

    return static_cast<int32_t>(self->m_stream->Read(data, static_cast<uint32_t>(bcount)));
}

int32_t WavpackPlayCtrl::WriteBytes(void* id, void* data, int32_t bcount)
{
    (void)id;
    (void)data;
    (void)bcount;
    return 0;
}

int64_t WavpackPlayCtrl::GetPos(void* id)
{
    WavpackPlayCtrl* self = static_cast<WavpackPlayCtrl*>(id);
    if ((self == nullptr) || (self->m_stream == nullptr)) {
        return -1;
    }

    try {
        return static_cast<int64_t>(self->m_stream->Tell());
    }
    catch (...) {
        return -1;
    }
}

int WavpackPlayCtrl::SetPosAbs(void* id, int64_t pos)
{
    WavpackPlayCtrl* self = static_cast<WavpackPlayCtrl*>(id);
    if ((self == nullptr) || (self->m_stream == nullptr)) {
        return 0;
    }

    try {
        self->m_stream->Seek(SeekBase::Begin, pos);
        return 1;
    }
    catch (...) {
        return 0;
    }
}

int WavpackPlayCtrl::SetPosRel(void* id, int64_t delta, int mode)
{
    WavpackPlayCtrl* self = static_cast<WavpackPlayCtrl*>(id);
    if ((self == nullptr) || (self->m_stream == nullptr)) {
        return 0;
    }

    SeekBase base = SeekBase::Begin;
    if (mode == SEEK_CUR) {
        base = SeekBase::Cur;
    }
    else if (mode == SEEK_END) {
        base = SeekBase::End;
    }

    try {
        self->m_stream->Seek(base, delta);
        return 1;
    }
    catch (...) {
        return 0;
    }
}

int WavpackPlayCtrl::PushBackByte(void* id, int c)
{
    (void)id;
    (void)c;
    return EOF;
}

int64_t WavpackPlayCtrl::GetLength(void* id)
{
    WavpackPlayCtrl* self = static_cast<WavpackPlayCtrl*>(id);
    if ((self == nullptr) || (self->m_stream == nullptr)) {
        return 0;
    }

    return static_cast<int64_t>(self->m_stream->GetLength());
}

int WavpackPlayCtrl::CanSeek(void* id)
{
    WavpackPlayCtrl* self = static_cast<WavpackPlayCtrl*>(id);
    if ((self == nullptr) || (self->m_stream == nullptr)) {
        return 0;
    }

    return ((self->m_stream->GetType() & dsTypeSeekable) != 0) ? 1 : 0;
}

int WavpackPlayCtrl::TruncateHere(void* id)
{
    (void)id;
    return 0;
}

int WavpackPlayCtrl::CloseReader(void* id)
{
    (void)id;
    return 0;
}

std::string WavpackPlayCtrl::ReadTagValue(const char* key) const
{
    if ((m_ctx == nullptr) || (key == nullptr)) {
        return {};
    }

    const int len = WavpackGetTagItem(const_cast<WavpackContext*>(m_ctx), key, nullptr, 0);
    if (len <= 0) {
        return {};
    }

    std::string value;
    value.resize(static_cast<std::size_t>(len) + 1);
    if (WavpackGetTagItem(const_cast<WavpackContext*>(m_ctx), key, value.data(), len + 1) <= 0) {
        return {};
    }
    value.resize(static_cast<std::size_t>(len));
    return value;
}
