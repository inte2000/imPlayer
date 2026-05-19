/*
20260424 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_15.txt

20260424 修改 inline，引入 core.lib
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_16.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <sndfile.h>
#include "DataStream.h"
#include "AudioInfo.h"
#include "MediaTag.h"

class LibSndPlayCtrl
{
public:
    LibSndPlayCtrl();
    ~LibSndPlayCtrl();

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
    static sf_count_t GetLengthCb(void* userData);
    static sf_count_t SeekCb(sf_count_t offset, int whence, void* userData);
    static sf_count_t ReadCb(void* ptr, sf_count_t count, void* userData);
    static sf_count_t WriteCb(const void* ptr, sf_count_t count, void* userData);
    static sf_count_t TellCb(void* userData);

    static uint32_t BitsPerSampleFromSndSubtype(int format);

private:
    CDataStream* m_stream;
    SNDFILE* m_file;
    SF_INFO m_sfInfo;
    SF_VIRTUAL_IO m_vio;
    AudioFormat m_srcAudioFmt;
    std::size_t m_totalFrames;
    std::size_t m_curFrames;
    uint32_t m_streamFmt;
    uint32_t m_activeStreamIdx;
    bool m_opened;
};
