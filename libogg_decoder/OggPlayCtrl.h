/*
20260526 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_81.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <vorbis/vorbisfile.h>

#include "DataStream.h"
#include "AudioInfo.h"
#include "MediaTag.h"

class OggPlayCtrl
{
public:
    OggPlayCtrl();
    ~OggPlayCtrl();

    bool Init(CDataStream* stream, uint32_t streamFmt);
    void Release();

    bool OpenStream(uint32_t streamIndex);
    void StopStream();

    bool IsSupportOutput(const AudioFormat* audioFmt) const;
    bool IsCanSeeking() const;

    uint32_t DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt);
    void Seek(std::size_t frames);

    std::size_t GetCurrentFrame() const { return m_curFrames; }
    std::size_t GetTotalFrame() const { return m_totalFrames; }
    float GetDurationSeconds() const;
    uint32_t GetStreamCount() const { return 1; }
    uint32_t GetActiveStreamIndex() const { return m_activeStreamIdx; }
    const AudioFormat& GetSourceAudioFormat() const { return m_srcAudioFmt; }
    void FillMetaTags(CMediaTag& tags) const;

private:
    static size_t ReadFunc(void* ptr, size_t size, size_t nmemb, void* datasource);
    static int SeekFunc(void* datasource, ogg_int64_t offset, int whence);
    static int CloseFunc(void* datasource);
    static long TellFunc(void* datasource);

    void ParseComments();

private:
    CDataStream* m_stream;
    OggVorbis_File m_vf;
    bool m_inited;
    AudioFormat m_srcAudioFmt;
    std::size_t m_curFrames;
    std::size_t m_totalFrames;
    uint32_t m_streamFmt;
    uint32_t m_activeStreamIdx;
    bool m_opened;
    std::vector<int16_t> m_tempS16;

    std::string m_tagTitle;
    std::string m_tagArtist;
    std::string m_tagAlbum;
    std::string m_tagGenre;
    std::string m_tagComment;
    std::string m_tagDate;
};
