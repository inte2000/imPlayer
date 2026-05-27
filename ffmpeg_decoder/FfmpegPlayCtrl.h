/*
20260523 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_53.txt

修改记录：
大模型：ChatGPT 5.3 Codex
todo_task_54.txt
todo_task_57.txt
todo_task_58.txt
todo_task_59.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "DataStream.h"
#include "AudioInfo.h"
#include "MediaTag.h"
#include "PluginConfig.h"

class FfmpegPlayCtrl
{
public:
    FfmpegPlayCtrl();
    ~FfmpegPlayCtrl();

    bool Init(CDataStream* stream, uint32_t streamFmt, const PluginConfig& config);
    void Release();

    bool OpenStream(uint32_t streamIndex);
    void StopStream();

    bool IsSupportOutput(const AudioFormat* audioFmt) const;
    bool IsCanSeeking() const;

    uint32_t DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt);
    void Seek(std::size_t frames);

    std::size_t GetCurrentFrame() const { return m_curFrames; }
    std::size_t GetTotalFrame() const { return m_totalFrames; }
    float GetDurationSeconds() const;
    uint32_t GetStreamCount() const { return static_cast<uint32_t>(m_audioStreamIdx.size()); }
    uint32_t GetActiveStreamIndex() const { return m_activeStreamIdx; }
    const AudioFormat& GetSourceAudioFormat() const { return m_srcAudioFmt; }
    void FillMetaTags(CMediaTag& tags) const;

private:
    static int ReadPacket(void* opaque, uint8_t* buf, int bufSize);
    static int64_t SeekPacket(void* opaque, int64_t offset, int whence);

    bool BuildAudioStreams();
    bool OpenCodecForActiveStream();
    bool PrepareSwr(const AudioFormat* outFmt);
    bool DecodeMore(const AudioFormat* outFmt);
    void FlushDecodeState();
    bool ResolveOutputFormat(const AudioFormat* requestFmt, AudioFormat& outFmt) const;

    static bool IsInterleavedPcm(AudioDataFormat fmt);
    static AVSampleFormat ToAvSampleFormat(AudioDataFormat fmt, bool& needPack24, bool& needSigned8);
    static uint32_t BytesPerSample(AudioDataFormat fmt);

private:
    CDataStream* m_stream;
    AVFormatContext* m_fmtCtx;
    AVCodecContext* m_codecCtx;
    const AVCodec* m_codec;
    AVIOContext* m_ioCtx;
    uint8_t* m_ioBuffer;
    AVPacket* m_packet;
    AVFrame* m_frame;
    SwrContext* m_swrCtx;

    std::vector<int> m_audioStreamIdx;
    std::vector<uint8_t> m_pendingPcm;

    PluginConfig m_pluginConfig;
    AudioFormat m_srcAudioFmt;
    AudioFormat m_lastOutFmt;
    bool m_hasOutFmt;
    bool m_needPack24;
    bool m_needSigned8;

    std::size_t m_totalFrames;
    std::size_t m_curFrames;
    uint32_t m_streamFmt;
    uint32_t m_activeStreamIdx;
    bool m_opened;
    bool m_eofSent;
    bool m_decoderEnded;
};
