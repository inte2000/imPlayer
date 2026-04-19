
#include "Mp3Handle.h"
#include "Utf8String.h"
#include "StringEx.h"
//#include "UnicodeNormalize.h"

const char * id3v1_genres[] = 
{
    "Blues",
    "ClassicRock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop ",
    "Jazz",
    "Metal",
    "NewAge",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "DeathMetal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno ",
    "Ambient",
    "Trip-Hop ",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "SoundClip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "InstrumentalPop",
    "InstrumentalRock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial ",
    "Electronic",
    "Pop-Folk ",
    "Eurodance",
    "Dream",
    "SouthernRock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top40",
    "ChristianRap",
    "Pop/Funk ",
    "Jungle",
    "NativeAmerican",
    "Cabaret",
    "NewWave",
    "Psychadelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi ",
    "Tribal",
    "AcidPunk",
    "AcidJazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock& amp; Roll",
    "HardRock",
    "Folk",
    "Folk-Rock ",
    "NationalFolk",
    "Swing",
    "FastFusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "GothicRock",
    "ProgessiveRock",
    "PsychedelicRock",
    "SymphonicRock",
    "SlowRock",
    "BigBand",
    "Chorus",
    "EasyListening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "ChamberMusic",
    "Sonata",
    "Symphony",
    "BootyBass",
    "Primus",
    "PornGroove",
    "Satire",
    "SlowJam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "PowerBallad",
    "RhythmicSoul",
    "Freestyle",
    "Duet",
    "PunkRock",
    "DrumSolo",
    "Acapella",
    "Euro-House ",
    "DanceHall",
    "Goa",
    "Drum& amp; Bass",
    "Club-House ",
    "Hardcore",
    "Terror",
    "Indie",
    "BritPop",
    "Negerpunk",
    "PolskPunk",
    "Beat",
    "ChristianGangstaRap",
    "HeavyMetal",
    "BlackMetal",
    "Crossover",
    "ContemporaryChristian",
    "ChristianRock",
    "Merengue",
    "Salsa",
    "TrashMetal",
    "Anime",
    "JPop",
    "Synthpop"
};

CMp3Handle::CMp3Handle()
{
    m_hMpg12 = nullptr;
    m_decodeDone = false;
    int valid = mpg123_init();
    if (valid == MPG123_OK)
    {
        //m_hMpg12 = mpg123_new(nullptr, nullptr);
        //20250814 orbit
        //遇到数据有错误的 mp3 流，增加同步尝试的限制，默认是 1024，遇到某些连续错误比较多的流，失败率比较高，所以将这个限制放大一点。
        mpg123_pars* mp = mpg123_new_pars(nullptr);
        valid = mpg123_par2(mp, MPG123_RESYNC_LIMIT, 128 * 1024, 0.0);
        m_hMpg12 = mpg123_parnew(mp, nullptr, nullptr);
        mpg123_delete_pars(mp);
    }
}

CMp3Handle::~CMp3Handle()
{
    if (m_hMpg12 != nullptr)
    {
        mpg123_close(m_hMpg12);
        mpg123_delete(m_hMpg12);
    }
}

bool CMp3Handle::OpenFile(const std::string& filename)
{
    if (m_hMpg12 == nullptr)
        return false;

    int valid = mpg123_open(m_hMpg12, filename.c_str());
    return (valid == MPG123_OK);
}

static int StreamRead(void* user_data, void* buffer, std::size_t size, std::size_t *readed)
{
    CDataStream* pStream = static_cast<CDataStream*>(user_data);
    uint32_t rcount = pStream->Read(buffer, static_cast<uint32_t>(size));
    *readed = static_cast<mpg123_ssize_t>(rcount);
    return MPG123_OK;
}

#ifdef off_t
#undef off_t
typedef long long off_t;
//#define off_t long long
#endif

int64_t StreamSeek(void* user_data, int64_t offset, int whence)
{
    auto* stream = static_cast<CDataStream*>(user_data);

    SeekBase base;
    switch (whence) {
    case SEEK_SET: base = SeekBase::Begin; break;
    case SEEK_CUR: base = SeekBase::Cur; break;
    case SEEK_END: base = SeekBase::End; break;
    default: return -1;
    }

    try {
        stream->Seek(base, offset);
        return static_cast<int64_t>(stream->Tell());
    }
    catch (...) {
        return -1;
    }
}

void StreamCleanup(void* user_data) 
{
    // 不需要释放 CDataStream*，由外部管理
}

AudioDataFormat AudioFormatFromMpg123Encoding(long encoding)
{
    AudioDataFormat format = AudioDataFormat::UNKNOWN;
    switch (encoding)
    {
    case MPG123_ENC_SIGNED_8:
        format = AudioDataFormat::PCM_S8;
        break;
    case MPG123_ENC_UNSIGNED_8:
        format = AudioDataFormat::PCM_U8;
        break;
    case MPG123_ENC_SIGNED_16:
        format = AudioDataFormat::PCM_S16;
        break;
    case MPG123_ENC_SIGNED_24:
        format = AudioDataFormat::PCM_S24;
        break;
    case MPG123_ENC_SIGNED_32:
        format = AudioDataFormat::PCM_S32;
        break;
    case MPG123_ENC_FLOAT_32:
        format = AudioDataFormat::Float32;
        break;
    case MPG123_ENC_FLOAT_64:
        format = AudioDataFormat::Float64;
        break;
    default:
        break;
    }

    return format;
}

long Mpg123EncodingFromAudioFormat(AudioDataFormat format)
{
    long encoding = MPG123_ENC_ANY;
    switch (format)
    {
    case AudioDataFormat::PCM_S8:
        encoding = MPG123_ENC_SIGNED_8;
        break;
    case AudioDataFormat::PCM_U8:
        encoding = MPG123_ENC_UNSIGNED_8;
        break;
    case AudioDataFormat::PCM_S16:
        encoding = MPG123_ENC_SIGNED_16;
        break;
    case AudioDataFormat::PCM_S24:
        encoding = MPG123_ENC_SIGNED_24;
        break;
    case AudioDataFormat::PCM_S32:
        encoding = MPG123_ENC_SIGNED_32;
        break;
    case AudioDataFormat::Float32:
        encoding = MPG123_ENC_FLOAT_32;
        break;
    case AudioDataFormat::Float64:
        encoding = MPG123_ENC_FLOAT_64;
        break;
    default:
        break;
    }

    return encoding;
}

bool CMp3Handle::OpenStream(CDataStream* pStream)
{
    if (m_hMpg12 == nullptr)
        return false;


    int valid = mpg123_reader64(m_hMpg12, StreamRead, StreamSeek, StreamCleanup);
    if(valid != MPG123_OK)
        return false;

    valid = mpg123_open_handle(m_hMpg12, pStream);
    if(valid != MPG123_OK)
        return false;

    long rate;
    int channels, encoding;
    valid = mpg123_getformat(m_hMpg12, &rate, &channels, &encoding);
    if(valid != MPG123_OK)
        return false;
   
    mpg123_scan(m_hMpg12);

    m_format = AudioFormatFromMpg123Encoding(encoding);
    m_channels = channels;
    m_sampleRate = rate;

    return true;
}

int32_t CMp3Handle::Read(void* pBuf, int32_t bufSize)
{
    if (m_hMpg12 == nullptr)
        return 0;

    std::size_t readed = 0;
    int valid = mpg123_read(m_hMpg12, pBuf, bufSize, &readed);
    if (valid == MPG123_OK)
        return static_cast<int32_t>(readed);

    m_decodeDone = (valid == MPG123_DONE);
    return static_cast<int32_t>(readed);
}

std::size_t CMp3Handle::GetTotalFrames() const 
{
    if (m_hMpg12 == nullptr)
        return 0;

    //return mpg123_framelength64(m_hMpg12);
    return mpg123_length64(m_hMpg12); //samples is PCM frames
}

std::size_t CMp3Handle::GetCurrentFrame() const
{
    if (m_hMpg12 == nullptr)
        return 0;

    // return mpg123_tellframe64(m_hMpg12);
    return mpg123_tell64(m_hMpg12); //samples is PCM frames
}

std::size_t CMp3Handle::SeekSamples(long long off, int where)
{
    if (m_hMpg12 == nullptr)
        return 0;

    m_decodeDone = false;
    //int64_t frames_off = mpg123_seek_frame64(m_hMpg12, off, where);
    int64_t sample_off = mpg123_seek64(m_hMpg12, off, where); //samples is PCM frames
    return sample_off;
}

Mp3Version Mp3VersionTranslate(mpg123_version ver)
{
    if (ver == MPG123_1_0)
        return Mp3Ver_V1;
    else if(ver == MPG123_2_0)
        return Mp3Ver_V2;
    else
        return Mp3Ver_V2_5;
}

Mp3Flags Mp3FlagsTranslate(mpg123_flags flags)
{
    if (flags == MPG123_CRC)
        return Mp3Flags_Crc;
    else if(flags == MPG123_COPYRIGHT)
        return Mp3Flags_Copyright;
    else if(flags == MPG123_PRIVATE)
        return Mp3Flags_Private;
    else
        return Mp3Flags_Original;
}

Mp3Mode Mp3ModeTranslate(mpg123_mode mode)
{
    if (mode == MPG123_M_STEREO)
        return Mp3Mode_Stereo;
    else if(mode == MPG123_M_JOINT)
        return Mp3Mode_JointStero;
    else if(mode == MPG123_M_DUAL)
        return Mp3Mode_DualChannel;
    else
        return Mp3Mode_Mono;
}

Mp3VbrMode Mp3VbrModeTranslate(mpg123_vbr vbr)
{
    if (vbr == MPG123_CBR)
        return Mp3VbrMode_CBR;
    else if (vbr == MPG123_VBR)
        return Mp3VbrMode_VBR;
    else
        return Mp3VbrMode_ABR;
}

using namespace std::literals;

std::string to_std_string(const mpg123_string& s)
{
    std::string result;

    if (s.p && s.fill > 0) 
    {
        result = std::string(s.p, s.fill);
        TrimRight(result, " \t\n\r\f\v\0"sv);
    }
    return result;
}

std::string to_std_string_v1(const char* s, int bufsize)
{
    const char* l = s + bufsize - 1;
    while ((l >= s) && ((*l == ' ') || (*l == '\0')))
        l--;
    if(l >= s)
        return std::string(s, l - s + 1);
    else
        return std::string();
}

bool IsValidIdV2(mpg123_id3v2* id2)
{
    if (id2 == nullptr)
        return false;

    if (id2->version == 0) 
        return false;

    return true;
}

bool IsValidIdV1(mpg123_id3v1* v1)
{
    if (v1 == nullptr)
        return false;

    if ((v1->tag[0] != 'T') || (v1->tag[1] != 'A') || (v1->tag[2] != 'G')) //"TAG"
        return false;

    return true;
}


bool CMp3Handle::GetExtraInfo(Mp3ExtraInfo& extraInfo)
{
    if (m_hMpg12 == nullptr)
        return false;

    mpg123_frameinfo2 mi;
    int valid = mpg123_info2(m_hMpg12, &mi);
    if (valid == MPG123_OK)
    {
        extraInfo.version = Mp3VersionTranslate((mpg123_version)mi.version);
        extraInfo.layer = mi.layer;
        extraInfo.bitrate = mi.bitrate;
        extraInfo.abr_rate = mi.abr_rate;
        extraInfo.flags = Mp3FlagsTranslate((mpg123_flags)mi.flags);
        extraInfo.framesize = mi.framesize;
        extraInfo.mode = Mp3ModeTranslate((mpg123_mode)mi.mode);
        extraInfo.rate = mi.rate;
        extraInfo.vbr = Mp3VbrModeTranslate((mpg123_vbr)mi.vbr);
    }

    // 获取 ID3 标签信息
    mpg123_id3v1* v1 = nullptr;
    mpg123_id3v2* v2 = nullptr;
    valid = mpg123_id3(m_hMpg12, &v1, &v2);
    if (valid == MPG123_OK)
    {
        if (IsValidIdV2(v2))
        {
            //version 2.3 使用 UTF16(LE/BE)，2.8 使用 UTF8
            extraInfo.title = DecodeUnknownString(v2->title ? to_std_string (*v2->title) : "");
            extraInfo.album = DecodeUnknownString(v2->album ? to_std_string(*v2->album) : "");
            extraInfo.artist = DecodeUnknownString(v2->artist ? to_std_string(*v2->artist) : "");
            extraInfo.year = DecodeUnknownString(v2->year ? to_std_string(*v2->year) : "");
            extraInfo.genre = DecodeUnknownString(v2->genre ? to_std_string(*v2->genre) : "");
            extraInfo.comment = DecodeUnknownString(v2->comment ? to_std_string(*v2->comment) : "");
        }

        if (IsValidIdV1(v1)) //如果有的话，作为补充
        {
            if(extraInfo.title.empty())
                extraInfo.title = DecodeUnknownString(to_std_string_v1(v1->title, 30));
            if(extraInfo.album.empty())
                extraInfo.album = DecodeUnknownString(to_std_string_v1(v1->album, 30));
            if(extraInfo.artist.empty())
                extraInfo.artist = DecodeUnknownString(to_std_string_v1(v1->artist, 30));
            if(extraInfo.year.empty())
                extraInfo.year = DecodeUnknownString(to_std_string_v1(v1->year, 4));
            if(extraInfo.comment.empty())
                extraInfo.comment = DecodeUnknownString(to_std_string_v1(v1->comment, 30));
            if (extraInfo.genre.empty())
            {
                if(v1->genre >= 0 && v1->genre <= 147)
                    extraInfo.genre = id3v1_genres[v1->genre];
            }
        }
    }

    mpg123_meta_free(m_hMpg12);

    return false;
}

bool CMp3Handle::Close()
{
    if (m_hMpg12 != nullptr)
    {
        mpg123_close(m_hMpg12);
    }

    return true;
}

bool CMp3Handle::SetAudioFormat(AudioDataFormat format)
{
    if (m_hMpg12 == nullptr)
        return false;

    long encoding = Mpg123EncodingFromAudioFormat(format);
    if (encoding != MPG123_ENC_ANY)
    {
        mpg123_format_none(m_hMpg12);
        // 设置格式为 32-bit float PCM，允许任意采样率和声道数
        //mpg123_format(mh, 0, MPG123_STEREO | MPG123_MONO, MPG123_ENC_FLOAT_32);
        int valid = mpg123_format2(m_hMpg12, 0, MPG123_STEREO | MPG123_MONO, encoding);
        if (valid == MPG123_OK)
        {
            long rate = 0;
            int channels = 0;
            int encoding2 = 0;
            mpg123_getformat(m_hMpg12, &rate, &channels, &encoding2);
            m_format = format;
            return true;
        }
    }

    return false;
}

