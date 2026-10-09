#ifndef _DECODER_PARAMETERS_H_
#define _DECODER_PARAMETERS_H_

#include <string>
#include <vector>
#include "DecodeInitCtx.h"

class CMidiDecoderInitCtx : public CDecodeInitCtx
{
public:
    CMidiDecoderInitCtx() = default;
    CMidiDecoderInitCtx(const std::string& backend, uint32_t sampleRate, float gain);
    
    uint32_t GetInitSampleRate() const override { return m_sampleRate; }
    AudioDataFormat GetInitDataFormat() const override { return AudioDataFormat::Float32; }
    uint32_t GetInitChannels() const override { return 2; }
    uint32_t GetInitChLayout() const override { return 0x03; }
    float GetGain() const override { return m_gain; }
    std::string GetMidiInstruments() const override { return m_soundfont; }
    std::string GetMidiBackendName() const override { return m_backendName; }
    uint32_t GetMultiChAreaMode() const override { return 0;}
private:
    std::string m_backendName;
    std::string m_soundfont;
    uint32_t m_sampleRate;
    float m_gain;
};

class CCDDecoderInitCtx : public CDecodeInitCtx
{
public:
    CCDDecoderInitCtx() = default;
    CCDDecoderInitCtx(uint32_t sacdSampleRate, bool bPreferMultiChArea);
    
    uint32_t GetInitSampleRate() const override { return m_sacdSampleRate; }
    AudioDataFormat GetInitDataFormat() const override { return AudioDataFormat::PCM_S16; }
    uint32_t GetInitChannels() const override { return 2; }
    uint32_t GetInitChLayout() const override { return 0x03; }
    float GetGain() const override { return 1.0f; }
    std::string GetMidiInstruments() const override { return ""; }
    std::string GetMidiBackendName() const override { return ""; }
    uint32_t GetMultiChAreaMode() const override { return m_bPreferMultiChArea ? 1 : 0;}
private:
    uint32_t m_sacdSampleRate;
    bool m_bPreferMultiChArea;
};

#endif //_DECODER_PARAMETERS_H_


