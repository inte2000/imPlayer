#ifndef WAV_AUDIO_ENCODER_H
#define WAV_AUDIO_ENCODER_H

#include <string>
#include <vector>

#include "AudioEncoder.h"
#include "EncoderFormatDefine.h"
#include "EncoderParamterDefine.h"

class CWavEncoder : public CAudioEncoder
{
public:
    explicit CWavEncoder(uint32_t streamFmt);
    ~CWavEncoder() override;

    void Init() override;
    bool Open(const std::vector<EncoderParamter>& params) override;
    AudioFormat GetTransFormat() const override;
    bool SetMetaInfo(const CMediaTag& metaTags) override;
    bool Start(const AudioFormat* audioFmt) override;
    uint32_t Encode(const void* pData, uint32_t frames) override;
    bool Stop() override;
    void Close() override;

    static std::string GetName();
    static std::vector<EncoderParamterDefine> GetParameterDefine();
    static std::vector<EncoderFormatDefine> GetFormatDefine();

private:
    static uint32_t ToWaveFormatTag(AudioDataFormat dataFmt, bool extWav);
    static bool IsSupportedDataFormat(AudioDataFormat dataFmt);

private:
    uint32_t m_streamFmt = 0;
    bool m_extWav = false;
    bool m_opened = false;
    bool m_started = false;
    bool m_stopped = false;
    bool m_stopResult = true;
    AudioFormat m_transFmt{};
    CMediaTag m_metaInfo;
    void* m_wav = nullptr;
};

#endif //WAV_AUDIO_ENCODER_H
