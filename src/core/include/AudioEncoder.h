/*
Human Action，指定接口定义
*/
#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include <memory>
#include <string>
#include <vector>
#include "AudioInfo.h"
#include "DataStream.h"
#include "EncodingParams.h"
#include "MediaTag.h"

const uint32_t ENCODE_TYPE_UNKNOWN = 0;
const uint32_t ENCODE_TYPE_NATIVE = 1;
const uint32_t ENCODE_TYPE_PLUGIN = 2;

class CAudioEncoder
{
public:
    CAudioEncoder() : m_verMajor(0), m_verMinor(0), m_type(ENCODE_TYPE_UNKNOWN), m_pStream(nullptr) {}
    virtual ~CAudioEncoder() {}

    //不支持拷贝，但是可以移动
    CAudioEncoder(const CAudioEncoder&) = delete;
    CAudioEncoder& operator =(const CAudioEncoder&) = delete;
    CAudioEncoder(CAudioEncoder&&) = default;
    CAudioEncoder& operator =(CAudioEncoder&&) = default;

    CDataStream* Attach(CDataStream* pStream)
    {
        CDataStream* pTmp = m_pStream;
        m_pStream = pStream;
        return pTmp;
    }
    const std::string& GetName() const { return m_name; }
    const std::string& GetPublisher() const { return m_publisher; }
    std::tuple<uint32_t, uint32_t> GetVersion() const { return { m_verMajor, m_verMinor }; }
    const uint32_t GetType() const { return m_type; }

    virtual void Init() = 0;
    virtual bool Open(const std::vector<EncoderParamter>& params) = 0;
    virtual AudioFormat GetTransFormat() const = 0;
    virtual bool SetMetaInfo(const CMediaTag& metaTags) = 0;
    // Start 固定本次编码的输入格式，Encode 期间不允许改变。
    virtual bool Start(const AudioFormat* audioFmt) = 0;
    virtual uint32_t Encode(const void* pData, uint32_t frames) = 0;
    virtual bool Stop() = 0;
    virtual void Close() = 0;
protected:
    std::string m_name;
    std::string m_publisher;
    uint32_t m_verMajor;
    uint32_t m_verMinor;
    uint32_t m_type;
    CDataStream* m_pStream;
};

#endif //AUDIO_ENCODER_H
