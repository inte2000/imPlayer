/*
20260526 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_78.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <FLAC/stream_decoder.h>

#include "DataStream.h"
#include "AudioInfo.h"
#include "MediaTag.h"

class FlacPlayCtrl
{
public:
    FlacPlayCtrl();
    ~FlacPlayCtrl();

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
    bool EnsureCacheFrames(uint32_t frames);
    uint32_t GetAvailableCacheFrames() const;
    void ConsumeCacheFrames(uint32_t frames);
    void CompactCache();
    void ParseVorbisCommentEntry(const FLAC__StreamMetadata_VorbisComment_Entry& entry);

    static FLAC__StreamDecoderReadStatus ReadCallback(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data);
    static FLAC__StreamDecoderSeekStatus SeekCallback(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data);
    static FLAC__StreamDecoderTellStatus TellCallback(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data);
    static FLAC__StreamDecoderLengthStatus LengthCallback(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data);
    static FLAC__bool EofCallback(const FLAC__StreamDecoder* decoder, void* client_data);
    static FLAC__StreamDecoderWriteStatus WriteCallback(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data);
    static void MetadataCallback(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data);
    static void ErrorCallback(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data);

private:
    CDataStream* m_stream;
    FLAC__StreamDecoder* m_decoder;
    AudioFormat m_srcAudioFmt;
    std::size_t m_curFrames;
    std::size_t m_totalFrames;
    uint32_t m_streamFmt;
    uint32_t m_activeStreamIdx;
    bool m_opened;
    bool m_endOfStream;
    bool m_hasError;

    uint32_t m_decodeChannels;
    uint32_t m_decodeSampleRate;
    uint32_t m_decodeBitsPerSample;
    std::vector<int32_t> m_pcmCache;
    std::size_t m_cacheOffsetFrames;

    std::string m_tagTitle;
    std::string m_tagArtist;
    std::string m_tagAlbum;
    std::string m_tagGenre;
    std::string m_tagComment;
    std::string m_tagDate;
};
