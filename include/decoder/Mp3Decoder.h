#ifndef MP3_AUDIO_DECODER_H
#define MP3_AUDIO_DECODER_H

#include <string>
#include <memory>
#include <mutex>
#include "AudioDecoder.h"
#include "Mp3Handle.h"
#include "AudioConvert.h"


class CMp3Decoder : public CAudioDecoder
{
public:
    CMp3Decoder(uint32_t streamFmt);

    bool InitDecode(const CDecodeInitCtx* decodeInit) override;
	bool StartStream(uint32_t streamIdx, std::size_t begin, std::size_t end, uint32_t loop) override;
	void StopStream(uint32_t streamIdx) override;
	CMediaTag GetTags(uint32_t streamIdx) override;
	AudioFormat GetAudioFormat(uint32_t streamIdx) const override {
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
protected:
	void DecideBitsRate(Mp3ExtraInfo& ei);
	void MakeMediaTags(CMediaTag& tags);

private:
	uint32_t m_streamFmt;
	std::mutex m_decodeMtx;
	CMp3Handle m_hMp3;
	Mp3ExtraInfo m_extraInfo;
	AudioFormat m_AudioFmt;
	std::size_t m_curFrames;
	std::size_t m_totalFrames;
	std::size_t m_dataOffset;
	std::unique_ptr<CAudioConverter> m_converter;
	std::unique_ptr<uint8_t[]> m_tempBuf;
	uint32_t m_tempBufSize;
};

uint32_t Mpg123QueryFileType(const std::wstring& filename);
bool IsMp3AudioFormat(const std::string& filename);


#endif //MP3_AUDIO_DECODER_H
