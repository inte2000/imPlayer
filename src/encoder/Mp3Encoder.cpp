#include <fstream>
#include <string_view>
#include <format>
#include "Mp3Encoder.h"
#include "ConvertFormat.h"
#include "UnicodeConvert.h"
#include "nlohmann/json.hpp" 

using json = nlohmann::json;



CMp3Encoder::CMp3Encoder(uint32_t streamFmt) : m_lame(nullptr)
{
    m_streamFmt = streamFmt;
    m_channels = 2;
    m_sampleRates = 0;
    m_mp3BufSize = 0;
    m_name = "Lame mp3 encoder";
}

CMp3Encoder::~CMp3Encoder()
{
    if (m_lame != nullptr)
    {
        lame_close(m_lame);
    }
}

typedef struct Mp3EncodingParams
{
    int sampleRate;
    int bitsrate;
    int bitsPerSample;
    MPEG_mode chMode;
    vbr_mode vbrMode;
    float vbrQuality; //0-9, 0 is the best
}Mp3EncodingParams;

static MPEG_mode MpegModeFromString(const std::string& value)
{
    if (value.compare("Joint Stereo") == 0)
        return JOINT_STEREO;
    else if (value.compare("Stereo") == 0)
        return STEREO;
    else if (value.compare("Dual Channel") == 0)
        return DUAL_CHANNEL;
    else
        return MONO;
}

static vbr_mode VbrModeFromString(const std::string& value)
{
    if (value.compare("Mtrh") == 0)
        return vbr_mtrh;
    else if (value.compare("Abr") == 0)
        return vbr_abr;
    else if (value.compare("Rh") == 0)
        return vbr_rh;
    else
        return vbr_off;
} 


static Mp3EncodingParams ExtractEncodingParams(const std::string& jsonParams)
{
    Mp3EncodingParams ebp = {};

    json jsonObj = json::parse(jsonParams.c_str()); //可能会抛异常
    auto bps = ExtractIntValue(jsonObj, paramname_bitspersample);
    ebp.bitsPerSample= bps ? bps.value() : 16;
    auto sampleRate = ExtractIntValue(jsonObj, paramname_samplerate);
    ebp.sampleRate = sampleRate ? sampleRate.value() : 44100;
    auto vbrQuality = ExtractFloatValue(jsonObj, paramname_vbrLevel);
    ebp.vbrQuality = vbrQuality ? vbrQuality.value() : 4.0f;
    auto bitrate = ExtractIntValue(jsonObj, paramname_bitrate);
    ebp.bitsrate = bitrate ? bitrate.value() : 128;
    auto chMode = ExtractValue(jsonObj, paramname_chmode);
    ebp.chMode = chMode ? MpegModeFromString(chMode.value()) : JOINT_STEREO;
    auto vbrMode = ExtractValue(jsonObj, paramname_vbrmode);
    ebp.vbrMode = vbrMode ? VbrModeFromString(vbrMode.value()) : vbr_off;
    
    return ebp;
}

bool CMp3Encoder::Init(const std::string& jsonParams)
{
    Mp3EncodingParams mp3Param = ExtractEncodingParams(jsonParams);

    m_lame = lame_init();
    if (!m_lame)
        throw std::runtime_error("Fail to initialize lame!");

    m_channels = (mp3Param.chMode == MONO) ? 1 : 2;
    m_sampleRates = mp3Param.sampleRate;

    // 设置 LAME 参数
    lame_set_num_channels(m_lame, m_channels);
    lame_set_in_samplerate(m_lame, mp3Param.sampleRate);
    lame_set_brate(m_lame, mp3Param.bitsrate);           // 设置比特率
    lame_set_quality(m_lame, 2);               // 设置质量 (0-9, 0最好)
    lame_set_mode(m_lame, mp3Param.chMode);       // 设置声道模式
    lame_set_VBR(m_lame, mp3Param.vbrMode);
    if (mp3Param.vbrMode != vbr_off)
    {
        lame_set_VBR_quality(m_lame, mp3Param.vbrQuality);
    }

    if (lame_init_params(m_lame) < 0) 
    {
        lame_close(m_lame);
        throw std::runtime_error("Fail to set lame parameters!");
    }

    SetId3Tags(m_lame, m_metaInfo);
    m_mp3BufSize = 32768 * 4; //按照 32k frame，双声道准备 MP3 编码缓冲区大小
    m_mp3Buf = std::make_unique<unsigned char[]>(m_mp3BufSize);

    return true;
}

void CMp3Encoder::SetMetaInfo(const std::string& jsonMeta)
{
    m_metaInfo = ParseBaseMetaInfo(jsonMeta);
}

std::vector<EncoderParamterDef> CMp3Encoder::QueryParamtersDefine() const
{
    std::vector<EncoderParamterDef> params;

    EncoderParamterDef rateParam = { paramname_samplerate, ParamTypeEnum::Selector, "44100", false, {},
        {"32000", "44100", "48000"} };
    params.push_back(std::move(rateParam));
    EncoderParamterDef channelmode = { paramname_chmode, ParamTypeEnum::Selector, "Joint Stereo", false, {}, 
        {"Stereo", "Joint Stereo", "Dual Channel", "Mono"} };
    params.push_back(std::move(channelmode));
    EncoderParamterDef bitrate = { paramname_bitrate, ParamTypeEnum::Selector, "128", false, {},
        {"32", "40", "56", "64", "80", "96", "112", "128", "160", "192", "224", "256", "320"} };
    params.push_back(std::move(bitrate));
    EncoderParamterDef vbrMode = { paramname_vbrmode, ParamTypeEnum::Selector, "Off", false, {},
        {"Off", "Abr", "Rh", "Mtrh"} };
    params.push_back(std::move(vbrMode));
    EncoderParamterDef vbrQuality = { paramname_vbrLevel, ParamTypeEnum::floatRange, "4.0", false, {},
        {}, {0.0f, 9.0f} };
    params.push_back(std::move(vbrQuality));

    return params;
}

uint32_t CMp3Encoder::Encode(const void* pData, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((m_lame == nullptr) || (m_pStream == nullptr))
        return 0;
    if ((audioFmt->sampleRate != m_sampleRates) || (audioFmt->numChannels != m_channels))
        throw std::runtime_error("Encoder not support sample rate or channels converting!");

    uint32_t encBufSize = frames * m_channels * 2; //16bits 
    if (m_mp3BufSize < encBufSize)
    {
        m_mp3Buf.reset(new unsigned char[encBufSize]);
        m_mp3BufSize = encBufSize;
    }

    int encodedBytes = 0;
    if (audioFmt->format == AudioDataFormat::Float32)
    {
        const float* pcmBuf = static_cast<const float*>(pData);
        if (m_channels == 2)
            encodedBytes = lame_encode_buffer_interleaved_ieee_float(m_lame, pcmBuf, frames, m_mp3Buf.get(), m_mp3BufSize);
        else
            encodedBytes = lame_encode_buffer_ieee_float(m_lame, pcmBuf, nullptr, frames, m_mp3Buf.get(), m_mp3BufSize);
    }
    else if (audioFmt->format == AudioDataFormat::Float64)
    {
        const double* pcmBuf = static_cast<const double*>(pData);
        if (m_channels == 2)
            encodedBytes = lame_encode_buffer_interleaved_ieee_double(m_lame, pcmBuf, frames, m_mp3Buf.get(), m_mp3BufSize);
        else
            encodedBytes = lame_encode_buffer_ieee_double(m_lame, pcmBuf, nullptr, frames, m_mp3Buf.get(), m_mp3BufSize);
    }
    else if (audioFmt->format == AudioDataFormat::PCM_S16)
    {
        const short* pcmBuf = static_cast<const short*>(pData);
        if (m_channels == 2)
            encodedBytes = lame_encode_buffer_interleaved(m_lame, (short*)pcmBuf, frames, m_mp3Buf.get(), m_mp3BufSize);
        else
            encodedBytes = lame_encode_buffer(m_lame, pcmBuf, nullptr, frames, m_mp3Buf.get(), m_mp3BufSize);
    }
    else if ((audioFmt->format == AudioDataFormat::PCM_S24) || (audioFmt->format == AudioDataFormat::PCM_S32))
    {
        const int* pcmBuf = static_cast<const int*>(pData);
        if (m_channels == 2)
            encodedBytes = lame_encode_buffer_interleaved_int(m_lame, pcmBuf, frames, m_mp3Buf.get(), m_mp3BufSize);
        else
            encodedBytes = lame_encode_buffer_int(m_lame, pcmBuf, nullptr, frames, m_mp3Buf.get(), m_mp3BufSize);
    }
    else
        encodedBytes = 0;
    
    if (encodedBytes < 0) {
        //encodedBytes 是错误码
        throw std::runtime_error("lame mp3 encoder fail to encode a buffer!");
    }

    uint32_t writtenBytes = m_pStream->Write(m_mp3Buf.get(), encodedBytes);
    if (writtenBytes != encodedBytes)
    {
        return 0;
    }

    return frames;
}

bool CMp3Encoder::Flush()
{
    if ((m_lame == nullptr) || (m_pStream == nullptr))
        return false;

    int encodedBytes = lame_encode_flush(m_lame, m_mp3Buf.get(), m_mp3BufSize);
    if (encodedBytes >= 0) 
    {
        uint32_t writtenBytes = m_pStream->Write(m_mp3Buf.get(), encodedBytes);
        return (writtenBytes == encodedBytes);
    }

    return false;
}

bool CMp3Encoder::IsSupportFormat(uint32_t mediaFmt) const
{
    return true;
}

void CMp3Encoder::SetId3Tags(lame_global_flags* lame, const MediaBaseMetaInfo& metaInfo)
{
    id3tag_init(lame);
    id3tag_add_v2(lame);
    //id3tag_v2_only(lame); // 只写 ID3v2
    //ISO-8859-1

    if(!metaInfo.title.empty())
        id3tag_set_title(lame, Utf8ToLocalMBCS(metaInfo.title).c_str());
    if (!metaInfo.artist.empty())
        id3tag_set_artist(lame, Utf8ToLocalMBCS(metaInfo.artist).c_str());
    if (!metaInfo.album.empty())
        id3tag_set_album(lame, Utf8ToLocalMBCS(metaInfo.album).c_str());
    if (!metaInfo.genre.empty())
        id3tag_set_genre(lame, Utf8ToLocalMBCS(metaInfo.genre).c_str());
    if (!metaInfo.year.empty())
        id3tag_set_year(lame, Utf8ToLocalMBCS(metaInfo.year).c_str());
    if (!metaInfo.tracks.empty())
        id3tag_set_track(lame, Utf8ToLocalMBCS(metaInfo.tracks).c_str());

    //id3tag_set_comment(lame_, comment);
}