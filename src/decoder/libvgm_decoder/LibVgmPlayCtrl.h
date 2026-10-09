/*
20260508 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_35.txt
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include "DataStream.h"
#include "AudioInfo.h"
#include "MediaTag.h"
#include "PluginConfig.h"
#include "emu/Resampler.h"

struct _data_loader;
class PlayerA;
class PlayerBase;

class LibVgmPlayCtrl
{
public:
    LibVgmPlayCtrl();
    ~LibVgmPlayCtrl();

    bool Init(CDataStream* stream, uint32_t streamFmt, const PluginConfig& config);
    void Release();

    bool OpenStream(uint32_t streamIndex);
    void StopStream();

    bool IsSupportOutput(const AudioFormat* audioFmt) const;
    bool IsCanSeeking() const;

    uint32_t DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt);
    void Seek(std::size_t frames);

    std::size_t GetCurrentFrame() const;
    std::size_t GetTotalFrame() const { return m_totalFrames; }
    float GetDurationSeconds() const;
    uint32_t GetStreamCount() const { return 1; }
    uint32_t GetActiveStreamIndex() const { return m_activeStreamIdx; }
    const AudioFormat& GetSourceAudioFormat() const { return m_srcAudioFmt; }
    void FillMetaTags(CMediaTag& tags) const;

private:
    bool InitPlayer();
    PlayerBase* GetPlayerEngine();
    const PlayerBase* GetPlayerEngine() const;
    bool ReadAllStreamData();
    void RefreshDurationInfo();
    void InitSourceAudioFormatByConfig();

    static int32_t Clamp32(int32_t sample);
    static int16_t ToPcmS16(int32_t sample);
    static int32_t ToPcmS32(int32_t sample);
    static float ToFloat32(int32_t sample);

private:
    CDataStream* m_stream;
    _data_loader* m_dataLoader;
    std::unique_ptr<PlayerA> m_player;
    std::vector<uint8_t> m_fileData;
    std::vector<WAVE_32BS> m_waveBuf;
    PluginConfig m_pluginConfig;
    AudioFormat m_srcAudioFmt;
    std::size_t m_totalFrames;
    uint32_t m_streamFmt;
    uint32_t m_activeStreamIdx;
    bool m_opened;
};
