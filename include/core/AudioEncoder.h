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


class CAudioEncoder
{
public:
    CAudioEncoder() {}
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
    std::string& GetName() { return m_name; }

    virtual bool Init(const std::string& jsonParams) = 0;
    virtual void SetMetaInfo(const CMediaTag& metaTags) = 0;
    virtual std::vector<EncoderParamterDef> QueryParamtersDefine() const = 0;
    virtual uint32_t Encode(const void* pData, uint32_t frames, const AudioFormat* audioFmt) = 0;
    virtual bool Flush() = 0;
    virtual bool IsSupportFormat(uint32_t mediaFmt) const = 0;
protected:
    std::string m_name;
    CDataStream* m_pStream;
};

#endif //AUDIO_ENCODER_H
