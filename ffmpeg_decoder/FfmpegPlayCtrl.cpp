/*
20260523 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_53.txt

�޸ļ�¼��
��ģ�ͣ�ChatGPT 5.3 Codex
todo_task_54.txt
todo_task_57.txt
todo_task_58.txt
todo_task_59.txt
*/
#include <algorithm>
#include <cctype>
#include <cstring>
#include <format>
#include <mutex>
#include <string>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/dict.h>
}

#include "MediaTagNames.h"
#include "FfmpegFunc.h"
#include "FfmpegPlayCtrl.h"

namespace {

constexpr int IO_BUFFER_SIZE = 64 * 1024;
constexpr const char* DURATION_ESTIMATE_WARN = "Estimating duration from bitrate, this may be inaccurate";

void FfmpegLogCallback(void* ptr, int level, const char* fmt, va_list vl)
{
    char msg[1024] = {};
    va_list vlMsg;
    va_copy(vlMsg, vl);
    vsnprintf(msg, sizeof(msg), fmt, vlMsg);
    va_end(vlMsg);

    if (strstr(msg, DURATION_ESTIMATE_WARN) != nullptr) {
        return;
    }

    va_list vlDefault;
    va_copy(vlDefault, vl);
    av_log_default_callback(ptr, level, fmt, vlDefault);
    va_end(vlDefault);
}

void InitFfmpegLogFilter()
{
    static std::once_flag once;
    std::call_once(once, []() {
        av_log_set_callback(FfmpegLogCallback);
    });
}

std::string DictValue(const AVDictionary* metadata, const char* key)
{
    const AVDictionaryEntry* e = av_dict_get(metadata, key, nullptr, 0);
    return (e == nullptr || e->value == nullptr) ? std::string() : std::string(e->value);
}

std::string ToLowerAscii(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string CodecDisplayName(AVCodecID codecId)
{
    switch (codecId)
    {
    case AV_CODEC_ID_MP3: return "mpeg layer III";
    case AV_CODEC_ID_MP2: return "mpeg layer II";
    case AV_CODEC_ID_MP1: return "mpeg layer I";
    case AV_CODEC_ID_FLAC: return "flac";
    case AV_CODEC_ID_AAC: return "aac";
    case AV_CODEC_ID_VORBIS: return "vorbis";
    case AV_CODEC_ID_OPUS: return "opus";
    case AV_CODEC_ID_APE: return "ape";
    case AV_CODEC_ID_WMAV1:
    case AV_CODEC_ID_WMAV2:
    case AV_CODEC_ID_WMAPRO:
    case AV_CODEC_ID_WMALOSSLESS: return "wma";
    case AV_CODEC_ID_DTS: return "dts";
    case AV_CODEC_ID_AMR_NB:
    case AV_CODEC_ID_AMR_WB: return "amr";
    case AV_CODEC_ID_DSD_LSBF:
    case AV_CODEC_ID_DSD_MSBF:
    case AV_CODEC_ID_DSD_LSBF_PLANAR:
    case AV_CODEC_ID_DSD_MSBF_PLANAR: return "dsd";
    case AV_CODEC_ID_PCM_U8:
    case AV_CODEC_ID_PCM_S8:
    case AV_CODEC_ID_PCM_S16LE:
    case AV_CODEC_ID_PCM_S16BE:
    case AV_CODEC_ID_PCM_S24LE:
    case AV_CODEC_ID_PCM_S24BE:
    case AV_CODEC_ID_PCM_S32LE:
    case AV_CODEC_ID_PCM_S32BE:
    case AV_CODEC_ID_PCM_F32LE:
    case AV_CODEC_ID_PCM_F32BE:
    case AV_CODEC_ID_PCM_F64LE:
    case AV_CODEC_ID_PCM_F64BE:
        return "pcm";
    default:
        return "audio";
    }
}

uint32_t EncodedBitsPerSample(const AVCodecParameters* codecpar)
{
    if (codecpar == nullptr) {
        return 0;
    }

    switch (codecpar->codec_id)
    {
    case AV_CODEC_ID_DSD_LSBF:
    case AV_CODEC_ID_DSD_MSBF:
    case AV_CODEC_ID_DSD_LSBF_PLANAR:
    case AV_CODEC_ID_DSD_MSBF_PLANAR:
        return 1;
    default:
        break;
    }

    if (codecpar->bits_per_raw_sample > 0) {
        return static_cast<uint32_t>(codecpar->bits_per_raw_sample);
    }
    if (codecpar->bits_per_coded_sample > 0) {
        return static_cast<uint32_t>(codecpar->bits_per_coded_sample);
    }
    return 0;
}

std::string FormatBitRateBrief(int64_t bitRate)
{
    if (bitRate <= 0) {
        return std::string();
    }

    constexpr int64_t ONE_K = 1000;
    constexpr int64_t ONE_M = 1000 * 1000;
    constexpr int64_t USE_M_THRESHOLD = 10000 * ONE_K;
    if (bitRate > USE_M_THRESHOLD)
    {
        const int64_t whole = bitRate / ONE_M;
        const int64_t frac = (bitRate % ONE_M) / 100000;
        if (frac == 0) {
            return std::format("{}M", whole);
        }
        return std::format("{}.{}M", whole, frac);
    }
    if (bitRate >= ONE_K)
    {
        return std::format("{}k", bitRate / ONE_K);
    }

    return std::format("{}", bitRate);
}

} // namespace

FfmpegPlayCtrl::FfmpegPlayCtrl()
    : m_stream(nullptr)
    , m_fmtCtx(nullptr)
    , m_codecCtx(nullptr)
    , m_codec(nullptr)
    , m_ioCtx(nullptr)
    , m_ioBuffer(nullptr)
    , m_packet(nullptr)
    , m_frame(nullptr)
    , m_swrCtx(nullptr)
    , m_audioStreamIdx()
    , m_pendingPcm()
    , m_pluginConfig({})
    , m_srcAudioFmt({})
    , m_lastOutFmt({})
    , m_hasOutFmt(false)
    , m_needPack24(false)
    , m_needSigned8(false)
    , m_totalFrames(0)
    , m_curFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
    , m_eofSent(false)
    , m_decoderEnded(false)
{
    InitFfmpegLogFilter();
    InitEmptyAudioFormat(&m_srcAudioFmt);
    InitEmptyAudioFormat(&m_lastOutFmt);
}

FfmpegPlayCtrl::~FfmpegPlayCtrl()
{
    Release();
}

bool FfmpegPlayCtrl::Init(CDataStream* stream, uint32_t streamFmt, const PluginConfig& config)
{
    Release();
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;
    m_pluginConfig = config;

    m_fmtCtx = avformat_alloc_context();
    if (m_fmtCtx == nullptr) {
        Release();
        return false;
    }

    m_ioBuffer = static_cast<uint8_t*>(av_malloc(IO_BUFFER_SIZE));
    if (m_ioBuffer == nullptr) {
        Release();
        return false;
    }

    m_ioCtx = avio_alloc_context(m_ioBuffer, IO_BUFFER_SIZE, 0, this, &FfmpegPlayCtrl::ReadPacket, nullptr, &FfmpegPlayCtrl::SeekPacket);
    if (m_ioCtx == nullptr) {
        Release();
        return false;
    }

    m_fmtCtx->pb = m_ioCtx;
    m_fmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_open_input(&m_fmtCtx, nullptr, nullptr, nullptr) < 0) {
        Release();
        return false;
    }
    if (avformat_find_stream_info(m_fmtCtx, nullptr) < 0) {
        Release();
        return false;
    }

    if (!BuildAudioStreams()) {
        Release();
        return false;
    }

    if (m_streamFmt == StreamFormatUnknown) {
        int codecId = AV_CODEC_ID_NONE;
        const AVStream* stream0 = m_fmtCtx->streams[m_audioStreamIdx[0]];
        if (stream0 != nullptr && stream0->codecpar != nullptr) {
            codecId = stream0->codecpar->codec_id;
        }
        const char* fmtName = (m_fmtCtx->iformat == nullptr) ? nullptr : m_fmtCtx->iformat->name;
        m_streamFmt = StreamFormatFromFfmpeg(fmtName, nullptr, codecId);
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if ((m_packet == nullptr) || (m_frame == nullptr)) {
        Release();
        return false;
    }

    if (!OpenStream(0)) {
        Release();
        return false;
    }

    StopStream();
    return true;
}

void FfmpegPlayCtrl::Release()
{
    if (m_swrCtx != nullptr) {
        swr_free(&m_swrCtx);
    }

    if (m_codecCtx != nullptr) {
        avcodec_free_context(&m_codecCtx);
    }

    if (m_packet != nullptr) {
        av_packet_free(&m_packet);
    }

    if (m_frame != nullptr) {
        av_frame_free(&m_frame);
    }

    if (m_fmtCtx != nullptr) {
        avformat_close_input(&m_fmtCtx);
    }

    if (m_ioCtx != nullptr) {
        av_freep(&m_ioCtx->buffer);
        avio_context_free(&m_ioCtx);
    }
    m_ioBuffer = nullptr;

    m_stream = nullptr;
    m_codec = nullptr;
    m_audioStreamIdx.clear();
    m_pendingPcm.clear();
    InitEmptyAudioFormat(&m_srcAudioFmt);
    InitEmptyAudioFormat(&m_lastOutFmt);
    m_hasOutFmt = false;
    m_needPack24 = false;
    m_needSigned8 = false;
    m_totalFrames = 0;
    m_curFrames = 0;
    m_streamFmt = StreamFormatUnknown;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    m_opened = false;
    m_eofSent = false;
    m_decoderEnded = false;
}

bool FfmpegPlayCtrl::OpenStream(uint32_t streamIndex)
{
    if (m_audioStreamIdx.empty() || (streamIndex >= m_audioStreamIdx.size() && streamIndex != static_cast<uint32_t>(-1))) {
        return false;
    }

    const uint32_t openIndex = (streamIndex == static_cast<uint32_t>(-1)) ? 0u : streamIndex;
    m_activeStreamIdx = openIndex;

    if (!OpenCodecForActiveStream()) {
        return false;
    }

    m_opened = true;
    FlushDecodeState();
    m_curFrames = 0;
    return true;
}

void FfmpegPlayCtrl::StopStream()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool FfmpegPlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
{
    if (audioFmt == nullptr) {
        return true;
    }
    if (audioFmt->sampleRate == 0 || audioFmt->numChannels == 0) {
        return false;
    }
    return IsInterleavedPcm(audioFmt->format);
}

bool FfmpegPlayCtrl::IsCanSeeking() const
{
    return (m_fmtCtx != nullptr) && ((m_stream->GetStyle() & dsStyleSeekable) != 0);
}

uint32_t FfmpegPlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((m_codecCtx == nullptr) || (!m_opened) || (pBuf == nullptr) || (frames == 0)) {
        return 0;
    }

    AudioFormat outFmt;
    if (!ResolveOutputFormat(audioFmt, outFmt)) {
        return 0;
    }
    if (!PrepareSwr(&outFmt)) {
        return 0;
    }

    const uint32_t bytesPerFrame = outFmt.blockAlign;
    const size_t targetBytes = static_cast<size_t>(frames) * bytesPerFrame;
    while ((m_pendingPcm.size() < targetBytes) && !m_decoderEnded)
    {
        if (!DecodeMore(&outFmt)) {
            break;
        }
    }

    const size_t copyBytes = std::min(targetBytes, m_pendingPcm.size());
    if (copyBytes == 0) {
        return 0;
    }

    std::memcpy(pBuf, m_pendingPcm.data(), copyBytes);
    m_pendingPcm.erase(m_pendingPcm.begin(), m_pendingPcm.begin() + static_cast<std::ptrdiff_t>(copyBytes));

    const uint32_t decodedFrames = static_cast<uint32_t>(copyBytes / bytesPerFrame);
    m_curFrames += decodedFrames;
    return decodedFrames;
}

void FfmpegPlayCtrl::Seek(std::size_t frames)
{
    if (!IsCanSeeking() || m_activeStreamIdx == static_cast<uint32_t>(-1)) {
        return;
    }

    const int realIdx = m_audioStreamIdx[m_activeStreamIdx];
    AVStream* stream = m_fmtCtx->streams[realIdx];
    if (stream == nullptr || stream->time_base.den == 0) {
        return;
    }

    const int64_t targetTs = av_rescale_q(static_cast<int64_t>(frames), AVRational{ 1, static_cast<int>(std::max<uint32_t>(1, m_srcAudioFmt.sampleRate)) }, stream->time_base);
    if (av_seek_frame(m_fmtCtx, realIdx, targetTs, AVSEEK_FLAG_BACKWARD) < 0) {
        return;
    }

    avcodec_flush_buffers(m_codecCtx);
    FlushDecodeState();
    m_curFrames = frames;
}

float FfmpegPlayCtrl::GetDurationSeconds() const
{
    if (m_srcAudioFmt.sampleRate == 0 || m_totalFrames == 0) {
        return 0.0f;
    }
    return static_cast<float>(static_cast<double>(m_totalFrames) / m_srcAudioFmt.sampleRate);
}

void FfmpegPlayCtrl::FillMetaTags(CMediaTag& tags) const
{
    tags.Clear();

    const AVStream* audioStream = nullptr;
    if ((m_fmtCtx != nullptr) && !m_audioStreamIdx.empty()) {
        uint32_t useIndex = (m_activeStreamIdx == static_cast<uint32_t>(-1)) ? 0 : m_activeStreamIdx;
        useIndex = std::min<uint32_t>(useIndex, static_cast<uint32_t>(m_audioStreamIdx.size() - 1));
        audioStream = m_fmtCtx->streams[m_audioStreamIdx[useIndex]];
    }

    const std::string type = FfmpegFormatName(m_streamFmt);
    const std::string containerName = type.empty() ? "ffmpeg" : std::string(type);

    AVCodecID codecId = AV_CODEC_ID_NONE;
    uint32_t channels = m_srcAudioFmt.numChannels;
    uint32_t chLayout = m_srcAudioFmt.chLayout;
    uint32_t bitsPerSample = 0;
    int64_t bitRate = 0;
    if ((audioStream != nullptr) && (audioStream->codecpar != nullptr)) {
        codecId = audioStream->codecpar->codec_id;
        if (audioStream->codecpar->ch_layout.nb_channels > 0) {
            channels = static_cast<uint32_t>(audioStream->codecpar->ch_layout.nb_channels);
        }
        if (audioStream->codecpar->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC) {
            chLayout = static_cast<uint32_t>(audioStream->codecpar->ch_layout.u.mask);
        }
        bitsPerSample = EncodedBitsPerSample(audioStream->codecpar);
        if (audioStream->codecpar->bit_rate > 0) {
            bitRate = audioStream->codecpar->bit_rate;
        }
    }
    if (bitRate <= 0 && m_fmtCtx != nullptr && m_fmtCtx->bit_rate > 0) {
        bitRate = m_fmtCtx->bit_rate;
    }

    std::string brief = std::format("{}-{}", ToLowerAscii(containerName), CodecDisplayName(codecId));
    if (bitsPerSample > 0) {
        brief += std::format(" {}bits", bitsPerSample);
    }
    if (bitRate > 0) {
        brief += std::format(" {}", FormatBitRateBrief(bitRate));
    }
    brief += std::format(" {}", ChannelBrifName(channels, chLayout));

    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    tags.AddTagInteger(MediaTag_Streams, static_cast<int32_t>(m_audioStreamIdx.size()));
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());

    tags.AddTagInteger(MediaTag_SamplesRate, static_cast<int32_t>(m_srcAudioFmt.sampleRate));
    tags.AddTagInteger(MediaTag_Channels, static_cast<int32_t>(channels));
    tags.AddTagInteger(MediaTag_ChannelsLayout, static_cast<int32_t>(chLayout));
    if (bitsPerSample > 0) {
        tags.AddTagInteger(MediaTag_BitsPerSample, static_cast<int32_t>(bitsPerSample));
    }
    tags.AddTagString(MediaTag_PcmFormat, StringFromAudioFormat(m_srcAudioFmt.format));
    if (bitRate > 0) {
        tags.AddTagInteger(MediaTag_BitsRate, static_cast<int32_t>(bitRate));
    }

    if (m_fmtCtx != nullptr) {
        tags.AddTagString(MediaTag_Title, DictValue(m_fmtCtx->metadata, "title"));
        tags.AddTagString(MediaTag_Artists, DictValue(m_fmtCtx->metadata, "artist"));
        tags.AddTagString(MediaTag_Album, DictValue(m_fmtCtx->metadata, "album"));
        tags.AddTagString(MediaTag_Genre, DictValue(m_fmtCtx->metadata, "genre"));
        tags.AddTagString(MediaTag_Comment, DictValue(m_fmtCtx->metadata, "comment"));
        tags.AddTagString(MediaTag_Date, DictValue(m_fmtCtx->metadata, "date"));
    }
}

int FfmpegPlayCtrl::ReadPacket(void* opaque, uint8_t* buf, int bufSize)
{
    FfmpegPlayCtrl* self = static_cast<FfmpegPlayCtrl*>(opaque);
    if (self == nullptr || self->m_stream == nullptr || buf == nullptr || bufSize <= 0) {
        return AVERROR_EOF;
    }

    const uint32_t once = self->m_stream->Read(buf, static_cast<uint32_t>(bufSize));
    if (once == 0) {
        return AVERROR_EOF;
    }
    return static_cast<int>(once);
}

int64_t FfmpegPlayCtrl::SeekPacket(void* opaque, int64_t offset, int whence)
{
    FfmpegPlayCtrl* self = static_cast<FfmpegPlayCtrl*>(opaque);
    if (self == nullptr || self->m_stream == nullptr) {
        return -1;
    }

    if (whence == AVSEEK_SIZE) {
        return static_cast<int64_t>(self->m_stream->GetLength());
    }

    SeekBase base = SeekBase::Begin;
    switch (whence)
    {
    case SEEK_SET:
        base = SeekBase::Begin;
        break;
    case SEEK_CUR:
        base = SeekBase::Cur;
        break;
    case SEEK_END:
        base = SeekBase::End;
        break;
    default:
        return -1;
    }

    self->m_stream->Seek(base, offset);
    return static_cast<int64_t>(self->m_stream->Tell());
}

bool FfmpegPlayCtrl::BuildAudioStreams()
{
    m_audioStreamIdx.clear();
    for (unsigned int i = 0; i < m_fmtCtx->nb_streams; ++i)
    {
        AVStream* stream = m_fmtCtx->streams[i];
        if (stream != nullptr && stream->codecpar != nullptr && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIdx.push_back(static_cast<int>(i));
        }
    }
    return !m_audioStreamIdx.empty();
}

bool FfmpegPlayCtrl::OpenCodecForActiveStream()
{
    if (m_activeStreamIdx >= m_audioStreamIdx.size()) {
        return false;
    }

    if (m_swrCtx != nullptr) {
        swr_free(&m_swrCtx);
    }
    if (m_codecCtx != nullptr) {
        avcodec_free_context(&m_codecCtx);
    }

    const int realIdx = m_audioStreamIdx[m_activeStreamIdx];
    AVStream* stream = m_fmtCtx->streams[realIdx];
    if (stream == nullptr || stream->codecpar == nullptr) {
        return false;
    }

    m_codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (m_codec == nullptr) {
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(m_codec);
    if (m_codecCtx == nullptr) {
        return false;
    }
    if (avcodec_parameters_to_context(m_codecCtx, stream->codecpar) < 0) {
        return false;
    }
    if (avcodec_open2(m_codecCtx, m_codec, nullptr) < 0) {
        return false;
    }

    const uint32_t channels = static_cast<uint32_t>(std::max(1, m_codecCtx->ch_layout.nb_channels));
    const uint32_t sampleRate = static_cast<uint32_t>(std::max(1, m_codecCtx->sample_rate));
    AudioDataFormat srcFmt = AudioDataFormatFromFfmpegCodec(stream->codecpar->codec_id);
    if (srcFmt == AudioDataFormat::UNKNOWN) {
        srcFmt = AudioDataFormat::Float32;
    }
    uint32_t bits = GetBitsPerSampleByFormat(srcFmt);
    if (bits == 0) {
        bits = 32;
    }
    InitAudioFormat(&m_srcAudioFmt, srcFmt, channels, sampleRate, bits);

    m_totalFrames = 0;
    if (stream->duration > 0 && stream->time_base.den > 0) {
        m_totalFrames = static_cast<std::size_t>(av_rescale_q(stream->duration, stream->time_base, AVRational{ 1, static_cast<int>(sampleRate) }));
    }

    return true;
}

bool FfmpegPlayCtrl::PrepareSwr(const AudioFormat* outFmt)
{
    if (outFmt == nullptr || m_codecCtx == nullptr) {
        return false;
    }

    if (m_hasOutFmt && IsSameAudioFormat(&m_lastOutFmt, outFmt) && m_swrCtx != nullptr) {
        return true;
    }

    if (m_swrCtx != nullptr) {
        swr_free(&m_swrCtx);
    }

    m_needPack24 = false;
    m_needSigned8 = false;
    AVSampleFormat outSampleFmt = ToAvSampleFormat(outFmt->format, m_needPack24, m_needSigned8);
    if (outSampleFmt == AV_SAMPLE_FMT_NONE) {
        return false;
    }

    AVChannelLayout outChLayout;
    AVChannelLayout inChLayout;
    av_channel_layout_default(&outChLayout, static_cast<int>(outFmt->numChannels));
    if (m_codecCtx->ch_layout.nb_channels > 0) {
        av_channel_layout_copy(&inChLayout, &m_codecCtx->ch_layout);
    }
    else {
        av_channel_layout_default(&inChLayout, std::max(1u, m_srcAudioFmt.numChannels));
    }

    const int ret = swr_alloc_set_opts2(
        &m_swrCtx,
        &outChLayout,
        outSampleFmt,
        static_cast<int>(outFmt->sampleRate),
        &inChLayout,
        m_codecCtx->sample_fmt,
        std::max(1, m_codecCtx->sample_rate),
        0,
        nullptr);

    av_channel_layout_uninit(&outChLayout);
    av_channel_layout_uninit(&inChLayout);

    if (ret < 0 || m_swrCtx == nullptr) {
        return false;
    }
    if (swr_init(m_swrCtx) < 0) {
        return false;
    }

    m_lastOutFmt = *outFmt;
    m_hasOutFmt = true;
    m_pendingPcm.clear();
    return true;
}

bool FfmpegPlayCtrl::DecodeMore(const AudioFormat* outFmt)
{
    if (m_codecCtx == nullptr || m_fmtCtx == nullptr || m_swrCtx == nullptr || outFmt == nullptr) {
        return false;
    }

    while (true)
    {
        while (true)
        {
            int ret = avcodec_receive_frame(m_codecCtx, m_frame);
            if (ret == AVERROR(EAGAIN)) {
                break;
            }
            if (ret == AVERROR_EOF) {
                m_decoderEnded = true;
                return false;
            }
            if (ret < 0) {
                return false;
            }

            const int outSamples = static_cast<int>(av_rescale_rnd(
                swr_get_delay(m_swrCtx, std::max(1, m_codecCtx->sample_rate)) + m_frame->nb_samples,
                static_cast<int64_t>(outFmt->sampleRate),
                std::max(1, m_codecCtx->sample_rate),
                AV_ROUND_UP));
            if (outSamples <= 0) {
                av_frame_unref(m_frame);
                continue;
            }

            uint8_t* outData[AV_NUM_DATA_POINTERS] = {};
            int outLineSize = 0;
            bool needPack24 = m_needPack24;
            bool needSigned8 = m_needSigned8;
            AVSampleFormat outSampleFmt = ToAvSampleFormat(outFmt->format, needPack24, needSigned8);
            if (av_samples_alloc(outData, &outLineSize, static_cast<int>(outFmt->numChannels), outSamples, outSampleFmt, 0) < 0) {
                av_frame_unref(m_frame);
                return false;
            }

            const int converted = swr_convert(m_swrCtx, outData, outSamples, const_cast<const uint8_t**>(m_frame->data), m_frame->nb_samples);
            av_frame_unref(m_frame);
            if (converted <= 0) {
                av_freep(&outData[0]);
                continue;
            }

            if (needPack24)
            {
                const int32_t* s32 = reinterpret_cast<const int32_t*>(outData[0]);
                const size_t sampleCount = static_cast<size_t>(converted) * outFmt->numChannels;
                const size_t oldSize = m_pendingPcm.size();
                m_pendingPcm.resize(oldSize + sampleCount * 3);
                uint8_t* dst = m_pendingPcm.data() + oldSize;
                for (size_t i = 0; i < sampleCount; ++i)
                {
                    const int32_t v = s32[i] >> 8;
                    dst[i * 3 + 0] = static_cast<uint8_t>(v & 0xFF);
                    dst[i * 3 + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
                    dst[i * 3 + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
                }
            }
            else
            {
                const size_t dataBytes = static_cast<size_t>(converted) * outFmt->blockAlign;
                const size_t oldSize = m_pendingPcm.size();
                m_pendingPcm.resize(oldSize + dataBytes);
                if (needSigned8)
                {
                    const uint8_t* src = outData[0];
                    uint8_t* dst = m_pendingPcm.data() + oldSize;
                    for (size_t i = 0; i < dataBytes; ++i)
                    {
                        dst[i] = static_cast<uint8_t>(src[i] ^ 0x80);
                    }
                }
                else
                {
                    std::memcpy(m_pendingPcm.data() + oldSize, outData[0], dataBytes);
                }
            }

            av_freep(&outData[0]);
            return true;
        }

        if (!m_eofSent)
        {
            if (av_read_frame(m_fmtCtx, m_packet) >= 0)
            {
                const int realIdx = m_audioStreamIdx[m_activeStreamIdx];
                if (m_packet->stream_index == realIdx) {
                    avcodec_send_packet(m_codecCtx, m_packet);
                }
                av_packet_unref(m_packet);
                continue;
            }

            avcodec_send_packet(m_codecCtx, nullptr);
            m_eofSent = true;
            continue;
        }

        m_decoderEnded = true;
        return false;
    }
}

void FfmpegPlayCtrl::FlushDecodeState()
{
    m_pendingPcm.clear();
    m_eofSent = false;
    m_decoderEnded = false;
    if (m_packet != nullptr) {
        av_packet_unref(m_packet);
    }
    if (m_frame != nullptr) {
        av_frame_unref(m_frame);
    }
}

bool FfmpegPlayCtrl::ResolveOutputFormat(const AudioFormat* requestFmt, AudioFormat& outFmt) const
{
    if (requestFmt != nullptr) {
        if (!IsSupportOutput(requestFmt)) {
            return false;
        }
        outFmt = *requestFmt;
        return true;
    }

    uint32_t sampleRate = (m_pluginConfig.DefaultSampleRate == 0) ? m_srcAudioFmt.sampleRate : m_pluginConfig.DefaultSampleRate;
    if (sampleRate == 0) {
        sampleRate = 48000;
    }
    uint32_t channels = (m_pluginConfig.DefaultChannels == 0) ? m_srcAudioFmt.numChannels : m_pluginConfig.DefaultChannels;
    if (channels == 0) {
        channels = 2;
    }

    AudioDataFormat fmt = static_cast<AudioDataFormat>(m_pluginConfig.DefaultPcmFormat);
    if (!IsInterleavedPcm(fmt)) {
        fmt = AudioDataFormat::Float32;
    }

    uint32_t bits = m_pluginConfig.DefaultBitsPerSample;
    if (bits == 0) {
        bits = GetBitsPerSampleByFormat(fmt);
    }

    InitAudioFormat(&outFmt, fmt, channels, sampleRate, bits);
    return true;
}

bool FfmpegPlayCtrl::IsInterleavedPcm(AudioDataFormat fmt)
{
    switch (fmt)
    {
    case AudioDataFormat::PCM_U8:
    case AudioDataFormat::PCM_S8:
    case AudioDataFormat::PCM_S16:
    case AudioDataFormat::PCM_S24:
    case AudioDataFormat::PCM_S24_32:
    case AudioDataFormat::PCM_S32:
    case AudioDataFormat::PCM_64:
    case AudioDataFormat::Float32:
    case AudioDataFormat::Float64:
        return true;
    default:
        return false;
    }
}

AVSampleFormat FfmpegPlayCtrl::ToAvSampleFormat(AudioDataFormat fmt, bool& needPack24, bool& needSigned8)
{
    needPack24 = false;
    needSigned8 = false;
    switch (fmt)
    {
    case AudioDataFormat::PCM_U8: return AV_SAMPLE_FMT_U8;
    case AudioDataFormat::PCM_S8:
        needSigned8 = true;
        return AV_SAMPLE_FMT_U8;
    case AudioDataFormat::PCM_S16: return AV_SAMPLE_FMT_S16;
    case AudioDataFormat::PCM_S24:
        needPack24 = true;
        return AV_SAMPLE_FMT_S32;
    case AudioDataFormat::PCM_S24_32:
    case AudioDataFormat::PCM_S32: return AV_SAMPLE_FMT_S32;
    case AudioDataFormat::PCM_64: return AV_SAMPLE_FMT_S64;
    case AudioDataFormat::Float32: return AV_SAMPLE_FMT_FLT;
    case AudioDataFormat::Float64: return AV_SAMPLE_FMT_DBL;
    default: return AV_SAMPLE_FMT_NONE;
    }
}

uint32_t FfmpegPlayCtrl::BytesPerSample(AudioDataFormat fmt)
{
    switch (fmt)
    {
    case AudioDataFormat::PCM_U8:
    case AudioDataFormat::PCM_S8: return 1;
    case AudioDataFormat::PCM_S16: return 2;
    case AudioDataFormat::PCM_S24: return 3;
    case AudioDataFormat::PCM_S24_32:
    case AudioDataFormat::PCM_S32:
    case AudioDataFormat::Float32: return 4;
    case AudioDataFormat::PCM_64:
    case AudioDataFormat::Float64: return 8;
    default: return 0;
    }
}
