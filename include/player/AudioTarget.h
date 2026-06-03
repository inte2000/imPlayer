#ifndef AUDIO_TARGET_H
#define AUDIO_TARGET_H

#include <memory>
#include <string>
#include <vector>
#include "AudioInfo.h"
#include "DataStream.h"
#include "AudioEncoder.h"


class CAudioTarget
{
public:
    CAudioTarget();
    CAudioTarget(std::unique_ptr<CDataStream> stream, std::unique_ptr<CAudioEncoder> encoder);
    std::wstring GetName() const;
    bool InitEncoder(const std::vector<EncoderParamter>& params);
    void SetMetaInformation(const CMediaTag& metaInfo);
    const AudioFormat& GetAudioFormat() const { return m_devFmt; }
    uint32_t WriteBuffer(const uint8_t* buf, uint32_t frames, const AudioFormat& audioFmt);
    uint32_t FlushBuffer();
private:
    AudioFormat m_devFmt; 
    std::unique_ptr<CDataStream> m_stream;
    std::unique_ptr<CAudioEncoder> m_encoder;

};

std::unique_ptr<CAudioTarget> MakeFileAudioTarget(const std::wstring& filename, uint32_t streamFmt, const std::string& encoder);

#endif //AUDIO_TARGET_H
