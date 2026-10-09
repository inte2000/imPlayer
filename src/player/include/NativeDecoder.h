#ifndef NATIVE_AUDIO_DECODER_H
#define NATIVE_AUDIO_DECODER_H

#include <string>
#include <mutex>
#include "AudioDecoder.h"
#include <sndfile.h>

class CNativeDecoder : public CAudioDecoder
{
public:
	CNativeDecoder(uint32_t streamFmt);
	~CNativeDecoder() override;
    AudioInfo InitDecode(const CDecodeInitCtx* decodeInit) override;
    uint32_t Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt) override;
    void SeekTo(std::size_t frames) override;
	std::size_t GetCurrentFrame() const override { return m_curFrames; }
	std::size_t GetTotalFrame() const override { return m_totalFrames; }
	bool IsSupportOutput(const AudioFormat* audioFmt) const override;

private:
	uint32_t m_streamFmt;
	std::mutex m_decodeMtx;
	uint32_t m_NativeType;
	AudioFormat m_AudioFmt;
	std::size_t m_curFrames;
	std::size_t m_totalFrames;
	SNDFILE* m_file;
};

uint32_t LibSndfileQueryFileType(const std::wstring& filename);
bool IsNativeSupportAudioFormat(const std::string& filename);


#endif //NATIVE_AUDIO_DECODER_H
