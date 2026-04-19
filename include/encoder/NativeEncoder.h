#ifndef NATIVE_AUDIO_ENCODER_H
#define NATIVE_AUDIO_ENCODER_H

#include <string>
#include "AudioEncoder.h"
#include <sndfile.h>

class CNativeEncoder : public CAudioEncoder
{
public:
	CNativeEncoder(uint32_t streamFmt);
	~CNativeEncoder() override;

	bool Init(const std::string& jsonParams) override;
	void SetMetaInfo(const std::string& jsonMeta) override;
	std::vector<EncoderParamterDef> QueryParamtersDefine() const override;
	uint32_t Encode(const void* pData, uint32_t frames, const AudioFormat* audioFmt) override;
	bool Flush() override { return true; }
	bool IsSupportFormat(uint32_t mediaFmt) const override;
protected:
	void AttachMetaInfo(SNDFILE* sf_file, const MediaBaseMetaInfo& metaInfo);
private:
	uint32_t m_streamFmt;
	EncodingParams m_encodingParam;
	SNDFILE* m_file;
	MediaBaseMetaInfo m_metaInfo;
};


#endif //NATIVE_AUDIO_ENCODER_H
