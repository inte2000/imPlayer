#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <memory>
#include "AudioInfo.h"
#include "DecodeInitCtx.h"
#include "DataStream.h"


struct DecodeInformation
{
    AudioDataFormat format;
    uint32_t channels;
    uint32_t bytesPerSample;
    long long curFrames;
    long long totalFrames;
};

const uint32_t MAX_DECODE_TMP_FRAMES = 163840;

const uint32_t DECODE_TYPE_UNKNOWN = 0;
const uint32_t DECODE_TYPE_NATIVE = 1;
const uint32_t DECODE_TYPE_PLUSIN = 2;

class CAudioDecoder
{
public:
    CAudioDecoder() {}
    virtual ~CAudioDecoder() {}

    //no copy, can move
    CAudioDecoder(const CAudioDecoder&) = delete;
    CAudioDecoder& operator =(const CAudioDecoder&) = delete;
    CAudioDecoder(CAudioDecoder&&) = default;
    CAudioDecoder& operator =(CAudioDecoder&&) = default;

    CDataStream* Attach(CDataStream* pStream)
    {
        CDataStream* pTmp = m_pStream;
        m_pStream = pStream;
        return pTmp;
    }
    const std::string& GetName() const { return m_name; }
    std::string& GetName() { return m_name; }
    const uint32_t GetType() const { return m_type; }

    const CDataStream* GetStream() const { return m_pStream; }
    CDataStream* GetStream() { return m_pStream; }

    virtual AudioInfo InitDecode(const CDecodeInitCtx* decodeInit) = 0;
    virtual uint32_t Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat *audioFmt) = 0;
    virtual void SeekTo(std::size_t frames) = 0;
    virtual std::size_t GetCurrentFrame() const = 0;
    virtual std::size_t GetTotalFrame() const = 0;
    virtual bool IsSupportOutput(const AudioFormat* audioFmt) const = 0;
protected:
    std::string m_name;
    uint32_t m_type;
    CDataStream* m_pStream;
};

#endif //AUDIO_DECODER_H
