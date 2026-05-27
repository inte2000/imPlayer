/*
20260527 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_84.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <wavpack/wavpack.h>

#include "DataStream.h"
#include "AudioInfo.h"
#include "MediaTag.h"

class WavpackPlayCtrl
{
public:
    WavpackPlayCtrl();
    ~WavpackPlayCtrl();

    bool Init(CDataStream* stream, uint32_t streamFmt);
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
    uint32_t GetStreamCount() const { return 1; }
    uint32_t GetActiveStreamIndex() const { return m_activeStreamIdx; }
    const AudioFormat& GetSourceAudioFormat() const { return m_srcAudioFmt; }
    void FillMetaTags(CMediaTag& tags) const;

private:
    static int32_t ReadBytes(void* id, void* data, int32_t bcount);
    static int32_t WriteBytes(void* id, void* data, int32_t bcount);
    static int64_t GetPos(void* id);
    static int SetPosAbs(void* id, int64_t pos);
    static int SetPosRel(void* id, int64_t delta, int mode);
    static int PushBackByte(void* id, int c);
    static int64_t GetLength(void* id);
    static int CanSeek(void* id);
    static int TruncateHere(void* id);
    static int CloseReader(void* id);

    std::string ReadTagValue(const char* key) const;

private:
    CDataStream* m_stream;
    WavpackContext* m_ctx;
    WavpackStreamReader64 m_reader;
    AudioFormat m_srcAudioFmt;
    std::size_t m_curFrames;
    std::size_t m_totalFrames;
    uint32_t m_streamFmt;
    uint32_t m_activeStreamIdx;
    bool m_opened;
    uint32_t m_sourceBitsPerSample;
    std::vector<int32_t> m_tempS32;
};
