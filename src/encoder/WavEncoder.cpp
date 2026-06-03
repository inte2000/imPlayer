#include <cstdint>
#include <memory>
#include <stdexcept>

#include "dr_wav.h"

#include "WavEncoder.h"
#include "encoder/EncoderParamName.h"
#include "core/EncodingParams.h"
#include "encoder/EncoderParamterDefineUtils.h"

namespace {

constexpr const char* s_WavEncoderName = "dr_wav Encoder";

static EncoderParamterDefine s_suppDefs[] =
{
    { EncoderParamName::CodeFormat, EncoderParamType::UnsignedInt, static_cast<uint32_t>(AudioDataFormat::PCM_S16), {
        static_cast<uint32_t>(AudioDataFormat::PCM_U8),
        static_cast<uint32_t>(AudioDataFormat::PCM_S16),
        static_cast<uint32_t>(AudioDataFormat::PCM_S24),
        static_cast<uint32_t>(AudioDataFormat::PCM_S32),
        static_cast<uint32_t>(AudioDataFormat::Float32),
        static_cast<uint32_t>(AudioDataFormat::Float64)
    } },
    { EncoderParamName::SampleRates, EncoderParamType::UnsignedInt, static_cast<uint32_t>(44100), {
        static_cast<uint32_t>(8000), static_cast<uint32_t>(9600), static_cast<uint32_t>(10125),
        static_cast<uint32_t>(20220), static_cast<uint32_t>(32000), static_cast<uint32_t>(44100),
        static_cast<uint32_t>(48000), static_cast<uint32_t>(96000), static_cast<uint32_t>(192000)
    } },
    { EncoderParamName::BitsPerSample, EncoderParamType::UnsignedInt, static_cast<uint32_t>(16), {
        static_cast<uint32_t>(8), static_cast<uint32_t>(16), static_cast<uint32_t>(24),
        static_cast<uint32_t>(32), static_cast<uint32_t>(64)
    } },
    { EncoderParamName::Channels, EncoderParamType::UnsignedInt, static_cast<uint32_t>(2), {
        static_cast<uint32_t>(1), static_cast<uint32_t>(2), static_cast<uint32_t>(3), static_cast<uint32_t>(4),
        static_cast<uint32_t>(5), static_cast<uint32_t>(6), static_cast<uint32_t>(7), static_cast<uint32_t>(8),
        static_cast<uint32_t>(9), static_cast<uint32_t>(10), static_cast<uint32_t>(11),static_cast<uint32_t>(12),
        static_cast<uint32_t>(13), static_cast<uint32_t>(14), static_cast<uint32_t>(15), static_cast<uint32_t>(16)
    } },
    { EncoderParamName::ExtWav, EncoderParamType::Bool, false, { true, false } }
};

static EncoderFormatDefine s_suppFmts[] =
{
    { StreamFormatWav, "MS-Wav format", ".wav" }
};

SeekBase ToStreamSeekBase(drwav_seek_origin origin)
{
    if (origin == DRWAV_SEEK_SET) {
        return SeekBase::Begin;
    }
    return SeekBase::Cur;
}

size_t WavWriteProc(void* pUserData, const void* pData, size_t bytesToWrite)
{
    CDataStream* pStream = static_cast<CDataStream*>(pUserData);
    if ((pStream == nullptr) || (pData == nullptr) || (bytesToWrite == 0)) {
        return 0;
    }

    const uint32_t writeSize = (bytesToWrite > static_cast<size_t>(UINT32_MAX)) ? UINT32_MAX : static_cast<uint32_t>(bytesToWrite);
    return static_cast<size_t>(pStream->Write(pData, writeSize));
}

drwav_bool32 WavSeekProc(void* pUserData, int offset, drwav_seek_origin origin)
{
    CDataStream* pStream = static_cast<CDataStream*>(pUserData);
    if (pStream == nullptr) {
        return DRWAV_FALSE;
    }

    try {
        pStream->Seek(ToStreamSeekBase(origin), offset);
    }
    catch (...) {
        return DRWAV_FALSE;
    }

    return DRWAV_TRUE;
}

} // namespace

CWavEncoder::CWavEncoder(uint32_t streamFmt)
    : m_streamFmt(streamFmt)
    , m_extWav(false)
    , m_initialized(false)
    , m_transFmt({})
    , m_metaInfo()
    , m_wav(new drwav())
{
    InitEmptyAudioFormat(&m_transFmt);

    m_name = GetName();
    m_publisher = "imPlayer";
    m_verMajor = 1;
    m_verMinor = 0;
    m_type = ENCODE_TYPE_NATIVE;
    m_pStream = nullptr;
}

CWavEncoder::~CWavEncoder()
{
    if (m_wav != nullptr) {
        drwav* wav = static_cast<drwav*>(m_wav);
        drwav_uninit(wav);
        delete wav;
        m_wav = nullptr;
    }
}

bool CWavEncoder::Init(const std::vector<EncoderParamter>& params)
{
    if ((m_wav == nullptr) || (m_pStream == nullptr)) {
        return false;
    }

    const auto dataFmtOpt = GetEncoderParamterValue<uint32_t>(params, EncoderParamName::CodeFormat);
    const auto sampleRateOpt = GetEncoderParamterValue<uint32_t>(params, EncoderParamName::SampleRates);
    const auto bitsPerSampleOpt = GetEncoderParamterValue<uint32_t>(params, EncoderParamName::BitsPerSample);
    const auto channelsOpt = GetEncoderParamterValue<uint32_t>(params, EncoderParamName::Channels);
    const auto extWavOpt = GetEncoderParamterValue<bool>(params, EncoderParamName::ExtWav);

    const auto defaults = BuildDefaultEncoderParamters(GetParameterDefine());

    const AudioDataFormat dataFmt = static_cast<AudioDataFormat>(dataFmtOpt.value_or(
        GetEncoderParamterValue<uint32_t>(defaults, EncoderParamName::CodeFormat).value_or(static_cast<uint32_t>(AudioDataFormat::PCM_S16))));
    const uint32_t sampleRate = sampleRateOpt.value_or(GetEncoderParamterValue<uint32_t>(defaults, EncoderParamName::SampleRates).value_or(44100));
    const uint32_t bitsPerSample = bitsPerSampleOpt.value_or(GetEncoderParamterValue<uint32_t>(defaults, EncoderParamName::BitsPerSample).value_or(16));
    const uint32_t channels = channelsOpt.value_or(GetEncoderParamterValue<uint32_t>(defaults, EncoderParamName::Channels).value_or(2));
    m_extWav = extWavOpt.value_or(GetEncoderParamterValue<bool>(defaults, EncoderParamName::ExtWav).value_or(false));

    if (!IsSupportedDataFormat(dataFmt)) {
        throw std::runtime_error("CWavEncoder unsupported CodeFormat");
    }
    if (channels == 0) {
        throw std::runtime_error("CWavEncoder invalid channels");
    }
    if (sampleRate == 0) {
        throw std::runtime_error("CWavEncoder invalid sample rate");
    }

    InitAudioFormat(&m_transFmt, dataFmt, channels, sampleRate, bitsPerSample);

    drwav_data_format format = {};
    format.container = drwav_container_riff;
    format.format = ToWaveFormatTag(dataFmt, m_extWav);
    format.channels = channels;
    format.sampleRate = sampleRate;
    format.bitsPerSample = bitsPerSample;

    drwav* wav = static_cast<drwav*>(m_wav);
    if (!drwav_init_write(wav, &format, WavWriteProc, WavSeekProc, m_pStream, nullptr)) {
        throw std::runtime_error("CWavEncoder failed to initialize dr_wav writer");
    }

    m_initialized = true;
    return true;
}

AudioFormat CWavEncoder::GetTransFormat() const
{
    return m_transFmt;
}

void CWavEncoder::SetMetaInfo(const CMediaTag& metaTags)
{
    m_metaInfo = metaTags;
}

uint32_t CWavEncoder::Encode(const void* pData, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((!m_initialized) || (m_wav == nullptr) || (pData == nullptr) || (audioFmt == nullptr) || (frames == 0)) {
        return 0;
    }

    if (!IsSameAudioFormat(audioFmt, &m_transFmt)) {
        throw std::runtime_error("CWavEncoder input format mismatch");
    }

    drwav* wav = static_cast<drwav*>(m_wav);
    const drwav_uint64 writtenFrames = drwav_write_pcm_frames(wav, static_cast<drwav_uint64>(frames), pData);
    return static_cast<uint32_t>(writtenFrames);
}

bool CWavEncoder::Flush()
{
    if (!m_initialized) {
        return false;
    }

    return true;
}

std::string CWavEncoder::GetName()
{
    return s_WavEncoderName;
}

std::vector<EncoderParamterDefine> CWavEncoder::GetParameterDefine()
{
    return std::vector<EncoderParamterDefine>(std::begin(s_suppDefs), std::end(s_suppDefs));
}

std::vector<EncoderFormatDefine> CWavEncoder::GetFormatDefine()
{
    return std::vector<EncoderFormatDefine>(std::begin(s_suppFmts), std::end(s_suppFmts));
}

uint32_t CWavEncoder::ToWaveFormatTag(AudioDataFormat dataFmt, bool extWav)
{
    if (extWav) {
        return DR_WAVE_FORMAT_EXTENSIBLE;
    }

    if ((dataFmt == AudioDataFormat::Float32) || (dataFmt == AudioDataFormat::Float64)) {
        return DR_WAVE_FORMAT_IEEE_FLOAT;
    }

    return DR_WAVE_FORMAT_PCM;
}

bool CWavEncoder::IsSupportedDataFormat(AudioDataFormat dataFmt)
{
    return (dataFmt == AudioDataFormat::PCM_U8)
        || (dataFmt == AudioDataFormat::PCM_S16)
        || (dataFmt == AudioDataFormat::PCM_S24)
        || (dataFmt == AudioDataFormat::PCM_S32)
        || (dataFmt == AudioDataFormat::Float32)
        || (dataFmt == AudioDataFormat::Float64);
}
