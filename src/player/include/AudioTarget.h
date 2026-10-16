#ifndef AUDIO_TARGET_H
#define AUDIO_TARGET_H

#include <memory>
#include <optional>
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
    ~CAudioTarget();
    std::wstring GetName() const;
    bool Open(const std::vector<EncoderParamter>& params);
    bool Start(const AudioFormat& audioFmt);
    void Close();
    void SetMetaInformation(const CMediaTag& metaInfo);
    const AudioFormat& GetInputFormat() const { return m_inputFmt; }
    uint32_t WriteBuffer(const uint8_t* buf, uint32_t frames, const AudioFormat& audioFmt);
    uint32_t FlushBuffer();
private:
    AudioFormat m_inputFmt{};
    AudioFormat m_startedFmt{};
    bool m_opened = false;
    bool m_started = false;
    bool m_stopped = false;
    std::optional<CMediaTag> m_metaInfo;
    std::unique_ptr<CDataStream> m_stream;
    std::unique_ptr<CAudioEncoder> m_encoder;

};

std::unique_ptr<CAudioTarget> MakeFileAudioTarget(const std::wstring& filename, uint32_t streamFmt, const std::string& encoder);

#endif //AUDIO_TARGET_H
