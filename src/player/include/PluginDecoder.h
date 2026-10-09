/*
20240425 初次生成
大模型：GPT 5.3 Codex
任务说明：todo_task_23.txt

20240425 增加 Name 静态函数，增加 m_decodeMtx 互斥体
大模型：GPT 5.3 Codex
任务说明：todo_task_24.txt
*/
#pragma once

#include <memory>
#include <mutex>
#include "AudioDecoder.h"
#include "DecoderDllWrapper.h"

class CPluginDecoder final : public CAudioDecoder
{
public:
    explicit CPluginDecoder(uint32_t streamFmt);
    ~CPluginDecoder() override;

    static std::string Name();

    void AttachModule(const std::shared_ptr<CDecoderDllWrapper>& dll);

    bool InitDecode(const CDecodeInitCtx* decodeInit) override;
    bool StartStream(uint32_t streamIdx, std::size_t begin, std::size_t end, uint32_t loop) override;
    void StopStream(uint32_t streamIdx) override;
    CMediaTag GetTags(uint32_t streamIdx) override;
    AudioFormat GetAudioFormat(uint32_t streamIdx) const override;
    uint32_t Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt) override;
    void SeekTo(std::size_t frames) override;
    std::size_t GetCurrentFrame() const override { return m_curFrames; }
    std::size_t GetTotalFrame() const override { return m_totalFrames; }
    bool IsSupportOutput(const AudioFormat* audioFmt) const override;
    bool IsCanSeeking(uint32_t streamIdx) const override;
    void Reset() override;

private:
    void UninitializePlugin();
    void ThrowPluginError(const char* action, int code) const;
    void UpdateStatus(uint32_t flags);

private:
    mutable std::mutex m_decodeMtx;
    std::shared_ptr<CDecoderDllWrapper> m_dll;
    void* m_ctxHdr;
    uint32_t m_streamFmt;
    AudioFormat m_audioFmt;
    std::size_t m_curFrames;
    std::size_t m_totalFrames;
};
