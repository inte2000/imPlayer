#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>

#include "EncodeControl.h"
#include "EncoderParamName.h"

namespace
{
struct FormatMapping
{
    uint32_t streamFmt;
    int major;
};

constexpr FormatMapping FORMAT_MAP[] = {
    { StreamFormatWav, SF_FORMAT_WAV }, { StreamFormatAiff, SF_FORMAT_AIFF },
    { StreamFormatAu, SF_FORMAT_AU }, { StreamFormatRaw, SF_FORMAT_RAW },
    { StreamFormatPaf, SF_FORMAT_PAF }, { StreamFormatSvx, SF_FORMAT_SVX },
    { StreamFormatNist, SF_FORMAT_NIST }, { StreamFormatVoc, SF_FORMAT_VOC },
    { StreamFormatIrcam, SF_FORMAT_IRCAM }, { StreamFormatW64, SF_FORMAT_W64 },
    { StreamFormatMat, SF_FORMAT_MAT4 }, { StreamFormatMat5, SF_FORMAT_MAT5 },
    { StreamFormatPvf, SF_FORMAT_PVF }, { StreamFormatXi, SF_FORMAT_XI },
    { StreamFormatHtk, SF_FORMAT_HTK }, { StreamFormatMidSds, SF_FORMAT_SDS },
    { StreamFormatAvr, SF_FORMAT_AVR }, { StreamFormatWavEx, SF_FORMAT_WAVEX },
    { StreamFormatFlac, SF_FORMAT_FLAC }, { StreamFormatCaf, SF_FORMAT_CAF },
    { StreamFormatWve, SF_FORMAT_WVE }, { StreamFormatOgg, SF_FORMAT_OGG },
    { StreamFormatMPC2k, SF_FORMAT_MPC2K }, { StreamFormatWav64, SF_FORMAT_RF64 },
    { StreamFormatMp3, SF_FORMAT_MPEG }
    // SD2 needs a named resource fork, unavailable through single-stream virtual I/O.
};

struct CodecMapping
{
    AudioDataFormat format;
    int subtype;
    uint32_t bits;
};

constexpr CodecMapping CODEC_MAP[] = {
    { AudioDataFormat::PCM_S16, SF_FORMAT_PCM_16, 16 },
    { AudioDataFormat::PCM_U8, SF_FORMAT_PCM_U8, 8 },
    { AudioDataFormat::PCM_S8, SF_FORMAT_PCM_S8, 8 },
    { AudioDataFormat::PCM_S24, SF_FORMAT_PCM_24, 24 },
    { AudioDataFormat::PCM_S32, SF_FORMAT_PCM_32, 32 },
    { AudioDataFormat::Float32, SF_FORMAT_FLOAT, 32 },
    { AudioDataFormat::Float64, SF_FORMAT_DOUBLE, 64 },
    { AudioDataFormat::Ulaw, SF_FORMAT_ULAW, 16 },
    { AudioDataFormat::Alaw, SF_FORMAT_ALAW, 16 },
    { AudioDataFormat::ImaAdpcm, SF_FORMAT_IMA_ADPCM, 16 },
    { AudioDataFormat::MsAdpcm, SF_FORMAT_MS_ADPCM, 16 },
    { AudioDataFormat::Gsm610, SF_FORMAT_GSM610, 16 },
    { AudioDataFormat::G721Adpcm, SF_FORMAT_G721_32, 16 },
    { AudioDataFormat::G723Adpcm_24, SF_FORMAT_G723_24, 16 },
    { AudioDataFormat::G723Adpcm_40, SF_FORMAT_G723_40, 16 },
    { AudioDataFormat::Dpcm_8, SF_FORMAT_DPCM_8, 8 },
    { AudioDataFormat::Dpcm_16, SF_FORMAT_DPCM_16, 16 },
    { AudioDataFormat::Vorbis, SF_FORMAT_VORBIS, 16 },
    { AudioDataFormat::Opus, SF_FORMAT_OPUS, 16 },
    { AudioDataFormat::Alac16, SF_FORMAT_ALAC_16, 16 },
    { AudioDataFormat::Alac20, SF_FORMAT_ALAC_20, 20 },
    { AudioDataFormat::Alac24, SF_FORMAT_ALAC_24, 24 },
    { AudioDataFormat::Alac32, SF_FORMAT_ALAC_32, 32 },
    { AudioDataFormat::MpegLayer3, SF_FORMAT_MPEG_LAYER_III, 16 }
};

int MajorFormat(uint32_t streamFmt)
{
    for (const auto& item : FORMAT_MAP) {
        if (item.streamFmt == streamFmt) {
            return item.major;
        }
    }
    throw std::runtime_error("Unsupported output stream format (SD2 requires a named resource fork).");
}

bool IsWritableSubtype(int major, int subtype)
{
    // 1.2.2 advertises AIFF/DWVW12 but does not initialize its codec writer.
    if (major == SF_FORMAT_AIFF && subtype == SF_FORMAT_DWVW_12
        && std::string_view(sf_version_string()) == "libsndfile-1.2.2") return false;
    // These MPEG subtypes are enumerated by libsndfile but have no writer.
    return subtype != SF_FORMAT_MPEG_LAYER_I && subtype != SF_FORMAT_MPEG_LAYER_II;
}

std::vector<int> Subtypes(int major)
{
    int count = 0;
    sf_command(nullptr, SFC_GET_FORMAT_SUBTYPE_COUNT, &count, sizeof(count));
    std::vector<int> result;
    for (int index = 0; index < count; ++index) {
        SF_FORMAT_INFO subtype{ index };
        if (sf_command(nullptr, SFC_GET_FORMAT_SUBTYPE, &subtype, sizeof(subtype)) != 0) {
            continue;
        }
        SF_INFO info{};
        info.samplerate = 44100;
        info.channels = 1;
        info.format = major | subtype.format;
        if (IsWritableSubtype(major, subtype.format) && sf_format_check(&info)) {
            result.push_back(subtype.format);
        }
    }
    return result;
}

const CodecMapping& DefaultCodec(const std::vector<int>& subtypes)
{
    for (const auto& codec : CODEC_MAP) {
        if (std::find(subtypes.begin(), subtypes.end(), codec.subtype) != subtypes.end()) {
            return codec;
        }
    }
    throw std::runtime_error("No writable codec is available for this output format.");
}

template <typename TValue>
TValue Parameter(const std::vector<EncoderParamter>& params, const char* name, TValue fallback)
{
    const auto* param = FindEncoderParamter(params, name);
    if (param == nullptr) {
        return fallback;
    }
    const auto value = param->GetValue<TValue>();
    if (!value) {
        throw std::runtime_error(std::string("Invalid encoder parameter type: ") + name);
    }
    return *value;
}

bool IsFloat(AudioDataFormat format)
{
    return format == AudioDataFormat::Float32 || format == AudioDataFormat::Float64;
}

AudioDataFormat TransferFormat(uint32_t bits, bool floating, bool unsigned8)
{
    if (floating) {
        if (bits == 32) return AudioDataFormat::Float32;
        if (bits == 64) return AudioDataFormat::Float64;
    } else {
        if (bits == 8) return unsigned8 ? AudioDataFormat::PCM_U8 : AudioDataFormat::PCM_S8;
        if (bits == 16) return AudioDataFormat::PCM_S16;
        if (bits == 20 || bits == 24 || bits == 32) return AudioDataFormat::PCM_S32;
        if (bits == 64) return AudioDataFormat::PCM_64;
    }
    throw std::runtime_error("Unsupported PCM bit depth or floating-point combination.");
}

int OutputSubtype(AudioDataFormat format, uint32_t bits)
{
    if (IsFloat(format)) {
        if (bits == 32) return SF_FORMAT_FLOAT;
        if (bits == 64) return SF_FORMAT_DOUBLE;
    } else if (format == AudioDataFormat::PCM_U8 || format == AudioDataFormat::PCM_S8
        || format == AudioDataFormat::PCM_S16 || format == AudioDataFormat::PCM_S24
        || format == AudioDataFormat::PCM_S24_32 || format == AudioDataFormat::PCM_S32
        || format == AudioDataFormat::PCM_64) {
        if (bits == 8) return format == AudioDataFormat::PCM_U8 ? SF_FORMAT_PCM_U8 : SF_FORMAT_PCM_S8;
        if (bits == 16) return SF_FORMAT_PCM_16;
        if (bits == 24) return SF_FORMAT_PCM_24;
        if (bits == 32) return SF_FORMAT_PCM_32;
        throw std::runtime_error("libsndfile cannot write signed 64-bit integer PCM.");
    } else {
        for (const auto& codec : CODEC_MAP) {
            if (codec.format == format) return codec.subtype;
        }
    }
    throw std::runtime_error("Unsupported output codec or bit depth.");
}
}

CEncodeControl::CEncodeControl(uint32_t streamFmt) : m_streamFmt(streamFmt)
{
}

CEncodeControl::~CEncodeControl()
{
    try {
        Close();
    } catch (...) {
        // Callback failures are recorded; destruction must not propagate exceptions.
    }
}

bool CEncodeControl::Fail(const std::string& message)
{
    m_error = message;
    return false;
}

void CEncodeControl::QueryFormatsDefine(std::vector<EncoderFormatDefine>& fmtDesc)
{
    fmtDesc.clear();
    for (const auto& item : FORMAT_MAP) {
        if (Subtypes(item.major).empty()) continue;
        SF_FORMAT_INFO info{ item.major };
        if (sf_command(nullptr, SFC_GET_FORMAT_INFO, &info, sizeof(info)) == 0 && info.name && info.extension) {
            fmtDesc.push_back({ item.streamFmt, info.name, std::string(".") + info.extension });
        }
    }
}

void CEncodeControl::QueryParamtersDefine(uint32_t streamFmt, std::vector<EncoderParamterDefine>& paramDef)
{
    paramDef.clear();
    const int major = MajorFormat(streamFmt);
    const auto subtypes = Subtypes(major);
    const auto& defaultCodec = DefaultCodec(subtypes);
    std::vector<EncoderParamterDefine::ValueType> codecs, bits, nativeSubtypes{ uint32_t(0) };
    for (const auto& codec : CODEC_MAP) {
        if (std::find(subtypes.begin(), subtypes.end(), codec.subtype) == subtypes.end()) continue;
        codecs.emplace_back(static_cast<uint32_t>(codec.format));
        const EncoderParamterDefine::ValueType depth = codec.bits;
        if (std::find(bits.begin(), bits.end(), depth) == bits.end()) bits.push_back(depth);
    }
    for (int subtype : subtypes) nativeSubtypes.emplace_back(static_cast<uint32_t>(subtype));

    paramDef.emplace_back(EncoderParamName::CodeFormat, EncoderParamType::UnsignedInt,
        static_cast<uint32_t>(defaultCodec.format), std::move(codecs));
    paramDef.emplace_back(EncoderParamName::BitsPerSample, EncoderParamType::UnsignedInt, defaultCodec.bits, std::move(bits));
    paramDef.emplace_back(EncoderParamName::SampleRates, EncoderParamType::UnsignedInt, uint32_t(major == SF_FORMAT_WVE ? 8000 : 44100));
    if (major == SF_FORMAT_WVE) paramDef.back().SetUnsignedIntRange(8000, 8000);
    else paramDef.back().SetUnsignedIntRange(1, static_cast<uint32_t>((std::numeric_limits<int>::max)()));
    paramDef.emplace_back(EncoderParamName::Channels, EncoderParamType::UnsignedInt, uint32_t(2));
    uint32_t maxChannels = 1;
    for (uint32_t channels = 2; channels <= 16; ++channels) {
        SF_INFO info{};
        info.samplerate = 44100;
        info.channels = static_cast<int>(channels);
        info.format = major | defaultCodec.subtype;
        if (sf_format_check(&info)) maxChannels = channels;
    }
    paramDef.back().SetUnsignedIntRange(1, maxChannels);
    paramDef.emplace_back(EncoderParamName::SndfileSubtype, EncoderParamType::UnsignedInt, uint32_t(0), std::move(nativeSubtypes));
    if (major == SF_FORMAT_WAV) {
        paramDef.emplace_back(EncoderParamName::ExtWav, EncoderParamType::Bool, false);
    }
    if (major == SF_FORMAT_FLAC) {
        paramDef.emplace_back(EncoderParamName::CompressLevel, EncoderParamType::UnsignedInt, uint32_t(5));
        paramDef.back().SetUnsignedIntRange(0, 8);
    }
    if (major == SF_FORMAT_OGG || major == SF_FORMAT_MPEG) {
        paramDef.emplace_back(EncoderParamName::Quality, EncoderParamType::Float, 0.5);
        paramDef.back().SetFloatRange(0.0, 1.0);
    }
}

bool CEncodeControl::Open(CDataStream* pStream, const std::vector<EncoderParamter>& params)
{
    if (m_file) return Fail("Encoder is already open; close it before opening another stream.");
    m_error.clear();
    m_ioError = false;
    m_stopped = false;
    m_started = false;
    m_stream = nullptr;
    m_inputFmt = {};
    m_transFmt = {};
    try {
        const auto required = dsStyleWritable | dsStyleSeekable | dsStyleTellPos;
        if (!pStream || (pStream->GetStyle() & required) != required) {
            return Fail("Encoding requires a writable, seekable stream with position reporting.");
        }
        if (pStream->GetLength() != 0 || pStream->Tell() != 0) {
            return Fail("Output stream must be empty and positioned at its beginning.");
        }
        int major = MajorFormat(m_streamFmt);
        if (m_streamFmt == StreamFormatWav && Parameter<bool>(params, EncoderParamName::ExtWav, false)) major = SF_FORMAT_WAVEX;
        const auto& defaultCodec = DefaultCodec(Subtypes(major));
        const auto format = static_cast<AudioDataFormat>(Parameter<uint32_t>(params, EncoderParamName::CodeFormat,
            static_cast<uint32_t>(defaultCodec.format)));
        if (format != AudioDataFormat::PCM_64 && format != AudioDataFormat::PCM_S24_32
            && std::none_of(std::begin(CODEC_MAP), std::end(CODEC_MAP), [format](const auto& codec) { return codec.format == format; })) {
            return Fail("Unknown output CodeFormat.");
        }
        uint32_t defaultBits = defaultCodec.bits;
        for (const auto& codec : CODEC_MAP) {
            if (codec.format == format) defaultBits = codec.bits;
        }
        const auto bits = Parameter<uint32_t>(params, EncoderParamName::BitsPerSample, defaultBits);
        const auto rate = Parameter<uint32_t>(params, EncoderParamName::SampleRates, major == SF_FORMAT_WVE ? 8000 : 44100);
        const auto channels = Parameter<uint32_t>(params, EncoderParamName::Channels, 2);
        const auto nativeSubtype = Parameter<uint32_t>(params, EncoderParamName::SndfileSubtype, 0);
        if (rate == 0 || rate > INT_MAX || channels == 0 || channels > 1024 || nativeSubtype > SF_FORMAT_SUBMASK) {
            return Fail("Invalid sample rate, channel count or libsndfile subtype.");
        }
        const auto available = Subtypes(major);
        int subtype = nativeSubtype ? static_cast<int>(nativeSubtype) : OutputSubtype(format, bits);
        if (!nativeSubtype && bits == 8 && subtype == SF_FORMAT_PCM_S8 && format != AudioDataFormat::PCM_S8
            && std::find(available.begin(), available.end(), SF_FORMAT_PCM_S8) == available.end()
            && std::find(available.begin(), available.end(), SF_FORMAT_PCM_U8) != available.end()) {
            subtype = SF_FORMAT_PCM_U8;
        }
        SF_INFO info{};
        info.samplerate = static_cast<int>(rate);
        info.channels = static_cast<int>(channels);
        info.format = major | subtype;
        if (std::find(available.begin(), available.end(), subtype) == available.end() || !sf_format_check(&info)) {
            return Fail("Unsupported container, codec, sample rate or channel combination.");
        }
        const auto transFormat = TransferFormat(bits, IsFloat(format) || subtype == SF_FORMAT_FLOAT || subtype == SF_FORMAT_DOUBLE,
            subtype == SF_FORMAT_PCM_U8);
        InitAudioFormat(&m_transFmt, transFormat, channels, rate);

        double quality = Parameter<double>(params, EncoderParamName::Quality, 0.5);
        const auto compression = Parameter<uint32_t>(params, EncoderParamName::CompressLevel, 5);
        if (!std::isfinite(quality) || quality < 0 || quality > 1 || compression > 8) {
            m_transFmt = {};
            return Fail("Quality must be in [0, 1] and compression level in [0, 8].");
        }
        m_stream = pStream;
        SF_VIRTUAL_IO io{ GetLengthCb, SeekCb, ReadCb, WriteCb, TellCb };
        m_file = sf_open_virtual(&io, SFM_WRITE, &info, this);
        if (!m_file || m_ioError) {
            const std::string message = m_ioError ? "Output stream I/O failed while opening encoder." : sf_strerror(nullptr);
            Close();
            return Fail(message);
        }
        if (info.samplerate != static_cast<int>(rate) || info.channels != static_cast<int>(channels)) {
            Close();
            return Fail("Codec changed the requested sample rate or channel count.");
        }
        // Integer PCM is full-scale, whereas floating PCM is normalized to [-1, 1].
        sf_command(m_file, SFC_SET_SCALE_INT_FLOAT_WRITE, nullptr, SF_TRUE);
        sf_command(m_file, SFC_SET_CLIPPING, nullptr, SF_TRUE);
        if (major == SF_FORMAT_FLAC) {
            double level = compression / 8.0;
            if (sf_command(m_file, SFC_SET_COMPRESSION_LEVEL, &level, sizeof(level)) != SF_TRUE) {
                Close();
                return Fail("Failed to set compression level.");
            }
        }
        if (major == SF_FORMAT_OGG || major == SF_FORMAT_MPEG) {
            const int command = subtype == SF_FORMAT_OPUS ? SFC_SET_COMPRESSION_LEVEL : SFC_SET_VBR_ENCODING_QUALITY;
            if (subtype == SF_FORMAT_OPUS) quality = 1.0 - quality;
            if (sf_command(m_file, command, &quality, sizeof(quality)) != SF_TRUE) {
                Close();
                return Fail("Failed to set encoding quality.");
            }
        }
        ApplyMetaInfo();
        return true;
    } catch (const std::exception& e) {
        const std::string message = e.what();
        Close();
        return Fail(message);
    }
}

void CEncodeControl::Close()
{
    if (m_file) Stop();
    m_stream = nullptr;
    m_started = false;
    m_stopped = false;
    m_transFmt = {};
    m_inputFmt = {};
}

void CEncodeControl::SetMetaInfo(uint32_t streamIdx, const CMediaTag& metaTags)
{
    if (streamIdx != 0) throw std::runtime_error("libsndfile supports only media stream zero.");
    m_tags = metaTags;
    ApplyMetaInfo();
}

void CEncodeControl::ApplyMetaInfo()
{
    if (!m_file) return;
    const std::pair<const char*, int> mapping[] = {
        { MediaTag_Title, SF_STR_TITLE }, { MediaTag_Artists, SF_STR_ARTIST },
        { MediaTag_Album, SF_STR_ALBUM }, { MediaTag_Comment, SF_STR_COMMENT },
        { MediaTag_Date, SF_STR_DATE }, { MediaTag_Tracks, SF_STR_TRACKNUMBER },
        { MediaTag_Genre, SF_STR_GENRE }, { MediaTag_Encoder, SF_STR_SOFTWARE }
    };
    for (const auto& [name, type] : mapping) {
        if (const auto value = m_tags.QueryTagString(name)) {
            // Metadata is optional: not all containers support every string type.
            sf_set_string(m_file, type, value->c_str());
        } else if (const auto value = m_tags.QueryTagInteger(name)) {
            sf_set_string(m_file, type, std::to_string(*value).c_str());
        }
    }
}

AudioFormat CEncodeControl::GetTransFormat() const
{
    return m_transFmt;
}

bool CEncodeControl::Start(const AudioFormat* audioFmt)
{
    m_error.clear();
    if (!m_file || m_started || m_ioError || !audioFmt) return Fail("Encoder is not ready to start.");
    if (audioFmt->sampleRate != m_transFmt.sampleRate || audioFmt->numChannels != m_transFmt.numChannels
        || audioFmt->chLayout != m_transFmt.chLayout) {
        return Fail("Input sample rate, channels or channel layout does not match output parameters.");
    }
    switch (audioFmt->format) {
    case AudioDataFormat::PCM_U8: case AudioDataFormat::PCM_S8:
    case AudioDataFormat::PCM_S16: case AudioDataFormat::PCM_S32:
    case AudioDataFormat::PCM_64: case AudioDataFormat::Float32: case AudioDataFormat::Float64:
        break;
    default: return Fail("Input must be interleaved PCM; packed 24-bit input is not supported.");
    }
    const auto storageBits = GetBitsPerSampleByFormat(audioFmt->format);
    if (audioFmt->bitsPerSample != storageBits || audioFmt->blockAlign != storageBits / 8 * audioFmt->numChannels) {
        return Fail("Invalid input PCM storage size or block alignment.");
    }
    m_inputFmt = *audioFmt;
    m_started = true;
    return true;
}

uint32_t CEncodeControl::Encode(const void* pData, uint32_t frames)
{
    m_error.clear();
    if (!m_file || !m_started || m_ioError || (!pData && frames)) {
        Fail("Encoder is not started, input is invalid or output stream has failed.");
        return 0;
    }
    if (frames == 0) return 0;
    if (frames > (std::numeric_limits<std::size_t>::max)() / m_inputFmt.blockAlign) {
        Fail("PCM buffer size exceeds addressable memory.");
        return 0;
    }
    sf_count_t written = 0;
    const auto samples = static_cast<std::size_t>(frames) * m_inputFmt.numChannels;
    switch (m_inputFmt.format) {
    case AudioDataFormat::PCM_S16: written = sf_writef_short(m_file, static_cast<const short*>(pData), frames); break;
    case AudioDataFormat::PCM_S32: written = sf_writef_int(m_file, static_cast<const int*>(pData), frames); break;
    case AudioDataFormat::Float32: written = sf_writef_float(m_file, static_cast<const float*>(pData), frames); break;
    case AudioDataFormat::Float64: written = sf_writef_double(m_file, static_cast<const double*>(pData), frames); break;
    case AudioDataFormat::PCM_U8:
    case AudioDataFormat::PCM_S8: {
        std::vector<short> converted(samples);
        for (std::size_t i = 0; i < samples; ++i) {
            const int value = m_inputFmt.format == AudioDataFormat::PCM_U8
                ? static_cast<const uint8_t*>(pData)[i] - 128 : static_cast<const int8_t*>(pData)[i];
            converted[i] = static_cast<short>(value * 256);
        }
        written = sf_writef_short(m_file, converted.data(), frames);
        break;
    }
    case AudioDataFormat::PCM_64: {
        std::vector<double> converted(samples);
        for (std::size_t i = 0; i < samples; ++i) converted[i] = std::ldexp(static_cast<double>(static_cast<const int64_t*>(pData)[i]), -63);
        written = sf_writef_double(m_file, converted.data(), frames);
        break;
    }
    default: break;
    }
    if (m_ioError || written != frames) Fail(m_ioError ? "Output stream write failed." : sf_strerror(m_file));
    return written > 0 ? static_cast<uint32_t>(written) : 0;
}

bool CEncodeControl::Stop()
{
    if (!m_file) return m_stopped ? m_error.empty() : Fail("Encoder is not open.");
    m_started = false;
    SNDFILE* file = m_file;
    m_file = nullptr;
    const int error = sf_close(file);
    m_stopped = true;
    if (m_ioError || error != SF_ERR_NO_ERROR) return Fail(m_ioError ? "Output stream I/O failed while finalizing file." : sf_error_number(error));
    return m_error.empty();
}

sf_count_t CEncodeControl::GetLengthCb(void* userData) noexcept
{
    auto& self = *static_cast<CEncodeControl*>(userData);
    try {
        const auto length = self.m_stream->GetLength();
        if (length <= static_cast<uint64_t>(SF_COUNT_MAX)) return static_cast<sf_count_t>(length);
    } catch (...) {}
    self.m_ioError = true;
    return -1;
}

sf_count_t CEncodeControl::TellCb(void* userData) noexcept
{
    auto& self = *static_cast<CEncodeControl*>(userData);
    try {
        const auto position = self.m_stream->Tell();
        if (position <= static_cast<uint64_t>(SF_COUNT_MAX)) return static_cast<sf_count_t>(position);
    } catch (...) {}
    self.m_ioError = true;
    return -1;
}

sf_count_t CEncodeControl::SeekCb(sf_count_t offset, int whence, void* userData) noexcept
{
    auto& self = *static_cast<CEncodeControl*>(userData);
    try {
        sf_count_t base = 0;
        if (whence == SEEK_CUR) base = TellCb(userData);
        else if (whence == SEEK_END) base = GetLengthCb(userData);
        else if (whence != SEEK_SET) { self.m_ioError = true; return -1; }
        if (base < 0 || offset < -base || (offset > 0 && base > SF_COUNT_MAX - offset)) {
            self.m_ioError = true;
            return -1;
        }
        const auto target = base + offset;
        self.m_stream->Seek(SeekBase::Begin, target);
        if (TellCb(userData) == target) return target;
    } catch (...) {}
    self.m_ioError = true;
    return -1;
}

sf_count_t CEncodeControl::ReadCb(void* ptr, sf_count_t count, void* userData) noexcept
{
    auto& self = *static_cast<CEncodeControl*>(userData);
    sf_count_t total = 0;
    try {
        while (total < count) {
            const auto chunk = static_cast<uint32_t>((std::min)(count - total, static_cast<sf_count_t>(UINT32_MAX)));
            const auto read = self.m_stream->Read(static_cast<char*>(ptr) + total, chunk);
            if (read > chunk) { self.m_ioError = true; break; }
            if (read == 0) break;
            total += read;
        }
    } catch (...) { self.m_ioError = true; }
    return total;
}

sf_count_t CEncodeControl::WriteCb(const void* ptr, sf_count_t count, void* userData) noexcept
{
    auto& self = *static_cast<CEncodeControl*>(userData);
    sf_count_t total = 0;
    try {
        while (total < count) {
            const auto chunk = static_cast<uint32_t>((std::min)(count - total, static_cast<sf_count_t>(UINT32_MAX)));
            const auto written = self.m_stream->Write(static_cast<const char*>(ptr) + total, chunk);
            if (written == 0 || written > chunk) { self.m_ioError = true; break; }
            total += written;
        }
    } catch (...) { self.m_ioError = true; }
    return total;
}
