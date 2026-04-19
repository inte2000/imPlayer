#include <fstream>
#include <string_view>
#include <format>
#include "NativeEncoder.h"
#include "ConvertFormat.h"
#include "UnicodeConvert.h"
#include "SndfileFunc.h"
#include "nlohmann/json.hpp" 

using json = nlohmann::json;


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

    uint32_t written = pStream->Write(ptr, (uint32_t)count);

    return written;
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
    vio.write = StreamWrite;
    vio.tell = StreamTell;

    return vio;
}

CNativeEncoder::CNativeEncoder(uint32_t streamFmt)
{
    m_streamFmt = streamFmt;
    m_file = nullptr;
    m_name = "Native audio encoder";
}

CNativeEncoder::~CNativeEncoder()
{
    if (m_file != nullptr)
    {
        sf_close(m_file);
    }
}

bool CNativeEncoder::Init(const std::string& jsonParams)
{
    ParseJsonParamsstring(jsonParams, m_encodingParam);

    SF_INFO sfinfo = { 0 };
    sfinfo.channels = m_encodingParam.channels;
    sfinfo.samplerate = m_encodingParam.sampleRate;
    sfinfo.format = SndfileStreamFmtToFileType(m_streamFmt) | SndfileGetSubType(m_encodingParam.format);

    SF_VIRTUAL_IO vio = CreateStreamVirtualIO();
    m_file = sf_open_virtual(&vio, SFM_WRITE, &sfinfo, m_pStream);
    if (m_file == nullptr)
        throw std::runtime_error("Fail to open generator file!");

    //if (m_encodingParam.bitsPerSample <= 16)
    {
        int enable = SF_TRUE;
        sf_command(m_file, SFC_SET_CLIPPING, &enable, sizeof(enable));
    }
    if ((m_streamFmt == StreamFormatOgg) && (m_encodingParam.mask & ParaMask_OggVbrQuality))
    {
        double quality = m_encodingParam.oggVbrQuality;  // 相当于中等质量
        sf_command(m_file, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(quality));
    }
    if ((m_streamFmt == StreamFormatFlac) || (m_streamFmt == StreamFormatOgg))
    {
        int compression = m_encodingParam.CompressLevel; // 0-8
        sf_command(m_file, SFC_SET_COMPRESSION_LEVEL, &compression, sizeof(compression));
    }

    AttachMetaInfo(m_file, m_metaInfo);

    return true;
}

void CNativeEncoder::SetMetaInfo(const std::string& jsonMeta)
{
    m_metaInfo = ParseBaseMetaInfo(jsonMeta);
}

std::vector<EncoderParamterDef> CNativeEncoder::QueryParamtersDefine() const
{
    return GetParamtersTypeInfo(m_streamFmt);
}

uint32_t CNativeEncoder::Encode(const void* pData, uint32_t frames, const AudioFormat* audioFmt)
{
    if ((audioFmt->sampleRate != m_encodingParam.sampleRate) || (audioFmt->numChannels != m_encodingParam.channels))
        throw std::runtime_error("Encoder not support sample rate or channels converting!");
    //return 0; //不支持采样率和声道的转换

    sf_count_t written = 0;
    if(audioFmt->format == AudioDataFormat::Float32)
        written = sf_writef_float(m_file, static_cast<const float*>(pData), frames);
    else if (audioFmt->format == AudioDataFormat::Float64)
        written = sf_writef_double(m_file, static_cast<const double*>(pData), frames);
    else if (audioFmt->format == AudioDataFormat::PCM_S16)
        written = sf_writef_short(m_file, static_cast<const short*>(pData), frames);
    else if ((audioFmt->format == AudioDataFormat::PCM_S24) || (audioFmt->format == AudioDataFormat::PCM_S32))
        written = sf_writef_int(m_file, static_cast<const int*>(pData), frames);
    else if ((audioFmt->format == AudioDataFormat::PCM_S8) || (audioFmt->format == AudioDataFormat::PCM_U8)
             || (audioFmt->format == AudioDataFormat::Ulaw) || (audioFmt->format == AudioDataFormat::Alaw))
    {
        uint32_t bytesCount = frames * audioFmt->numChannels * audioFmt->bytesPerSample;
        auto writtenBytes = sf_write_raw(m_file, pData, bytesCount);
        written = (writtenBytes / audioFmt->bytesPerSample) / audioFmt->numChannels;
    }
    else if ((audioFmt->format == AudioDataFormat::MsAdpcm) || (audioFmt->format == AudioDataFormat::ImaAdpcm))
    {
        uint32_t bytesCount = frames * audioFmt->numChannels / 2;
        auto writtenBytes = sf_write_raw(m_file, pData, bytesCount);
        written = (2 * writtenBytes / audioFmt->numChannels);
    }
    else
    {
        throw std::runtime_error("Data source format not support by encoder");
    }

    if ((frames > 0) && (written == 0))
        throw std::runtime_error(sf_strerror(m_file));
    
    return static_cast<uint32_t>(written);
}

bool CNativeEncoder::IsSupportFormat(uint32_t mediaFmt) const
{
    return true;
}

void CNativeEncoder::AttachMetaInfo(SNDFILE* sf_file, const MediaBaseMetaInfo& metaInfo)
{
    if(!metaInfo.title.empty())
        sf_set_string(sf_file, SF_STR_TITLE, metaInfo.title.c_str());
    if (!metaInfo.artist.empty())
        sf_set_string(sf_file, SF_STR_ARTIST, metaInfo.artist.c_str());
    if (!metaInfo.album.empty())
        sf_set_string(sf_file, SF_STR_ALBUM, metaInfo.album.c_str());
    if (!metaInfo.year.empty())
        sf_set_string(sf_file, SF_STR_DATE, metaInfo.year.c_str());
    if (!metaInfo.genre.empty())
        sf_set_string(sf_file, SF_STR_GENRE, metaInfo.genre.c_str());
    if (!metaInfo.tracks.empty())
        sf_set_string(sf_file, SF_STR_TRACKNUMBER, metaInfo.tracks.c_str());
        
    sf_set_string(sf_file, SF_STR_SOFTWARE, m_name.c_str());
    //sf_set_string(sf_file, SF_STR_COMMENT, metaInfo..c_str());
}
