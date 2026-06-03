#ifndef WAV_AUDIO_ENCODER_H
#define WAV_AUDIO_ENCODER_H

#include <string>
#include <vector>

#include "AudioEncoder.h"
#include "encoder/EncoderFormatDefine.h"
#include "encoder/EncoderParamterDefine.h"

class CWavEncoder : public CAudioEncoder
{
public:
    explicit CWavEncoder(uint32_t streamFmt);
    ~CWavEncoder() override;

    bool Init(const std::vector<EncoderParamter>& params) override;
    AudioFormat GetTransFormat() const override;
    void SetMetaInfo(const CMediaTag& metaTags) override;
    uint32_t Encode(const void* pData, uint32_t frames, const AudioFormat* audioFmt) override;
    bool Flush() override;

    static std::string GetName();
    static std::vector<EncoderParamterDefine> GetParameterDefine();
    static std::vector<EncoderFormatDefine> GetFormatDefine();

private:
    static uint32_t ToWaveFormatTag(AudioDataFormat dataFmt, bool extWav);
    static bool IsSupportedDataFormat(AudioDataFormat dataFmt);

private:
    uint32_t m_streamFmt = 0;
    bool m_extWav = false;
    bool m_initialized = false;
    AudioFormat m_transFmt{};
    CMediaTag m_metaInfo;
    void* m_wav = nullptr;
};

#endif //WAV_AUDIO_ENCODER_H
