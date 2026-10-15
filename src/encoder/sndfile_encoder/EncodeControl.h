#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <sndfile.h>

#include "AudioInfo.h"
#include "DataStream.h"
#include "EncoderFormatDefine.h"
#include "EncoderParamterDefine.h"
#include "EncodingParams.h"
#include "MediaTag.h"

class CEncodeControl
{
public:
    explicit CEncodeControl(uint32_t streamFmt);
    ~CEncodeControl();
    CEncodeControl(const CEncodeControl&) = delete;
    CEncodeControl& operator=(const CEncodeControl&) = delete;

    static void QueryFormatsDefine(std::vector<EncoderFormatDefine>& fmtDesc);
    static void QueryParamtersDefine(uint32_t streamFmt, std::vector<EncoderParamterDefine>& paramDef);

    bool Open(CDataStream* pStream, const std::vector<EncoderParamter>& params);
    void Close();
    void SetMetaInfo(uint32_t streamIdx, const CMediaTag& metaTags);
    AudioFormat GetTransFormat() const;
    bool Start(const AudioFormat* audioFmt);
    uint32_t Encode(const void* pData, uint32_t frames);
    bool Stop(); // Finalizes the file, including buffered compressed audio.
    const std::string& GetErrorMessage() const { return m_error; }

private:
    bool Fail(const std::string& message);
    void ApplyMetaInfo();
    static sf_count_t GetLengthCb(void* userData) noexcept;
    static sf_count_t SeekCb(sf_count_t offset, int whence, void* userData) noexcept;
    static sf_count_t ReadCb(void* ptr, sf_count_t count, void* userData) noexcept;
    static sf_count_t WriteCb(const void* ptr, sf_count_t count, void* userData) noexcept;
    static sf_count_t TellCb(void* userData) noexcept;

    uint32_t m_streamFmt;
    CDataStream* m_stream = nullptr;
    SNDFILE* m_file = nullptr;
    AudioFormat m_transFmt{};
    AudioFormat m_inputFmt{};
    CMediaTag m_tags;
    std::string m_error;
    bool m_started = false;
    bool m_stopped = false;
    bool m_ioError = false;
};
