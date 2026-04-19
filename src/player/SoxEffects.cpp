#include <string_view>
#include <cassert>
#include "ScopeGuard.h"
#include "SoxInputEffect.h"
#include "SoxOutputEffect.h"
#include "SoxEffects.h"
#include "SoxLibInit.h"



bool MakeSureSoxInit()
{
    static CSoxLibInit libIbit;

    return libIbit.IsValid();
}

void InitSignalInfo(sox_signalinfo_t& info, const AudioFormat& fmt, std::size_t totalFrames)
{
    info.rate = fmt.sampleRate;
    info.channels = fmt.numChannels;
    info.precision = GetBitsPerSampleByFormat(fmt.format);
    info.length = totalFrames * fmt.numChannels;
    info.mult = nullptr;
}

CSoxEffects::CSoxEffects() : m_inEfect(nullptr), m_outEfect(nullptr), m_chain(nullptr),
                             m_bInitialized(false), m_bHaveTransFormat(false)
{
}

sox_encoding_t SoxEncodingFromPcmFormat(AudioDataFormat format)
{
    sox_encoding_t encoding = SOX_ENCODING_UNKNOWN;
    switch (format)
    {
    case AudioDataFormat::PCM_S8:
    case AudioDataFormat::PCM_S16:
    case AudioDataFormat::PCM_S24:
    case AudioDataFormat::PCM_S24_32:
    case AudioDataFormat::PCM_S32:
    case AudioDataFormat::PCM_64:
        encoding = SOX_ENCODING_SIGN2;
        break;
    case AudioDataFormat::PCM_U8:
        encoding = SOX_ENCODING_UNSIGNED;
        break;
    case AudioDataFormat::Float32:
    case AudioDataFormat::Float64:
        encoding = SOX_ENCODING_FLOAT;
        break;
    case AudioDataFormat::Ulaw:
        encoding = SOX_ENCODING_ULAW;
        break;
    case AudioDataFormat::Alaw:
        encoding = SOX_ENCODING_ALAW;
        break;
    case AudioDataFormat::G721Adpcm:
        encoding = SOX_ENCODING_G721;
        break;
    case AudioDataFormat::G723Adpcm_24:
    case AudioDataFormat::G723Adpcm_40:
        encoding = SOX_ENCODING_G723;
        break;
    case AudioDataFormat::MpegLayer3:
        encoding = SOX_ENCODING_MP3;
        break;
    case AudioDataFormat::ImaAdpcm:
        encoding = SOX_ENCODING_IMA_ADPCM;
        break;
    case AudioDataFormat::MsAdpcm:
        encoding = SOX_ENCODING_MS_ADPCM;
        break;
    case AudioDataFormat::Gsm610:
        encoding = SOX_ENCODING_GSM;
        break;
    case AudioDataFormat::Vorbis:
        encoding = SOX_ENCODING_VORBIS;
        break;
    case AudioDataFormat::Opus:
        encoding = SOX_ENCODING_OPUS;
        break;
    case AudioDataFormat::Dpcm_8:
        encoding = SOX_ENCODING_CL_ADPCM;
        break;
    case AudioDataFormat::Dpcm_16:
        encoding = SOX_ENCODING_CL_ADPCM16;
        break;
    default:
        encoding = SOX_ENCODING_UNKNOWN;
        break;
    }

    return encoding;
}

bool CSoxEffects::Init(const AudioFormat& inFmt, const AudioFormat& outFmt)
{
    assert(m_chain == nullptr);
    assert(m_outEfect == nullptr);
    assert(m_inEfect == nullptr);
    /*
    if (sox_init() != SOX_SUCCESS)
        return false;

    sox_format_init();
    */
    m_bInitialized = MakeSureSoxInit();
    InitSignalInfo(m_inSignal, inFmt, 0);
    InitSignalInfo(m_outSignal, outFmt, 0);
    m_inEncoding = { SoxEncodingFromPcmFormat(inFmt.format), m_inSignal.precision, sox_option_default, sox_option_default, sox_option_default, sox_option_default };
    m_outEncoding = { SoxEncodingFromPcmFormat(outFmt.format), m_outSignal.precision, sox_option_default, sox_option_default, sox_option_default, sox_option_default };
    m_inAudioFmt = inFmt.format;
    m_outAudioFmt = outFmt.format;

    // 处理链
    m_chain = sox_create_effects_chain(&m_inEncoding, &m_outEncoding);
    if (m_chain == nullptr)
        return false;

    m_interm_signal = m_inSignal;
    m_bHaveTransFormat = false;
    // ========== 添加自定义输入 effect ==========
    m_inEfect = sox_create_effect(get_buf_input_handler());
    sox_add_effect(m_chain, m_inEfect, &m_inSignal, &m_interm_signal);
    if (m_inAudioFmt != m_outAudioFmt)
    {
        m_bHaveTransFormat = true;
    }

    if (m_inSignal.rate != m_outSignal.rate)
    {
        // 添加 rate (重采样)
        sox_effect_t* e = sox_create_effect(sox_find_effect("rate"));
        std::string strRate = std::to_string(m_outSignal.rate);
        char* rateArgs[] = { (char *)"-v", (char*)strRate.c_str() };
        sox_effect_options(e, 2, rateArgs);
        sox_add_effect(m_chain, e, &m_interm_signal, &m_interm_signal);
        free(e);
        m_bHaveTransFormat = true;
    }

    if (m_inSignal.channels != m_outSignal.channels)
    {
        sox_effect_t* e = sox_create_effect(sox_find_effect("channels"));
        std::string strChan = std::to_string(m_outSignal.channels);
        char* argsC[] = { (char*)strChan.c_str() };  // 目标声道数
        sox_effect_options(e, 1, argsC);
        sox_add_effect(m_chain, e, &m_interm_signal, &m_interm_signal);
        free(e);
        m_bHaveTransFormat = true;
    }

    if (m_bHaveTransFormat && (m_outSignal.precision <= 16))
    {
    }

    m_outEfect = sox_create_effect(get_buf_output_handler());
    sox_add_effect(m_chain, m_outEfect, &m_outSignal, &m_outSignal);

    return true;
}

void CSoxEffects::Release()
{
    if (m_inEfect != nullptr)
    {
        free(m_inEfect);
        m_inEfect = nullptr;
    }
    if (m_outEfect != nullptr)
    {
        free(m_outEfect);
        m_outEfect = nullptr;
    }

    if (m_chain != nullptr)
    {
        sox_delete_effects_chain(m_chain);
        m_chain = nullptr;
    }
    m_bInitialized = false;
    m_bHaveTransFormat = false;
}

uint32_t CSoxEffects::Process(const uint8_t* input, uint32_t inSamples, uint8_t* output, uint32_t outSamples, bool bFlush)
{
    assert(m_chain != nullptr);
    assert(m_outEfect != nullptr);
    assert(m_inEfect != nullptr);

    std::string strInSize = std::to_string(inSamples);
    char* argsIn[] = { (char*)input, (char*)strInSize.c_str() };
    sox_effect_options(m_inEfect, 2, argsIn);

    double ch_rate = double(m_outSignal.channels) / m_inSignal.channels;
    uint32_t outBufferSamples = uint32_t((double(inSamples * m_outSignal.rate) / m_inSignal.rate + 0.5) * ch_rate);
    if (outSamples < outBufferSamples)
        return 0;

    std::string strOutSize = std::to_string(outSamples);
    char* argsOut[] = { (char*)output, (char*)strOutSize.c_str() };
    sox_effect_options(m_outEfect, 2, argsOut);

    sox_effect_t* rateEff = sox_get_effect(m_chain, "rate");
    if ((rateEff != nullptr) && bFlush)
    {
        sox_set_rate_effect_flush_sign(rateEff);
    }

    int ret = sox_flow_effects(m_chain, NULL, NULL);
    if (ret == SOX_SUCCESS)
    {
        return outSamples;
    }

    return 0;
}

