/*
20260522 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_66.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gme/gme.h>

#include "DataStream.h"
#include "AudioInfo.h"
#include "MediaTag.h"
#include "PluginConfig.h"

class GmePlayCtrl
{
public:
    GmePlayCtrl();
    ~GmePlayCtrl();

    bool Init(CDataStream* stream, uint32_t streamFmt, const PluginConfig& config);
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
    uint32_t GetStreamCount() const { return m_streamCount; }
    uint32_t GetActiveStreamIndex() const { return m_activeStreamIdx; }
    const AudioFormat& GetSourceAudioFormat() const { return m_srcAudioFmt; }
    void FillMetaTags(CMediaTag& tags, uint32_t streamIndex) const;

private:
    bool ReadAllStreamData();
    void InitSourceAudioFormatByConfig();
    bool RefreshDurationInfo(uint32_t streamIndex);
    bool QueryTrackInfo(uint32_t streamIndex, gme_info_t** outInfo) const;

    static int32_t ToPcmS32(int16_t sample);
    static float ToFloat32(int16_t sample);

private:
    CDataStream* m_stream;
    Music_Emu* m_emu;
    std::vector<uint8_t> m_fileData;
    std::vector<int16_t> m_decodeBuf;
    PluginConfig m_pluginConfig;
    AudioFormat m_srcAudioFmt;
    std::size_t m_totalFrames;
    std::size_t m_curFrames;
    uint32_t m_streamFmt;
    uint32_t m_streamCount;
    uint32_t m_activeStreamIdx;
    bool m_opened;
};
