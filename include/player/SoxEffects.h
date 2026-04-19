#ifndef SOX_EFFECTS_H
#define SOX_EFFECTS_H

#include <string>
#include "AudioInfo.h"

extern "C" {
#include <sox.h>
}


class CSoxEffects final
{
public:
    CSoxEffects();
    ~CSoxEffects() {
        Release();
    }
    bool Init(const AudioFormat& inFmt, const AudioFormat& outFmt);
    void Release();
    bool IsInitialized() const { return m_bInitialized; }
    bool HasActiveTransform() const { return m_bHaveTransFormat; }
    uint32_t Process(const uint8_t* input, uint32_t inSamples, uint8_t* output, uint32_t outSamples, bool bFlush);

protected:

private:
    bool m_bInitialized;
    bool m_bHaveTransFormat;
    sox_signalinfo_t m_inSignal;
    sox_signalinfo_t m_outSignal;
    sox_signalinfo_t m_interm_signal;
    sox_encodinginfo_t m_inEncoding;
    sox_encodinginfo_t m_outEncoding;
    AudioDataFormat m_inAudioFmt;
    AudioDataFormat m_outAudioFmt;
    sox_effect_t* m_inEfect;
    sox_effect_t* m_outEfect;
    sox_effects_chain_t* m_chain;
};

#endif //SOX_EFFECTS_H
