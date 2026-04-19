#include <fstream>
#include <string_view>
#include <format>
#include "NativeDecoder.h"
#include "ConvertFormat.h"
#include "UnicodeConvert.h"
#include "SndfileFunc.h"
#include "nlohmann/json.hpp" 

using json = nlohmann::json;

static std::string AdaptiveSfString(const char* sfString)
{
    std::string result;

    if (sfString != nullptr)
    {
        result = AdaptiveToUtf8(sfString);
    }

    return result;
}

std::string FlacExtraInfoToJson(SNDFILE* dnsFile, uint32_t type, const AudioFormat& fmt, double totalSeconds)
{
    std::string brief = std::format("{}: {} {} {} channels",
        SndfileGetFileTypeName(type), SampleRateBrifStr(fmt.sampleRate), StringFromAudioFormat(fmt.format), ChannelBrifName(fmt.numChannels, fmt.chLayout));

    const char *pTitle = sf_get_string(dnsFile, SF_STR_TITLE);
    const char* pArtist = sf_get_string(dnsFile, SF_STR_ARTIST);
    const char* pGener = sf_get_string(dnsFile, SF_STR_GENRE);
    const char* pAlbum = sf_get_string(dnsFile, SF_STR_ALBUM);
    const char* pDate = sf_get_string(dnsFile, SF_STR_DATE);
    const char* pTracks = sf_get_string(dnsFile, SF_STR_TRACKNUMBER);
    const char* pComment = sf_get_string(dnsFile, SF_STR_COMMENT);
    
    std::string_view strtype = SndfileGetFileTypeName(type);
    json info = {
        {"type", strtype},
        {"sample_rate", fmt.sampleRate },
        {"bits_per_sample", fmt.bytesPerSample * 8 },
        {"channels", fmt.numChannels },
        {"channels_layout", fmt.chLayout },
        {"bitrate", fmt.bytesPerSample * 8 * fmt.sampleRate },
        {"length", totalSeconds},
        {"year", AdaptiveSfString(pDate)},
        {"artist", AdaptiveSfString(pArtist)},
        {"title", AdaptiveSfString(pTitle)},
        {"album", AdaptiveSfString(pAlbum)},
        {"gener", AdaptiveSfString(pGener)},
        {"tracks", AdaptiveSfString(pTracks)},
        {"comment", AdaptiveSfString(pComment)},
        {"brief", brief}
    };

    return info.dump(4);
}

static sf_count_t StreamGetFileLen(void* stream) 
{
    CDataStream* pStream = static_cast<CDataStream*>(stream);
    return static_cast<sf_count_t>(pStream->GetLength());
}

static sf_count_t StreamSeek(sf_count_t offset, int whence, void* stream)
{
    CDataStream* pStream = static_cast<CDataStream*>(stream);

    SeekBase base;
    switch (whence) {
    case SEEK_SET: base = SeekBase::Begin; break;
    case SEEK_CUR: base = SeekBase::Cur; break;
    case SEEK_END: base = SeekBase::End; break;
    default: return -1;
    }

    try {
        pStream->Seek(base, offset);
        return static_cast<sf_count_t>(pStream->Tell());
    }
    catch (...) {
        return -1;
    }
}

static sf_count_t StreamRead(void* ptr, sf_count_t count, void* stream)
{
    CDataStream* pStream = static_cast<CDataStream*>(stream);

    uint32_t readed = pStream->Read(ptr, (uint32_t)count);

    return readed;
}

static sf_count_t StreamWrite(const void* ptr, sf_count_t count, void* stream)
{
    CDataStream* pStream = static_cast<CDataStream*>(stream);

    return 0;
}

static sf_count_t StreamTell(void* stream) 
{
    CDataStream* pStream = static_cast<CDataStream*>(stream);
    return static_cast<sf_count_t>(pStream->Tell());
}

static SF_VIRTUAL_IO CreateStreamVirtualIO() 
{
    SF_VIRTUAL_IO vio = { 0 };

    vio.get_filelen = StreamGetFileLen;
    vio.seek = StreamSeek;
    vio.read = StreamRead;
    vio.write = nullptr;
    vio.tell = StreamTell;

    return vio;
}

CNativeDecoder::CNativeDecoder(uint32_t streamFmt)
{
    m_streamFmt = streamFmt;
    m_NativeType = 0;
    InitEmptyAudioFormat(&m_AudioFmt);
    m_curFrames = 0;
    m_file = nullptr;
    m_name = "Native libsndfile Decoder";
    m_type = DECODE_TYPE_NATIVE;
}

CNativeDecoder::~CNativeDecoder()
{
    if (m_file != nullptr)
    {
        sf_close(m_file);
    }
}

void QueryChannelLayout(SNDFILE* sf, AudioFormat& audioFmt)
{
    uint32_t mask = 0;
    int cmd_ch = (audioFmt.numChannels > 64) ? 64 : audioFmt.numChannels;
    int ch_map[64] = { 0 };
    if (sf_command(sf, SFC_GET_CHANNEL_MAP_INFO, ch_map, cmd_ch * sizeof(int)))
    {
        for (int i = 0; i < cmd_ch; i++)
        {
            switch (ch_map[i])
            {
            case SF_CHANNEL_MAP_MONO:  mask |= CHANNEL_FRONT_CENTER;  break;
            case SF_CHANNEL_MAP_LEFT:  mask |= CHANNEL_FRONT_LEFT;  break;
            case SF_CHANNEL_MAP_RIGHT:  mask |= CHANNEL_FRONT_RIGHT;  break;
            case SF_CHANNEL_MAP_CENTER:  mask |= CHANNEL_FRONT_CENTER;  break;
            case SF_CHANNEL_MAP_LFE: mask |= CHANNEL_LOW_FREQUENCY;  break;
            case SF_CHANNEL_MAP_REAR_LEFT: mask |= CHANNEL_BACK_LEFT; break;
            case SF_CHANNEL_MAP_REAR_RIGHT: mask |= CHANNEL_BACK_RIGHT; break;
            case SF_CHANNEL_MAP_SIDE_LEFT: mask |= CHANNEL_SIDE_LEFT; break;
            case SF_CHANNEL_MAP_SIDE_RIGHT: mask |= CHANNEL_SIDE_RIGHT; break;
            case SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER: mask |= CHANNEL_FRONT_LEFT_OF_CENTER; break;
            case SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER: mask |= CHANNEL_FRONT_RIGHT_OF_CENTER; break;
            case SF_CHANNEL_MAP_REAR_CENTER: mask |= CHANNEL_BACK_CENTER; break;
            case SF_CHANNEL_MAP_TOP_CENTER: mask |= CHANNEL_TOP_CENTER; break;
            case SF_CHANNEL_MAP_TOP_FRONT_LEFT: mask |= CHANNEL_TOP_FRONT_LEFT; break;
            case SF_CHANNEL_MAP_TOP_FRONT_CENTER: mask |= CHANNEL_TOP_FRONT_CENTER; break;
            case SF_CHANNEL_MAP_TOP_FRONT_RIGHT: mask |= CHANNEL_TOP_FRONT_RIGHT; break;
            case SF_CHANNEL_MAP_TOP_REAR_LEFT: mask |= CHANNEL_TOP_BACK_LEFT; break;
            case SF_CHANNEL_MAP_TOP_REAR_CENTER: mask |= CHANNEL_TOP_BACK_CENTER; break;
            case SF_CHANNEL_MAP_TOP_REAR_RIGHT: mask |= CHANNEL_TOP_BACK_RIGHT; break;
            default: break;
            }
        }
    }
    else
    {
        mask = StandLayoutByChannelsCount(audioFmt.numChannels);
    }

    audioFmt.chLayout = mask;
}
AudioInfo CNativeDecoder::InitDecode(const CDecodeInitCtx* decodeInit)
{
    assert(m_pStream != nullptr);

    AudioInfo audioInfo;
    SF_INFO sfinfo = { 0 };
    SF_VIRTUAL_IO vio = CreateStreamVirtualIO();
    m_file = sf_open_virtual(&vio, SFM_READ, &sfinfo, m_pStream);
    if (m_file == nullptr)
        throw std::runtime_error(sf_strerror(NULL));

    // 解析格式信息
    m_NativeType = sfinfo.format & SF_FORMAT_TYPEMASK;
    m_AudioFmt.format = SndfileTransSubType(sfinfo.format & SF_FORMAT_SUBMASK);
    if(m_AudioFmt.format == AudioDataFormat::UNKNOWN)
        throw std::runtime_error("Unkonwn flac file audio format!");

    m_AudioFmt.numChannels = sfinfo.channels;
    m_AudioFmt.sampleRate = sfinfo.samplerate;
    m_AudioFmt.bytesPerSample = GetBitsPerSampleByFormat(m_AudioFmt.format) / 8;
    m_AudioFmt.blockAlign = m_AudioFmt.bytesPerSample * m_AudioFmt.numChannels;
    QueryChannelLayout(m_file, m_AudioFmt);

    audioInfo.m_audioFmt = m_AudioFmt;
    audioInfo.m_totalFrames = sfinfo.frames;
    //uint32_t frameSize = audioInfo.m_audioFmt.bytesPerSample * audioInfo.m_audioFmt.numChannels;

    m_totalFrames = audioInfo.m_totalFrames;

    audioInfo.extraInfo = FlacExtraInfoToJson(m_file, m_NativeType, audioInfo.m_audioFmt, double(m_totalFrames) / sfinfo.samplerate);

    return audioInfo;
}

uint32_t CNativeDecoder::Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_curFrames >= m_totalFrames)
        return 0;

    sf_seek(m_file, m_curFrames, SEEK_SET);
    uint32_t readFrames = 0;
    if (audioFmt->format == AudioDataFormat::Float32)
        readFrames = (uint32_t)sf_readf_float(m_file, (float*)pBuf, frames);
    else if (audioFmt->format == AudioDataFormat::PCM_S16)
        readFrames = (uint32_t)sf_readf_short(m_file, (short*)pBuf, frames);
    else if (audioFmt->format == AudioDataFormat::PCM_S32)
        readFrames = (uint32_t)sf_readf_int(m_file, (int*)pBuf, frames);
    else if (audioFmt->format == AudioDataFormat::Float64)
        readFrames = (uint32_t)sf_readf_double(m_file, (double*)pBuf, frames);
    else
        readFrames = 0;

    if (readFrames == 0)
    {
        if (sf_error(m_file) == SF_ERR_NO_ERROR) 
        {
            m_curFrames = m_totalFrames;
            //sf_count_t current_pos = sf_seek(m_file, 0, SEEK_CUR);
        }
    }
    else
        m_curFrames += readFrames;

    return readFrames;
}

void CNativeDecoder::SeekTo(std::size_t frames)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    m_curFrames = (frames <= m_totalFrames) ? frames : m_totalFrames;
}

bool CNativeDecoder::IsSupportOutput(const AudioFormat* audioFmt) const
{
    //支持 PCM_S16、PCM_S32、Float32 和 Float64 输出格式
    if ((audioFmt->format != AudioDataFormat::PCM_S16) && (audioFmt->format != AudioDataFormat::PCM_S32)
        && (audioFmt->format != AudioDataFormat::Float32) && (audioFmt->format != AudioDataFormat::Float64))
    {
        return false;
    }

    //目前不支持采样率转换输出、声道转换
    if (audioFmt->sampleRate != m_AudioFmt.sampleRate)
        return false;
    if (audioFmt->numChannels != m_AudioFmt.numChannels)
        return false;

    return true;
}

uint32_t LibSndfileQueryFileType(const std::wstring& filename)
{
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    std::string local_name = Utf16LeToLocalMBCS(filename);
    SNDFILE* file = sf_open(local_name.c_str(), SFM_READ, &sfinfo);
    if (!file) 
        return false;

    uint32_t nativeFmt = StreamFormatUnknown;
    uint32_t subType = sfinfo.format & SF_FORMAT_TYPEMASK;
    switch (subType)
    {
        case SF_FORMAT_AIFF: nativeFmt = StreamFormatAiff; break;
        case SF_FORMAT_AU: nativeFmt = StreamFormatAu; break;
        case SF_FORMAT_RAW: nativeFmt = StreamFormatRaw; break;
        case SF_FORMAT_WAV: nativeFmt = StreamFormatWav; break;
        case SF_FORMAT_VOC: nativeFmt = StreamFormatVoc; break;
        case SF_FORMAT_FLAC: nativeFmt = StreamFormatFlac; break;
        case SF_FORMAT_CAF: nativeFmt = StreamFormatCaf; break;
        case SF_FORMAT_OGG: nativeFmt = StreamFormatOgg; break;
        case SF_FORMAT_MPEG: nativeFmt = StreamFormatMp3; break;
        case SF_FORMAT_WVE: nativeFmt = StreamFormatWve; break;
        case SF_FORMAT_RF64: nativeFmt = StreamFormatWav64; break;
        case SF_FORMAT_W64: nativeFmt = StreamFormatW64; break;
        case SF_FORMAT_WAVEX: nativeFmt = StreamFormatWav; break;
        case SF_FORMAT_SDS: nativeFmt = StreamFormatMidSds; break;
        case SF_FORMAT_PAF: nativeFmt = StreamFormatPaf; break;
        case SF_FORMAT_SVX: nativeFmt = StreamFormatSvx; break;
        case SF_FORMAT_NIST: nativeFmt = StreamFormatNist; break;
        case SF_FORMAT_IRCAM: nativeFmt = StreamFormatIrcam; break;
        case SF_FORMAT_MAT4: nativeFmt = StreamFormatMat4; break;
        case SF_FORMAT_MAT5: nativeFmt = StreamFormatMat5; break;
        case SF_FORMAT_PVF: nativeFmt = StreamFormatPvf; break;
        case SF_FORMAT_XI: nativeFmt = StreamFormatXi; break;
        case SF_FORMAT_HTK: nativeFmt = StreamFormatHtk; break;
        case SF_FORMAT_AVR: nativeFmt = StreamFormatAvr; break;
        case SF_FORMAT_SD2: nativeFmt = StreamFormatSd2; break;
        case SF_FORMAT_MPC2K: nativeFmt = StreamFormatMPC2k; break;
        default: nativeFmt = StreamFormatUnknown; break;
    }

    sf_close(file);

    return nativeFmt;
}

