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

    AudioInfo InitDecode(const CDecodeInitCtx* decodeInit) override;
    uint32_t Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt) override;
    void SeekTo(std::size_t frames) override;
    std::size_t GetCurrentFrame() const override { return m_curFrames; }
	std::size_t GetTotalFrame() const override { return m_totalFrames; }
	bool IsSupportOutput(const AudioFormat* audioFmt) const override;

protected:
	void DecideBitsRate(Mp3ExtraInfo& ei);

private:
	uint32_t m_streamFmt;
	std::mutex m_decodeMtx;
	CMp3Handle m_hMp3;
	AudioFormat m_AudioFmt;
	std::size_t m_curFrames;
	std::size_t m_totalFrames;
	std::size_t m_dataOffset;
	std::unique_ptr<CAudioConverter> m_converter;
	std::unique_ptr<uint8_t[]> m_tempBuf;
	uint32_t m_tempBufSize;
};

bool IsMp3AudioFormat(const std::string& filename);


#endif //MP3_AUDIO_DECODER_H
