#ifndef _MP3_HANDLE_H_
#define _MP3_HANDLE_H_

#include <string>
#include "DataStream.h"
#include "AudioInfo.h"
#include "mpg123.h"

class CMp3Handle final
{
public:
    CMp3Handle();
    ~CMp3Handle();
    bool OpenFile(const std::string& filename);
    bool OpenStream(CDataStream *pStream);
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


