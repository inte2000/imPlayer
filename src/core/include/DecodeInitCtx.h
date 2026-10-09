#ifndef DECODE_INIT_CONTEXT_H
#define DECODE_INIT_CONTEXT_H

#include <memory>
#include "AudioInfo.h"

class CDecodeInitCtx
{
public:
    virtual ~CDecodeInitCtx() = default; 
    
    virtual uint32_t GetInitSampleRate() const = 0; //for midi decoder or SACD decoder
    virtual AudioDataFormat GetInitDataFormat() const = 0;
    virtual uint32_t GetInitChannels() const = 0;
    virtual uint32_t GetInitChLayout() const = 0;
    virtual float GetGain() const = 0;
    virtual std::string GetMidiInstruments() const = 0;
    virtual std::string GetMidiBackendName() const = 0;
    virtual uint32_t GetMultiChAreaMode() const = 0;
};

#endif //DECODE_INIT_CONTEXT_H
