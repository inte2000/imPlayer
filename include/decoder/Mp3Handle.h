#ifndef _MP3_HANDLE_H_
#define _MP3_HANDLE_H_

#include <string>
#include "DataStream.h"
#include "AudioInfo.h"
#include "mpg123.h"


enum Mp3Mode
{
	Mp3Mode_Stereo = 0,
	Mp3Mode_JointStero,
	Mp3Mode_DualChannel,
	Mp3Mode_Mono
};

enum Mp3Flags 
{
	Mp3Flags_Crc = 0x01,
	Mp3Flags_Copyright = 0x02,
	Mp3Flags_Private = 0x04,
	Mp3Flags_Original = 0x08
};

enum Mp3Version
{
	Mp3Ver_V1 = 0,
	Mp3Ver_V2,
	Mp3Ver_V2_5
};

enum Mp3VbrMode
{
	Mp3VbrMode_CBR = 0,
	Mp3VbrMode_VBR,
	Mp3VbrMode_ABR
};

struct Mp3ExtraInfo
{
	Mp3Version version;
	int layer;
	long rate;
	Mp3Mode mode; 
	int framesize;
	Mp3Flags flags;
	int bitrate; // kbps
	int abr_rate;
	Mp3VbrMode vbr;
	std::string title;
	std::string album;
	std::string artist;
	std::string year;
	std::string genre;
	std::string comment;
};

class CMp3Handle final
{
public:
    CMp3Handle();
    ~CMp3Handle();
    bool OpenFile(const std::string& filename);
    bool OpenStream(CDataStream *pStream);
    bool IsValid() const { return (m_hMpg12 != nullptr); }
    operator bool() const { return (m_hMpg12 != nullptr); }
    int32_t Read(void *pBuf, int32_t bufSize);
    std::size_t GetTotalFrames() const;
    std::size_t GetCurrentFrame() const;
    std::size_t SeekSamples(long long off, int where);
    bool GetExtraInfo(Mp3ExtraInfo& extraInfo);
    bool Close();

    bool IsDecodeDone() const { return m_decodeDone; }
    AudioDataFormat GetAudioFormat() const { return m_format;  }
    bool SetAudioFormat(AudioDataFormat format);
    uint32_t GetChannels() const { return m_channels;  }
    uint32_t GetSampleRate() const { return m_sampleRate; }
private:
    bool m_decodeDone;
    mpg123_handle *m_hMpg12;
    AudioDataFormat m_format;
    uint32_t m_channels;
    uint32_t m_sampleRate;
};




#endif //_MP3_HANDLE_H_


