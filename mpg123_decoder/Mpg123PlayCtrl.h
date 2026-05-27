/*
20260526 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_74.txt

修改记录：
大模型：ChatGPT 5.3 Codex
todo_task_75.txt
todo_task_77.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>

#include "AudioInfo.h"
#include "MediaTag.h"
#include "Mp3Handle.h"
#include "AudioConvert.h"

class Mpg123PlayCtrl
{
public:
    Mpg123PlayCtrl();
    ~Mpg123PlayCtrl();

    bool Init(CDataStream* stream, uint32_t streamFmt);
    void Release();

    bool OpenStream(uint32_t streamIndex);
    void StopStream();

    bool IsSupportOutput(const AudioFormat* audioFmt) const;
    bool IsCanSeeking() const;

    uint32_t DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt);
    void Seek(std::size_t frames);
    void Reset();
    
    std::size_t GetCurrentFrame() const { return m_curFrames; }
    std::size_t GetTotalFrame() const { return m_totalFrames; }
    float GetDurationSeconds() const;
    uint32_t GetStreamCount() const { return 1; }
    uint32_t GetActiveStreamIndex() const { return m_activeStreamIdx; }
    const AudioFormat& GetSourceAudioFormat() const { return m_srcAudioFmt; }
    void FillMetaTags(CMediaTag& tags) const;

private:
    void DecideBitsRate(Mp3ExtraInfo& extraInfo);

private:
    CDataStream* m_stream;
    CMp3Handle m_mp3;
    Mp3ExtraInfo m_extraInfo;
    AudioFormat m_srcAudioFmt;
    std::size_t m_curFrames;
    std::size_t m_totalFrames;
    uint32_t m_streamFmt;
    uint32_t m_activeStreamIdx;
    bool m_opened;
	std::unique_ptr<CAudioConverter> m_converter;
	std::unique_ptr<uint8_t[]> m_tempBuf;
	uint32_t m_tempBufSize;
};
