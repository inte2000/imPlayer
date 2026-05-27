/*
大模型：GPT 5.3 Codex
任务说明：todo_task_73.txt
*/
#ifndef WAV_AUDIO_DECODER_H
#define WAV_AUDIO_DECODER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "dr_wav.h"

#include "AudioDecoder.h"

class CWavDecoder : public CAudioDecoder
{
public:
    CWavDecoder(uint32_t streamFmt);
    ~CWavDecoder() override;

    bool InitDecode(const CDecodeInitCtx* decodeInit) override;
    bool StartStream(uint32_t streamIdx, std::size_t begin, std::size_t end, uint32_t loop) override;
    void StopStream(uint32_t streamIdx) override;
    CMediaTag GetTags(uint32_t streamIdx) override;
    AudioFormat GetAudioFormat(uint32_t streamIdx) const override
    {
        return m_AudioFmt;
    }
    uint32_t Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt) override;
    void SeekTo(std::size_t frames) override;
    std::size_t GetCurrentFrame() const override { return m_curFrames; }
    std::size_t GetTotalFrame() const override { return m_totalFrames; }
    bool IsSupportOutput(const AudioFormat* audioFmt) const override;
    bool IsCanSeeking(uint32_t streamIdx) const override;
    void Reset() override;

    static std::string Name();

private:
    static AudioDataFormat GuessOutputFormat(uint16_t translatedFormatTag, uint16_t bitsPerSample);
    static const char* TypeNameFromStreamFormat(uint32_t streamFmt);
    void MakeMediaTags(CMediaTag& tags);

private:
    uint32_t m_streamFmt;
    std::mutex m_decodeMtx;
    std::unique_ptr<drwav> m_wav;
    AudioFormat m_AudioFmt;
    std::size_t m_curFrames;
    std::size_t m_totalFrames;
    std::size_t m_fileTotalFrames;
};

uint32_t WavQueryFileType(const std::wstring& filename);

#endif // WAV_AUDIO_DECODER_H
