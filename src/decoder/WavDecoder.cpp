/*
大模型：GPT 5.3 Codex
任务说明：todo_task_73.txt
*/
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <format>
#include <stdexcept>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "MediaTagNames.h"
#include "WavDecoder.h"

namespace {

static const char* sDecoderName = "dr_wav Decoder";

SeekBase ToStreamSeekBase(int origin)
{
    if (origin == SEEK_SET) {
        return SeekBase::Begin;
    }
    if (origin == SEEK_CUR) {
        return SeekBase::Cur;
    }

    return SeekBase::End;
}

bool IsWavContainer(drwav_container container)
{
    return (container == drwav_container_riff) || (container == drwav_container_rf64) || (container == drwav_container_rifx);
}

size_t WavReadProc(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    CDataStream* pStream = static_cast<CDataStream*>(pUserData);
    if ((pStream == nullptr) || (pBufferOut == nullptr) || (bytesToRead == 0)) {
        return 0;
    }

    const uint32_t readSize = (bytesToRead > static_cast<size_t>(UINT32_MAX)) ? UINT32_MAX : static_cast<uint32_t>(bytesToRead);
    return static_cast<size_t>(pStream->Read(pBufferOut, readSize));
}

drwav_bool32 WavSeekProc(void* pUserData, int offset, drwav_seek_origin origin)
{
    CDataStream* pStream = static_cast<CDataStream*>(pUserData);
    if (pStream == nullptr) {
        return DRWAV_FALSE;
    }

    try {
        pStream->Seek(ToStreamSeekBase(static_cast<int>(origin)), offset);
    }
    catch (...) {
        return DRWAV_FALSE;
    }

    return DRWAV_TRUE;
}

drwav_bool32 WavTellProc(void* pUserData, drwav_int64* pCursor)
{
    CDataStream* pStream = static_cast<CDataStream*>(pUserData);
    if ((pStream == nullptr) || (pCursor == nullptr)) {
        return DRWAV_FALSE;
    }

    try {
        *pCursor = static_cast<drwav_int64>(pStream->Tell());
    }
    catch (...) {
        return DRWAV_FALSE;
    }

    return DRWAV_TRUE;
}

} // namespace

std::string CWavDecoder::Name()
{
    return sDecoderName;
}

CWavDecoder::CWavDecoder(uint32_t streamFmt)
    : m_streamFmt(streamFmt)
    , m_decodeMtx()
    , m_wav(std::make_unique<drwav>())
    , m_AudioFmt({})
    , m_curFrames(0)
    , m_totalFrames(0)
    , m_fileTotalFrames(0)
{
    InitEmptyAudioFormat(&m_AudioFmt);
    m_name = sDecoderName;
    m_type = DECODE_TYPE_NATIVE;
}

CWavDecoder::~CWavDecoder()
{
    Reset();
}

bool CWavDecoder::InitDecode(const CDecodeInitCtx* decodeInit)
{
    (void)decodeInit;
    assert(m_pStream != nullptr);

    if (!drwav_init(m_wav.get(), WavReadProc, WavSeekProc, WavTellProc, m_pStream, nullptr)) {
        throw std::runtime_error("dr_wav fail to open wav data stream!");
    }

    const uint32_t channels = std::max<uint32_t>(1U, m_wav->channels);
    const uint32_t sampleRate = std::max<uint32_t>(1U, m_wav->sampleRate);
    const AudioDataFormat dataFmt = GuessOutputFormat(m_wav->translatedFormatTag, m_wav->bitsPerSample);
    const uint32_t bitsPerSample = std::max<uint32_t>(16U, GetBitsPerSampleByFormat(dataFmt));

    InitAudioFormat(&m_AudioFmt, dataFmt, channels, sampleRate, bitsPerSample);

    m_fileTotalFrames = static_cast<std::size_t>(m_wav->totalPCMFrameCount);
    m_totalFrames = m_fileTotalFrames;
    m_curFrames = 0;
    m_curStreamIdx = 0;
    m_StreamCount = 1;

    return true;
}

bool CWavDecoder::StartStream(uint32_t streamIdx, std::size_t begin, std::size_t end, uint32_t loop)
{
    (void)loop;
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_wav == nullptr) {
        return false;
    }
    if ((streamIdx != 0) && (streamIdx != static_cast<uint32_t>(-1))) {
        return false;
    }

    m_curStreamIdx = 0;
    m_totalFrames = m_fileTotalFrames;
    if (end < m_totalFrames) {
        m_totalFrames = end;
    }

    m_curFrames = (begin <= m_totalFrames) ? begin : m_totalFrames;
    return drwav_seek_to_pcm_frame(m_wav.get(), static_cast<drwav_uint64>(m_curFrames)) == DRWAV_TRUE;
}

void CWavDecoder::StopStream(uint32_t streamIdx)
{
    (void)streamIdx;
    std::lock_guard<std::mutex> guard(m_decodeMtx);
}

CMediaTag CWavDecoder::GetTags(uint32_t streamIdx)
{
    (void)streamIdx;
    CMediaTag tags;
    MakeMediaTags(tags);
    return tags;
}

uint32_t CWavDecoder::Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if ((m_wav == nullptr) || (pBuf == nullptr) || (audioFmt == nullptr) || (frames == 0) || !IsSupportOutput(audioFmt)) {
        return 0;
    }
    if (m_curFrames >= m_totalFrames) {
        return 0;
    }

    const uint32_t maxFrames = static_cast<uint32_t>(std::min<std::size_t>(frames, m_totalFrames - m_curFrames));
    if (maxFrames == 0) {
        return 0;
    }
    if (bufSize < (maxFrames * audioFmt->blockAlign)) {
        return 0;
    }

    drwav_uint64 readFrames = 0;
    if (audioFmt->format == AudioDataFormat::PCM_S16) {
        readFrames = drwav_read_pcm_frames_s16(m_wav.get(), maxFrames, reinterpret_cast<drwav_int16*>(pBuf));
    }
    else if (audioFmt->format == AudioDataFormat::PCM_S32) {
        readFrames = drwav_read_pcm_frames_s32(m_wav.get(), maxFrames, reinterpret_cast<drwav_int32*>(pBuf));
    }
    else if (audioFmt->format == AudioDataFormat::Float32) {
        readFrames = drwav_read_pcm_frames_f32(m_wav.get(), maxFrames, reinterpret_cast<float*>(pBuf));
    }
    else {
        readFrames = 0;
    }

    m_curFrames += static_cast<std::size_t>(readFrames);
    return static_cast<uint32_t>(readFrames);
}

void CWavDecoder::SeekTo(std::size_t frames)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_wav == nullptr) {
        return;
    }

    const std::size_t targetFrame = (frames <= m_totalFrames) ? frames : m_totalFrames;
    if (drwav_seek_to_pcm_frame(m_wav.get(), static_cast<drwav_uint64>(targetFrame)) == DRWAV_TRUE) {
        m_curFrames = targetFrame;
    }
}

bool CWavDecoder::IsSupportOutput(const AudioFormat* audioFmt) const
{
    if (audioFmt == nullptr) {
        return false;
    }
    if (audioFmt->sampleRate != m_AudioFmt.sampleRate) {
        return false;
    }
    if (audioFmt->numChannels != m_AudioFmt.numChannels) {
        return false;
    }

    return (audioFmt->format == AudioDataFormat::PCM_S16)
        || (audioFmt->format == AudioDataFormat::PCM_S32)
        || (audioFmt->format == AudioDataFormat::Float32);
}

bool CWavDecoder::IsCanSeeking(uint32_t streamIdx) const
{
    (void)streamIdx;
    if (m_pStream == nullptr) {
        return false;
    }

    return (m_pStream->GetType() & dsTypeSeekable) != 0;
}

void CWavDecoder::Reset()
{
    if (m_wav != nullptr) {
        drwav_uninit(m_wav.get());
        m_wav = std::make_unique<drwav>();
    }

    InitEmptyAudioFormat(&m_AudioFmt);
    m_curFrames = 0;
    m_totalFrames = 0;
    m_fileTotalFrames = 0;
}

AudioDataFormat CWavDecoder::GuessOutputFormat(uint16_t translatedFormatTag, uint16_t bitsPerSample)
{
    if (translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT) {
        return AudioDataFormat::Float32;
    }

    if (bitsPerSample <= 16) {
        return AudioDataFormat::PCM_S16;
    }

    return AudioDataFormat::PCM_S32;
}

const char* CWavDecoder::TypeNameFromStreamFormat(uint32_t streamFmt)
{
    if (streamFmt == StreamFormatWavEx) {
        return "WAVE Extensible";
    }

    return "WAV Audio";
}

void CWavDecoder::MakeMediaTags(CMediaTag& tags)
{
    tags.Clear();

    const std::string type = TypeNameFromStreamFormat(m_streamFmt);
    const std::string brief = std::format("{}: {}", type, AudioFormatBrifStr(&m_AudioFmt));

    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    const double totalSeconds = (m_AudioFmt.sampleRate == 0) ? 0.0 : (static_cast<double>(m_totalFrames) / m_AudioFmt.sampleRate);
    tags.AddTagDecimal(MediaTag_Duration, totalSeconds);

    tags.AddTagInteger(MediaTag_BitsPerSample, m_AudioFmt.bitsPerSample);
    tags.AddTagInteger(MediaTag_SamplesRate, m_AudioFmt.sampleRate);
    tags.AddTagInteger(MediaTag_Channels, m_AudioFmt.numChannels);
    tags.AddTagInteger(MediaTag_ChannelsLayout, m_AudioFmt.chLayout);
    tags.AddTagInteger(MediaTag_BitsRate, m_AudioFmt.bitsPerSample * m_AudioFmt.sampleRate * m_AudioFmt.numChannels);
}

uint32_t WavQueryFileType(const std::wstring& filename)
{
    drwav wav = {};
    if (!drwav_init_file_w(&wav, filename.c_str(), nullptr)) {
        return StreamFormatUnknown;
    }

    const bool isWavFamily = IsWavContainer(wav.container);
    const uint32_t streamFmt = (isWavFamily && (wav.fmt.formatTag == DR_WAVE_FORMAT_EXTENSIBLE)) ? StreamFormatWavEx : StreamFormatWav;

    drwav_uninit(&wav);
    return isWavFamily ? streamFmt : StreamFormatUnknown;
}
