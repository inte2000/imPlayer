/*
20260522 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_66.txt
*/
#include <algorithm>
#include <cstring>
#include <format>
#include <string>

#include "MediaTagNames.h"
#include "GmeFunc.h"
#include "GmePlayCtrl.h"

namespace {

constexpr uint32_t GME_BUFFER_FRAMES = 2048;

} // namespace

GmePlayCtrl::GmePlayCtrl()
    : m_stream(nullptr)
    , m_emu(nullptr)
    , m_fileData()
    , m_decodeBuf()
    , m_pluginConfig({})
    , m_srcAudioFmt({})
    , m_totalFrames(0)
    , m_curFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_streamCount(0)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
{
    InitSourceAudioFormatByConfig();
}

GmePlayCtrl::~GmePlayCtrl()
{
    Release();
}

bool GmePlayCtrl::Init(CDataStream* stream, uint32_t streamFmt, const PluginConfig& config)
{
    Release();
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;
    m_pluginConfig = config;
    InitSourceAudioFormatByConfig();

    if (!ReadAllStreamData()) {
        Release();
        return false;
    }

    gme_err_t err = gme_open_data(m_fileData.data(), static_cast<long>(m_fileData.size()), &m_emu, static_cast<int>(m_srcAudioFmt.sampleRate));
    if (err != nullptr) {
        Release();
        return false;
    }

    m_streamCount = static_cast<uint32_t>(std::max(gme_track_count(m_emu), 0));
    if (m_streamCount == 0) {
        Release();
        return false;
    }

    m_totalFrames = 0;
    m_curFrames = 0;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    return true;
}

void GmePlayCtrl::Release()
{
    if (m_emu != nullptr) {
        gme_delete(m_emu);
        m_emu = nullptr;
    }

    m_stream = nullptr;
    m_fileData.clear();
    m_decodeBuf.clear();
    m_totalFrames = 0;
    m_curFrames = 0;
    m_streamFmt = StreamFormatUnknown;
    m_streamCount = 0;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    InitSourceAudioFormatByConfig();
}

bool GmePlayCtrl::OpenStream(uint32_t streamIndex)
{
    if (m_emu == nullptr) {
        return false;
    }

    if (streamIndex == static_cast<uint32_t>(-1)) {
        streamIndex = 0;
    }
    if (streamIndex >= m_streamCount) {
        return false;
    }

    gme_err_t err = gme_start_track(m_emu, static_cast<int>(streamIndex));
    if (err != nullptr) {
        return false;
    }

    if (!RefreshDurationInfo(streamIndex)) {
        return false;
    }

    m_curFrames = 0;
    m_activeStreamIdx = streamIndex;
    m_opened = true;
    return true;
}

void GmePlayCtrl::StopStream()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool GmePlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
{
    if ((m_emu == nullptr) || (audioFmt == nullptr)) {
        return false;
    }
    if (audioFmt->sampleRate != m_srcAudioFmt.sampleRate) {
        return false;
    }
    if (audioFmt->numChannels != m_srcAudioFmt.numChannels) {
        return false;
    }

    switch (audioFmt->format)
    {
    case AudioDataFormat::PCM_S16:
    case AudioDataFormat::PCM_S32:
    case AudioDataFormat::Float32:
        return true;
    default:
        return false;
    }
}

bool GmePlayCtrl::IsCanSeeking() const
{
    return (m_emu != nullptr) && m_opened;
}

uint32_t GmePlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((m_emu == nullptr) || (!m_opened) || (pBuf == nullptr) || (frames == 0) || !IsSupportOutput(audioFmt)) {
        return 0;
    }

    uint32_t remainFrames = frames;
    if (m_totalFrames > 0) {
        remainFrames = static_cast<uint32_t>(std::min<std::size_t>(remainFrames, m_totalFrames - std::min(m_curFrames, m_totalFrames)));
    }

    uint32_t readFrames = 0;
    uint8_t* byteBuf = static_cast<uint8_t*>(pBuf);

    while (remainFrames > 0)
    {
        const uint32_t chunkFrames = std::min(remainFrames, GME_BUFFER_FRAMES);
        const uint32_t chunkSamples = chunkFrames * m_srcAudioFmt.numChannels;
        m_decodeBuf.resize(chunkSamples);

        gme_err_t err = gme_play(m_emu, static_cast<int>(chunkSamples), m_decodeBuf.data());
        if (err != nullptr) {
            break;
        }

        if (audioFmt->format == AudioDataFormat::PCM_S16) {
            std::memcpy(byteBuf, m_decodeBuf.data(), chunkSamples * sizeof(int16_t));
            byteBuf += chunkSamples * sizeof(int16_t);
        }
        else if (audioFmt->format == AudioDataFormat::PCM_S32) {
            int32_t* out = reinterpret_cast<int32_t*>(byteBuf);
            for (uint32_t i = 0; i < chunkSamples; ++i)
            {
                out[i] = ToPcmS32(m_decodeBuf[i]);
            }
            byteBuf += chunkSamples * sizeof(int32_t);
        }
        else {
            float* out = reinterpret_cast<float*>(byteBuf);
            for (uint32_t i = 0; i < chunkSamples; ++i)
            {
                out[i] = ToFloat32(m_decodeBuf[i]);
            }
            byteBuf += chunkSamples * sizeof(float);
        }

        readFrames += chunkFrames;
        m_curFrames += chunkFrames;
        remainFrames -= chunkFrames;

        if (gme_track_ended(m_emu) != 0) {
            break;
        }
    }

    return readFrames;
}

void GmePlayCtrl::Seek(std::size_t frames)
{
    if (m_emu == nullptr) {
        return;
    }

    std::size_t targetFrames = frames;
    if (m_totalFrames > 0) {
        targetFrames = std::min(targetFrames, m_totalFrames);
    }

    const int seekMsec = static_cast<int>((targetFrames * 1000ULL) / std::max(1U, m_srcAudioFmt.sampleRate));
    gme_err_t err = gme_seek(m_emu, seekMsec);
    if (err == nullptr) {
        m_curFrames = targetFrames;
    }
}

float GmePlayCtrl::GetDurationSeconds() const
{
    if ((m_srcAudioFmt.sampleRate == 0) || (m_totalFrames == 0)) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(m_totalFrames) / static_cast<double>(m_srcAudioFmt.sampleRate));
}

void GmePlayCtrl::FillMetaTags(CMediaTag& tags, uint32_t streamIndex) const
{
    tags.Clear();

    const std::string type = GmeStreamFormatName(m_streamFmt);
    const std::string brief = std::format("{}: {}", type, AudioFormatBrifStr(&m_srcAudioFmt));

    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    tags.AddTagInteger(MediaTag_Streams, static_cast<int32_t>(m_streamCount));
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());

    tags.AddTagInteger(MediaTag_SamplesRate, static_cast<int32_t>(m_srcAudioFmt.sampleRate));
    tags.AddTagInteger(MediaTag_Channels, static_cast<int32_t>(m_srcAudioFmt.numChannels));
    tags.AddTagInteger(MediaTag_ChannelsLayout, static_cast<int32_t>(m_srcAudioFmt.chLayout));
    tags.AddTagInteger(MediaTag_BitsPerSample, static_cast<int32_t>(m_srcAudioFmt.bitsPerSample));
    tags.AddTagString(MediaTag_PcmFormat, StringFromAudioFormat(m_srcAudioFmt.format));
    const uint32_t bitrates = m_srcAudioFmt.bitsPerSample * m_srcAudioFmt.sampleRate * m_srcAudioFmt.numChannels;
    tags.AddTagInteger(MediaTag_BitsRate, static_cast<int32_t>(bitrates));

    gme_info_t* info = nullptr;
    if (!QueryTrackInfo(streamIndex, &info) || (info == nullptr)) {
        return;
    }

    if ((info->song != nullptr) && (info->song[0] != '\0')) {
        tags.AddTagString(MediaTag_Title, info->song);
    }
    if ((info->author != nullptr) && (info->author[0] != '\0')) {
        tags.AddTagString(MediaTag_Artists, info->author);
    }
    if ((info->game != nullptr) && (info->game[0] != '\0')) {
        tags.AddTagString(MediaTag_Album, info->game);
    }
    if ((info->system != nullptr) && (info->system[0] != '\0')) {
        tags.AddTagString(MediaTag_Genre, info->system);
    }
    if ((info->copyright != nullptr) && (info->copyright[0] != '\0')) {
        tags.AddTagString(MediaTag_Comment, info->copyright);
    }
    if ((info->comment != nullptr) && (info->comment[0] != '\0')) {
        tags.AddTagString(MediaTag_Comment, info->comment);
    }

    gme_free_info(info);
}

bool GmePlayCtrl::ReadAllStreamData()
{
    if (m_stream == nullptr) {
        return false;
    }

    const std::size_t totalSize = m_stream->GetLength();
    if (totalSize == 0) {
        return false;
    }

    m_fileData.resize(totalSize);
    m_stream->Seek(SeekBase::Begin, 0);

    std::size_t readed = 0;
    while (readed < totalSize)
    {
        uint32_t once = m_stream->Read(m_fileData.data() + readed, static_cast<uint32_t>(totalSize - readed));
        if (once == 0) {
            break;
        }
        readed += once;
    }

    if (readed != totalSize) {
        m_fileData.resize(readed);
    }
    return !m_fileData.empty();
}

void GmePlayCtrl::InitSourceAudioFormatByConfig()
{
    const uint32_t sampleRate = (m_pluginConfig.SampleRate == 0) ? 44100 : m_pluginConfig.SampleRate;
    uint32_t bitsPerSample = m_pluginConfig.BitsPerSample;
    if ((bitsPerSample != 16) && (bitsPerSample != 24) && (bitsPerSample != 32)) {
        bitsPerSample = 32;
    }

    InitAudioFormat(&m_srcAudioFmt, AudioFormatByBitsPerSample(bitsPerSample), 2, sampleRate, bitsPerSample);
}

bool GmePlayCtrl::RefreshDurationInfo(uint32_t streamIndex)
{
    gme_info_t* info = nullptr;
    if (!QueryTrackInfo(streamIndex, &info) || (info == nullptr)) {
        m_totalFrames = 0;
        return false;
    }

    int lengthMsec = info->play_length;
    if (lengthMsec <= 0) {
        lengthMsec = info->length;
    }
    if ((lengthMsec <= 0) && (info->intro_length > 0) && (info->loop_length > 0)) {
        lengthMsec = info->intro_length + info->loop_length * 2;
    }

    if (lengthMsec > 0) {
        m_totalFrames = static_cast<std::size_t>((static_cast<uint64_t>(lengthMsec) * m_srcAudioFmt.sampleRate) / 1000ULL);
    }
    else {
        m_totalFrames = 0;
    }

    gme_free_info(info);
    return true;
}

bool GmePlayCtrl::QueryTrackInfo(uint32_t streamIndex, gme_info_t** outInfo) const
{
    if ((m_emu == nullptr) || (outInfo == nullptr)) {
        return false;
    }

    if (streamIndex == static_cast<uint32_t>(-1)) {
        streamIndex = (m_activeStreamIdx == static_cast<uint32_t>(-1)) ? 0 : m_activeStreamIdx;
    }
    if (streamIndex >= m_streamCount) {
        return false;
    }

    *outInfo = nullptr;
    gme_err_t err = gme_track_info(m_emu, outInfo, static_cast<int>(streamIndex));
    return (err == nullptr);
}

int32_t GmePlayCtrl::ToPcmS32(int16_t sample)
{
    return static_cast<int32_t>(sample) << 16;
}

float GmePlayCtrl::ToFloat32(int16_t sample)
{
    return static_cast<float>(sample) / 32768.0f;
}
