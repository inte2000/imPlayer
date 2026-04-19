#ifndef MP3_AUDIO_ENCODER_H
#define MP3_AUDIO_ENCODER_H

#include <string>
#include <lame/lame.h>
#include "AudioEncoder.h"


class CMp3Encoder : public CAudioEncoder
{
public:
	CMp3Encoder(uint32_t streamFmt);
	~CMp3Encoder() override;

	bool Init(const std::string& jsonParams) override;
	void SetMetaInfo(const std::string& jsonMeta) override;
	std::vector<EncoderParamterDef> QueryParamtersDefine() const override;
	uint32_t Encode(const void* pData, uint32_t frames, const AudioFormat* audioFmt) override;
	bool Flush() override;
	bool IsSupportFormat(uint32_t mediaFmt) const override;
protected:
	void SetId3Tags(lame_global_flags* lame, const MediaBaseMetaInfo& metaInfo);
private:
	uint32_t m_streamFmt;
	lame_global_flags* m_lame;
	uint32_t m_channels;
	uint32_t m_sampleRates;
	std::unique_ptr<unsigned char[]> m_mp3Buf;
	uint32_t m_mp3BufSize;
	MediaBaseMetaInfo m_metaInfo;
};


#endif //MP3_AUDIO_ENCODER_H
