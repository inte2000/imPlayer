/*
这个头文件是在 AI 输出的基础上，进行了手工修改，然后再由 AI 维护的。手工修改主要集中在接口函数的参数类型上

20260207：
引入 MediaTag，替代之前的 JSON 字符串作为媒体元信息的载体。隐约记得当时给出的任务要求是：
“将 InitDecode 接口的返回值改成 bool 类型，用于标识初始化操作的成功与否，不再通过返回值获取媒体元信息。新增一个获取
元信息的接口：GetTags，入口参数是流索引，返回值类型是 CMediaTag，声明在 MediaTag.h 中。同时请根据接口的变化同步修改
已经存在的解码器：CMp3Decoder、CFlacDecoder、CLibSndDecoder、CCDTrackDecoder、CMidiDecoder 和 CCueTrackDecoder”
使用的工具是 OpenCode + GLM4.7，其结果是对这个文件的修改没有问题，还自动引入了头文件，但是对相关的几个解码器修改 
InitDecode 函数和增加 GetTags 函数的部分不理想，需要做一些手工修改
*/
#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <memory>
#include "AudioInfo.h"
#include "DataStream.h"
#include "MediaTag.h"

const uint32_t MAX_DECODE_TMP_FRAMES = 163840;

const uint32_t DECODE_TYPE_UNKNOWN = 0;
const uint32_t DECODE_TYPE_NATIVE = 1;
const uint32_t DECODE_TYPE_PLUGIN = 2;

class CAudioDecoder
{
public:
    CAudioDecoder() : m_type(DECODE_TYPE_UNKNOWN), m_curStreamIdx(-1), m_verMajor(0), m_verMinor(0),
                      m_StreamCount(1), m_pStream(nullptr) {}
    virtual ~CAudioDecoder() {}

    //不支持拷贝，但是可以移动
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
    const std::string& GetPublisher() const { return m_publisher; }
    std::tuple<uint32_t, uint32_t> GetVersion() const { return { m_verMajor, m_verMinor }; }
    const uint32_t GetType() const { return m_type; }
    uint32_t GetCurrentStreamIndex() const { return m_curStreamIdx;  }
    uint32_t GetAudioStreamCount() const { return m_StreamCount; }

    const CDataStream* GetStream() const { return m_pStream; }
    CDataStream* GetStream() { return m_pStream; }

    virtual bool InitDecode(const CDecodeInitCtx* decodeInit) = 0;
    virtual bool StartStream(uint32_t streamIdx, std::size_t begin, std::size_t end, uint32_t loop) = 0;
    virtual void StopStream(uint32_t streamIdx) = 0;
    virtual CMediaTag GetTags(uint32_t streamIdx) = 0;
    virtual AudioFormat GetAudioFormat(uint32_t streamIdx) const = 0;
    virtual uint32_t Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat *audioFmt) = 0;
    virtual void SeekTo(std::size_t frames) = 0;
    virtual std::size_t GetCurrentFrame() const = 0;
    virtual std::size_t GetTotalFrame() const = 0;
    virtual bool IsSupportOutput(const AudioFormat* audioFmt) const = 0;
    virtual bool IsCanSeeking(uint32_t streamIdx) const = 0;
    virtual void Reset() = 0;
protected:
    std::string m_name;
    std::string m_publisher;
    uint32_t m_verMajor; 
    uint32_t m_verMinor;
    uint32_t m_type;
    uint32_t m_curStreamIdx;
    uint32_t m_StreamCount;
    CDataStream* m_pStream;
};

#endif //AUDIO_DECODER_H
