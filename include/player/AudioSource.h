#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include <memory>
#include <string>
#include "AudioInfo.h"
#include "DataStream.h"
#include "AudioDecoder.h"
#include "SoxEffects.h"

//const uint32_t DECODE_BUF_FRAMES = 16384;
constexpr uint32_t DECODE_BUF_FRAMES = 32 * 1024; //64k

class CAudioSource
{
public:
    CAudioSource();
    CAudioSource(std::unique_ptr<CDataStream> stream, std::unique_ptr<CAudioDecoder> decoder, uint32_t streamFmt);
    std::wstring GetName() const;
    const AudioFormat& GetAudioFormat() const { return m_audioInfo.m_audioFmt; }
    const AudioFormat& GetOutputFormat() const { return m_outFmt; }
    bool SetOutputFormat(const AudioFormat& outFmt, bool bDevide);
    const std::string& GetExtraInformation() const { return m_audioInfo.extraInfo; }
    uint32_t ReadBuffer(uint8_t* buf, uint32_t bufSize, uint32_t frames);
    float GetTotalSeconds() const {
        return float(m_audioInfo.m_totalFrames) / float(m_audioInfo.m_audioFmt.sampleRate);
    }
    std::size_t SourceFrameToDeviceFrame(std::size_t srcFrame) const;
    std::size_t DeviceFrameToSourceFrame(std::size_t devFrame) const;

    void SeekToFrame(std::size_t frames);
    std::size_t GetCurrentFrame() const;
    std::size_t GetTotalFrames() const;
    float FramesToSeconds(std::size_t frames) const;
    std::size_t SecondsToFrames(float seconds) const;

private:
    AudioInfo m_audioInfo;
    bool m_bDeviceOutput;
    AudioFormat m_outFmt;
    AudioFormat m_decodeFmt; 
    std::string m_name; // Audio source name, file name or net server name
    std::unique_ptr<CDataStream> m_stream;
    std::unique_ptr<CAudioDecoder> m_decoder;

    CSoxEffects m_Effects;
    bool m_needConvert;
    std::unique_ptr<uint8_t[]> m_tmpBuf; 
    uint32_t m_tmpSize;
};

std::unique_ptr<CAudioSource> MakeFileAudioSource(const std::wstring& filename);
std::unique_ptr<CAudioSource> MakeCDAudioSource(const std::wstring& deviceName, uint32_t track);


#endif //AUDIO_SOURCE_H
