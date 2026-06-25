/*
20260526 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_78.txt
*/
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <format>
#include <limits>

#include "MediaTagNames.h"
#include "FlacPlayCtrl.h"

namespace {

std::string ToUpperAscii(const std::string& text)
{
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

int16_t ConvertToS16(int32_t sample, uint32_t bps)
{
    if (bps == 0) {
        bps = 16;
    }
    int32_t value = sample;
    if (bps > 16) {
        value >>= (bps - 16);
    }
    else if (bps < 16) {
        value <<= (16 - bps);
    }
    value = std::clamp(value, -0x8000, 0x7FFF);
    return static_cast<int16_t>(value);
}

int32_t ConvertToS32(int32_t sample, uint32_t bps)
{
    if (bps == 0) {
        bps = 16;
    }
    int64_t value = sample;
    if (bps < 32) {
        value <<= (32 - bps);
    }
    else if (bps > 32) {
        value >>= (bps - 32);
    }
    value = std::clamp<int64_t>(value, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
    return static_cast<int32_t>(value);
}

float ConvertToFloat32(int32_t sample, uint32_t bps)
{
    if (bps == 0) {
        bps = 16;
    }
    const double scale = std::ldexp(1.0, static_cast<int>(bps - 1));
    if (scale <= 0.0) {
        return 0.0f;
    }

    const double v = static_cast<double>(sample) / scale;
    return static_cast<float>(std::clamp(v, -1.0, 0.9999999403953552));
}

} // namespace

FlacPlayCtrl::FlacPlayCtrl()
    : m_stream(nullptr)
    , m_decoder(nullptr)
    , m_srcAudioFmt({})
    , m_curFrames(0)
    , m_totalFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
    , m_endOfStream(false)
    , m_hasError(false)
    , m_decodeChannels(0)
    , m_decodeSampleRate(0)
    , m_decodeBitsPerSample(0)
    , m_pcmCache()
    , m_cacheOffsetFrames(0)
    , m_tagTitle()
    , m_tagArtist()
    , m_tagAlbum()
    , m_tagGenre()
    , m_tagComment()
    , m_tagDate()
{
    InitEmptyAudioFormat(&m_srcAudioFmt);
}

FlacPlayCtrl::~FlacPlayCtrl()
{
    Release();
}

bool FlacPlayCtrl::Init(CDataStream* stream, uint32_t streamFmt)
{
    Release();
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;

    m_decoder = FLAC__stream_decoder_new();
    if (m_decoder == nullptr) {
        Release();
        return false;
    }

    FLAC__stream_decoder_set_metadata_respond(m_decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
    const FLAC__StreamDecoderInitStatus initStatus = FLAC__stream_decoder_init_stream(
        m_decoder,
        ReadCallback,
        SeekCallback,
        TellCallback,
        LengthCallback,
        EofCallback,
        WriteCallback,
        MetadataCallback,
        ErrorCallback,
        this);
    if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        Release();
        return false;
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(m_decoder)) {
        Release();
        return false;
    }

    if ((m_decodeSampleRate == 0) || (m_decodeChannels == 0)) {
        Release();
        return false;
    }

    uint32_t bitsPerSample = m_decodeBitsPerSample;
    if ((bitsPerSample == 0) || (bitsPerSample > 32)) {
        bitsPerSample = 24;
    }
    InitAudioFormat(&m_srcAudioFmt, AudioFormatByBitsPerSample(bitsPerSample), m_decodeChannels, m_decodeSampleRate, bitsPerSample);

    const FLAC__uint64 totalSamples = FLAC__stream_decoder_get_total_samples(m_decoder);
    m_totalFrames = static_cast<std::size_t>(totalSamples);
    m_curFrames = 0;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    m_endOfStream = false;
    m_hasError = false;
    m_pcmCache.clear();
    m_cacheOffsetFrames = 0;
    return true;
}

void FlacPlayCtrl::Release()
{
    if (m_decoder != nullptr) {
        FLAC__stream_decoder_finish(m_decoder);
        FLAC__stream_decoder_delete(m_decoder);
        m_decoder = nullptr;
    }

    m_stream = nullptr;
    InitEmptyAudioFormat(&m_srcAudioFmt);
    m_curFrames = 0;
    m_totalFrames = 0;
    m_streamFmt = StreamFormatUnknown;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    m_endOfStream = false;
    m_hasError = false;
    m_decodeChannels = 0;
    m_decodeSampleRate = 0;
    m_decodeBitsPerSample = 0;
    m_pcmCache.clear();
    m_cacheOffsetFrames = 0;
    m_tagTitle.clear();
    m_tagArtist.clear();
    m_tagAlbum.clear();
    m_tagGenre.clear();
    m_tagComment.clear();
    m_tagDate.clear();
}

bool FlacPlayCtrl::OpenStream(uint32_t streamIndex)
{
    if (m_decoder == nullptr) {
        return false;
    }
    if ((streamIndex != 0) && (streamIndex != static_cast<uint32_t>(-1))) {
        return false;
    }

    if (FLAC__stream_decoder_seek_absolute(m_decoder, 0) == false) {
        return false;
    }

    m_curFrames = 0;
    m_pcmCache.clear();
    m_cacheOffsetFrames = 0;
    m_activeStreamIdx = 0;
    m_opened = true;
    m_endOfStream = false;
    m_hasError = false;
    return true;
}

void FlacPlayCtrl::StopStream()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool FlacPlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
{
    if (audioFmt == nullptr) {
        return false;
    }
    if ((audioFmt->format != AudioDataFormat::PCM_S16) && (audioFmt->format != AudioDataFormat::PCM_S32) && (audioFmt->format != AudioDataFormat::Float32)) {
        return false;
    }
    if (audioFmt->sampleRate != m_srcAudioFmt.sampleRate) {
        return false;
    }
    if (audioFmt->numChannels != m_srcAudioFmt.numChannels) {
        return false;
    }

    return true;
}

bool FlacPlayCtrl::IsCanSeeking() const
{
    if (m_stream == nullptr) {
        return false;
    }

    return (m_stream->GetStyle() & dsStyleSeekable) != 0;
}

uint32_t FlacPlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((m_decoder == nullptr) || (!m_opened) || (pBuf == nullptr) || (frames == 0) || !IsSupportOutput(audioFmt)) {
        return 0;
    }

    uint32_t requestFrames = frames;
    if (m_totalFrames > 0) {
        requestFrames = static_cast<uint32_t>(std::min<std::size_t>(requestFrames, m_totalFrames - std::min(m_curFrames, m_totalFrames)));
    }
    if (requestFrames == 0) {
        return 0;
    }

    if (!EnsureCacheFrames(requestFrames)) {
        return 0;
    }

    const uint32_t availableFrames = GetAvailableCacheFrames();
    const uint32_t readFrames = std::min(requestFrames, availableFrames);
    if (readFrames == 0) {
        return 0;
    }

    const uint32_t channels = m_srcAudioFmt.numChannels;
    const uint32_t bps = (m_decodeBitsPerSample == 0) ? m_srcAudioFmt.bitsPerSample : m_decodeBitsPerSample;
    const std::size_t baseSample = m_cacheOffsetFrames * channels;

    if (audioFmt->format == AudioDataFormat::PCM_S16)
    {
        int16_t* out = static_cast<int16_t*>(pBuf);
        for (uint32_t i = 0; i < readFrames; ++i)
        {
            for (uint32_t ch = 0; ch < channels; ++ch)
            {
                const int32_t sample = m_pcmCache[baseSample + static_cast<std::size_t>(i) * channels + ch];
                out[static_cast<std::size_t>(i) * channels + ch] = ConvertToS16(sample, bps);
            }
        }
    }
    else if (audioFmt->format == AudioDataFormat::PCM_S32)
    {
        int32_t* out = static_cast<int32_t*>(pBuf);
        for (uint32_t i = 0; i < readFrames; ++i)
        {
            for (uint32_t ch = 0; ch < channels; ++ch)
            {
                const int32_t sample = m_pcmCache[baseSample + static_cast<std::size_t>(i) * channels + ch];
                out[static_cast<std::size_t>(i) * channels + ch] = ConvertToS32(sample, bps);
            }
        }
    }
    else
    {
        float* out = static_cast<float*>(pBuf);
        for (uint32_t i = 0; i < readFrames; ++i)
        {
            for (uint32_t ch = 0; ch < channels; ++ch)
            {
                const int32_t sample = m_pcmCache[baseSample + static_cast<std::size_t>(i) * channels + ch];
                out[static_cast<std::size_t>(i) * channels + ch] = ConvertToFloat32(sample, bps);
            }
        }
    }

    ConsumeCacheFrames(readFrames);
    m_curFrames += readFrames;
    return readFrames;
}

void FlacPlayCtrl::Seek(std::size_t frames)
{
    if (m_decoder == nullptr) {
        return;
    }

    std::size_t targetFrame = frames;
    if (m_totalFrames > 0) {
        targetFrame = std::min(targetFrame, m_totalFrames);
    }

    if (FLAC__stream_decoder_seek_absolute(m_decoder, static_cast<FLAC__uint64>(targetFrame)))
    {
        m_curFrames = targetFrame;
        m_pcmCache.clear();
        m_cacheOffsetFrames = 0;
        m_endOfStream = false;
        m_hasError = false;
    }
}

float FlacPlayCtrl::GetDurationSeconds() const
{
    if ((m_srcAudioFmt.sampleRate == 0) || (m_totalFrames == 0)) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(m_totalFrames) / static_cast<double>(m_srcAudioFmt.sampleRate));
}

void FlacPlayCtrl::FillMetaTags(CMediaTag& tags) const
{
    tags.Clear();

    const std::string type = "FLAC Audio";
    const std::string brief = std::format("{}: {}", type, AudioFormatBrifStr(&m_srcAudioFmt));

    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());
    tags.AddTagInteger(MediaTag_SamplesRate, static_cast<int32_t>(m_srcAudioFmt.sampleRate));
    tags.AddTagInteger(MediaTag_Channels, static_cast<int32_t>(m_srcAudioFmt.numChannels));
    tags.AddTagInteger(MediaTag_ChannelsLayout, static_cast<int32_t>(m_srcAudioFmt.chLayout));
    tags.AddTagInteger(MediaTag_BitsPerSample, static_cast<int32_t>(m_srcAudioFmt.bitsPerSample));
    tags.AddTagString(MediaTag_PcmFormat, StringFromAudioFormat(m_srcAudioFmt.format));
    tags.AddTagInteger(MediaTag_BitsRate, static_cast<int32_t>(m_srcAudioFmt.bitsPerSample * m_srcAudioFmt.sampleRate * m_srcAudioFmt.numChannels));

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

bool FlacPlayCtrl::EnsureCacheFrames(uint32_t frames)
{
    while ((GetAvailableCacheFrames() < frames) && !m_endOfStream && !m_hasError)
    {
        if (!FLAC__stream_decoder_process_single(m_decoder)) {
            m_hasError = true;
            break;
        }

        const FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(m_decoder);
        if (state == FLAC__STREAM_DECODER_END_OF_STREAM) {
            m_endOfStream = true;
        }
        if ((state == FLAC__STREAM_DECODER_ABORTED) || (state == FLAC__STREAM_DECODER_OGG_ERROR) || (state == FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR)) {
            m_hasError = true;
        }
    }

    return !m_hasError;
}

uint32_t FlacPlayCtrl::GetAvailableCacheFrames() const
{
    if (m_decodeChannels == 0) {
        return 0;
    }

    const std::size_t totalFrames = m_pcmCache.size() / m_decodeChannels;
    if (totalFrames <= m_cacheOffsetFrames) {
        return 0;
    }

    return static_cast<uint32_t>(totalFrames - m_cacheOffsetFrames);
}

void FlacPlayCtrl::ConsumeCacheFrames(uint32_t frames)
{
    m_cacheOffsetFrames += frames;
    CompactCache();
}

void FlacPlayCtrl::CompactCache()
{
    if (m_decodeChannels == 0) {
        m_pcmCache.clear();
        m_cacheOffsetFrames = 0;
        return;
    }

    if (m_cacheOffsetFrames == 0) {
        return;
    }
    if ((m_cacheOffsetFrames < 4096) && (m_cacheOffsetFrames * m_decodeChannels < m_pcmCache.size() / 2)) {
        return;
    }

    const std::size_t dropSamples = m_cacheOffsetFrames * m_decodeChannels;
    if (dropSamples >= m_pcmCache.size()) {
        m_pcmCache.clear();
    }
    else {
        m_pcmCache.erase(m_pcmCache.begin(), m_pcmCache.begin() + static_cast<std::ptrdiff_t>(dropSamples));
    }
    m_cacheOffsetFrames = 0;
}

void FlacPlayCtrl::ParseVorbisCommentEntry(const FLAC__StreamMetadata_VorbisComment_Entry& entry)
{
    if ((entry.entry == nullptr) || (entry.length == 0)) {
        return;
    }

    const std::string kv(reinterpret_cast<const char*>(entry.entry), entry.length);
    const std::size_t pos = kv.find('=');
    if ((pos == std::string::npos) || (pos == 0)) {
        return;
    }

    const std::string key = ToUpperAscii(kv.substr(0, pos));
    const std::string value = kv.substr(pos + 1);
    if (value.empty()) {
        return;
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

FLAC__StreamDecoderReadStatus FlacPlayCtrl::ReadCallback(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data)
{
    (void)decoder;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if ((self == nullptr) || (self->m_stream == nullptr) || (buffer == nullptr) || (bytes == nullptr)) {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }

    if (*bytes == 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }

    const uint32_t ask = (*bytes > static_cast<size_t>(UINT32_MAX)) ? UINT32_MAX : static_cast<uint32_t>(*bytes);
    const uint32_t readed = self->m_stream->Read(buffer, ask);
    *bytes = readed;
    if (readed > 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}

FLAC__StreamDecoderSeekStatus FlacPlayCtrl::SeekCallback(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data)
{
    (void)decoder;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if ((self == nullptr) || (self->m_stream == nullptr)) {
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }

    try {
        self->m_stream->Seek(SeekBase::Begin, static_cast<long long>(absolute_byte_offset));
    }
    catch (...) {
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus FlacPlayCtrl::TellCallback(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data)
{
    (void)decoder;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if ((self == nullptr) || (self->m_stream == nullptr) || (absolute_byte_offset == nullptr)) {
        return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    }

    try {
        *absolute_byte_offset = static_cast<FLAC__uint64>(self->m_stream->Tell());
    }
    catch (...) {
        return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    }
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus FlacPlayCtrl::LengthCallback(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data)
{
    (void)decoder;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if ((self == nullptr) || (self->m_stream == nullptr) || (stream_length == nullptr)) {
        return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
    }

    *stream_length = static_cast<FLAC__uint64>(self->m_stream->GetLength());
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool FlacPlayCtrl::EofCallback(const FLAC__StreamDecoder* decoder, void* client_data)
{
    (void)decoder;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if ((self == nullptr) || (self->m_stream == nullptr)) {
        return true;
    }

    try {
        return (self->m_stream->Tell() >= self->m_stream->GetLength()) ? true : false;
    }
    catch (...) {
        return true;
    }
}

FLAC__StreamDecoderWriteStatus FlacPlayCtrl::WriteCallback(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data)
{
    (void)decoder;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if ((self == nullptr) || (frame == nullptr) || (buffer == nullptr)) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    const uint32_t channels = frame->header.channels;
    const uint32_t blocksize = frame->header.blocksize;
    const uint32_t bps = frame->header.bits_per_sample;
    if ((channels == 0) || (blocksize == 0) || (channels > 8)) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    if (self->m_decodeChannels == 0) {
        self->m_decodeChannels = channels;
    }
    if (self->m_decodeSampleRate == 0) {
        self->m_decodeSampleRate = frame->header.sample_rate;
    }
    if (self->m_decodeBitsPerSample == 0) {
        self->m_decodeBitsPerSample = bps;
    }

    const std::size_t oldSize = self->m_pcmCache.size();
    self->m_pcmCache.resize(oldSize + static_cast<std::size_t>(blocksize) * channels);
    int32_t* out = self->m_pcmCache.data() + oldSize;

    for (uint32_t i = 0; i < blocksize; ++i)
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            out[static_cast<std::size_t>(i) * channels + ch] = buffer[ch][i];
        }
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacPlayCtrl::MetadataCallback(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data)
{
    (void)decoder;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if ((self == nullptr) || (metadata == nullptr)) {
        return;
    }

    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        self->m_decodeSampleRate = metadata->data.stream_info.sample_rate;
        self->m_decodeChannels = metadata->data.stream_info.channels;
        self->m_decodeBitsPerSample = metadata->data.stream_info.bits_per_sample;
        self->m_totalFrames = static_cast<std::size_t>(metadata->data.stream_info.total_samples);
        return;
    }

    if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
    {
        const auto& vc = metadata->data.vorbis_comment;
        for (uint32_t i = 0; i < vc.num_comments; ++i)
        {
            self->ParseVorbisCommentEntry(vc.comments[i]);
        }
    }
}

void FlacPlayCtrl::ErrorCallback(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data)
{
    (void)decoder;
    (void)status;
    FlacPlayCtrl* self = static_cast<FlacPlayCtrl*>(client_data);
    if (self != nullptr) {
        self->m_hasError = true;
    }
}
