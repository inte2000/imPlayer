/*
20260508 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_35.txt

相关修改：
todo_task_35.txt
todo_task_37.txt
todo_task_39.txt
todo_task_40.txt
*/
#include <algorithm>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include "MediaTagNames.h"
#include "player/playera.hpp"
#include "player/playerbase.hpp"
#include "player/vgmplayer.hpp"
#include "player/s98player.hpp"
#include "player/droplayer.hpp"
#include "player/gymplayer.hpp"
#include "utils/MemoryLoader.h"
#include "utils/DataLoader.h"
#include "LibvgmFunc.h"
#include "LibVgmPlayCtrl.h"

#define BUFFER_LEN 4096

LibVgmPlayCtrl::LibVgmPlayCtrl()
    : m_stream(nullptr)
    , m_dataLoader(nullptr)
    , m_player(nullptr)
    , m_fileData()
    , m_waveBuf()
    , m_pluginConfig({})
    , m_srcAudioFmt({})
    , m_totalFrames(0)
    , m_streamFmt(StreamFormatUnknown)
    , m_activeStreamIdx(static_cast<uint32_t>(-1))
    , m_opened(false)
{
    InitSourceAudioFormatByConfig();
}

LibVgmPlayCtrl::~LibVgmPlayCtrl()
{
    Release();
}

bool LibVgmPlayCtrl::Init(CDataStream* stream, uint32_t streamFmt, const PluginConfig& config)
{
    if (stream == nullptr) {
        return false;
    }

    m_stream = stream;
    m_streamFmt = streamFmt;
    m_pluginConfig = config;
    InitSourceAudioFormatByConfig();

    if (!ReadAllStreamData()) {
        Release();
        return false;
    }

    m_dataLoader = MemoryLoader_Init(m_fileData.data(), static_cast<UINT32>(m_fileData.size()));
    if (m_dataLoader == nullptr) {
        Release();
        return false;
    }

    DataLoader_SetPreloadBytes(m_dataLoader, 0x100);
    if (DataLoader_Load(m_dataLoader) != 0x00) {
        Release();
        return false;
    }
    //DataLoader_ReadAll(m_dataLoader);

    if (!InitPlayer()) {
        Release();
        return false;
    }

    RefreshDurationInfo();
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
    return true;
}

void LibVgmPlayCtrl::Release()
{
    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);

    if (m_player) {
        m_player->Stop();
        m_player->UnloadFile();
        m_player->UnregisterAllPlayers(); //Manual bug fixing
        m_player.reset();
    }

    if (m_dataLoader != nullptr) {
        DataLoader_Deinit(m_dataLoader);
        m_dataLoader = nullptr;
    }

    m_stream = nullptr;
    m_fileData.clear();
    m_waveBuf.clear();
    InitSourceAudioFormatByConfig();
    m_totalFrames = 0;
    m_streamFmt = StreamFormatUnknown;
}

bool LibVgmPlayCtrl::OpenStream(uint32_t streamIndex)
{
    PlayerBase* player = GetPlayerEngine();
    if (player == nullptr) {
        return false;
    }
    if ((streamIndex != 0) && (streamIndex != static_cast<uint32_t>(-1))) {
        return false;
    }

    if (player->Start() != 0x00) {
        return false;
    }

    m_opened = true;
    m_activeStreamIdx = 0;
    return true;
}

void LibVgmPlayCtrl::StopStream()
{
    PlayerBase* player = GetPlayerEngine();
    if (player != nullptr) {
        player->Stop();
    }

    m_opened = false;
    m_activeStreamIdx = static_cast<uint32_t>(-1);
}

bool LibVgmPlayCtrl::IsSupportOutput(const AudioFormat* audioFmt) const
{
    if (m_srcAudioFmt.sampleRate != audioFmt->sampleRate)
        return false;
    if (m_srcAudioFmt.numChannels != audioFmt->numChannels)
        return false;    
    if (audioFmt->numChannels != 2) {
        return false;
    }

    switch (audioFmt->format)
    {
    case AudioDataFormat::PCM_S16:
    case AudioDataFormat::PCM_S32:
    case AudioDataFormat::Float32:
        return true;
    default:
        return false;
    }
}

bool LibVgmPlayCtrl::IsCanSeeking() const
{
    return (GetPlayerEngine() != nullptr);
}

uint32_t LibVgmPlayCtrl::DecodeFrames(void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if (audioFmt != nullptr)
    {
        if ((audioFmt->format != AudioDataFormat::Float32) &&
            (audioFmt->format != AudioDataFormat::PCM_S32) && 
            (audioFmt->format != AudioDataFormat::PCM_S16))
            return 0;
    }

    uint32_t frameSize = (m_srcAudioFmt.bitsPerSample / 8) * m_srcAudioFmt.numChannels;
    UINT32 curFrames = m_player->GetCurPos(PLAYPOS_SAMPLE);
    //UINT32 curFrame2 = pCtx->plrEngine->GetCurPos(PLAYPOS_SAMPLE);
    UINT32 remainFrames = std::min((UINT32)m_totalFrames - curFrames, frames);
    UINT32 readFrames = 0;

    std::size_t bufPos = 0;
    UINT8* pByteBuf = reinterpret_cast<UINT8*>(pBuf);
    while (remainFrames > 0)
    {
        UINT32 curFrames = (BUFFER_LEN > remainFrames ? remainFrames : BUFFER_LEN);
        m_player->Render(curFrames * frameSize, m_waveBuf.data());
     
        if (audioFmt->format == AudioDataFormat::PCM_S16) {
            int16_t* out = reinterpret_cast<int16_t*>(pByteBuf);
            for (UINT32 i = 0; i < curFrames; ++i)
            {
                out[i * 2 + 0] = ToPcmS16(m_waveBuf[i].L);
                out[i * 2 + 1] = ToPcmS16(m_waveBuf[i].R);
            }
        }
        else if (audioFmt->format == AudioDataFormat::PCM_S32) {
            int32_t* out = reinterpret_cast<int32_t*>(pByteBuf);
            for (UINT32 i = 0; i < curFrames; ++i)
            {
                out[i * 2 + 0] = ToPcmS32(m_waveBuf[i].L);
                out[i * 2 + 1] = ToPcmS32(m_waveBuf[i].R);
            }
        }
        else {
            float* out = reinterpret_cast<float*>(pByteBuf);
            for (UINT32 i = 0; i < curFrames; ++i)
            {
                out[i * 2 + 0] = ToFloat32(m_waveBuf[i].L);
                out[i * 2 + 1] = ToFloat32(m_waveBuf[i].R);
            }
        }    

        bufPos = (std::size_t)frameSize * curFrames;
        pByteBuf += bufPos;
        remainFrames -= curFrames;
        readFrames += curFrames;
    }

    return readFrames;
}

void LibVgmPlayCtrl::Seek(std::size_t frames)
{
    UINT32 FramePos = static_cast<UINT32>(frames);
    m_player->Seek(PLAYPOS_SAMPLE, FramePos);
}

std::size_t LibVgmPlayCtrl::GetCurrentFrame() const
{
    if (m_player == nullptr) {
        return 0;
    }

    return m_player->GetCurPos(PLAYPOS_SAMPLE);
}

float LibVgmPlayCtrl::GetDurationSeconds() const
{
    if ((m_srcAudioFmt.sampleRate == 0) || (m_totalFrames == 0)) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(m_totalFrames) / static_cast<double>(m_srcAudioFmt.sampleRate));
}

void LibVgmPlayCtrl::FillMetaTags(CMediaTag& tags) const
{
    tags.Clear();

    const std::string type = LibvgmFormatName(m_streamFmt);
    const std::string brief = std::format("{}: {}", type, AudioFormatBrifStr(&m_srcAudioFmt));

    tags.AddTagString(MediaTag_Type, type);
    tags.AddTagString(MediaTag_Brief, brief);
    tags.AddTagInteger(MediaTag_Streams, 1);
    tags.AddTagDecimal(MediaTag_Duration, GetDurationSeconds());

    tags.AddTagInteger(MediaTag_SamplesRate, static_cast<int32_t>(m_srcAudioFmt.sampleRate));
    tags.AddTagInteger(MediaTag_Channels, static_cast<int32_t>(m_srcAudioFmt.numChannels));
    tags.AddTagInteger(MediaTag_ChannelsLayout, static_cast<int32_t>(m_srcAudioFmt.chLayout));
    tags.AddTagInteger(MediaTag_BitsPerSample, static_cast<int32_t>(m_srcAudioFmt.bitsPerSample));
    tags.AddTagString(MediaTag_PcmFormat, StringFromAudioFormat(m_srcAudioFmt.format));
    const uint32_t bitrates = m_srcAudioFmt.bitsPerSample * m_srcAudioFmt.sampleRate * m_srcAudioFmt.numChannels;
    tags.AddTagInteger(MediaTag_BitsRate, static_cast<int32_t>(bitrates));

    PlayerBase* player = const_cast<LibVgmPlayCtrl*>(this)->GetPlayerEngine();
    if (player != nullptr) {
        const char* const* tagList = player->GetTags();
        for (const char* const* t = tagList; (t != nullptr) && (t[0] != nullptr) && (t[1] != nullptr); t += 2)
        {
            if (strcmp(t[0], "TITLE") == 0) {
                tags.AddTagString(MediaTag_Title, t[1]);
            }
            else if (strcmp(t[0], "ARTIST") == 0) {
                tags.AddTagString(MediaTag_Artists, t[1]);
            }
            else if (strcmp(t[0], "GAME") == 0) {
                tags.AddTagString(MediaTag_Album, t[1]);
            }
            else if (strcmp(t[0], "DATE") == 0) {
                tags.AddTagString(MediaTag_Date, t[1]);
            }
            else if (strcmp(t[0], "COMMENT") == 0) {
                tags.AddTagString(MediaTag_Comment, t[1]);
            }
        }
    }
}

bool LibVgmPlayCtrl::InitPlayer()
{
    if (m_dataLoader == nullptr) {
        return false;
    }

    m_player = std::make_unique<PlayerA>();
    m_player->RegisterPlayerEngine(new VGMPlayer());
    m_player->RegisterPlayerEngine(new S98Player());
    m_player->RegisterPlayerEngine(new DROPlayer());
    m_player->RegisterPlayerEngine(new GYMPlayer());
    if (m_player->SetOutputSettings(m_srcAudioFmt.sampleRate, m_srcAudioFmt.numChannels, m_srcAudioFmt.bitsPerSample, BUFFER_LEN)) {
        m_player.reset();
        return false;
    }

    //m_player->SetSampleRate(m_srcAudioFmt.sampleRate);
    //m_player->SetLoopCount(m_pluginConfig.Loops);
    //m_player->SetFadeSamples(static_cast<UINT32>(m_srcAudioFmt.sampleRate * m_pluginConfig.FadeLen));

    if (m_player->LoadFile(m_dataLoader) != 0x00) {
        m_player.reset();
        return false;
    }
    PlayerBase* plrEngine = m_player->GetPlayer();
    if (plrEngine->GetPlayerType() == FCC_VGM)
    {
        VGMPlayer* vgmplay = dynamic_cast<VGMPlayer*>(plrEngine);
        m_player->SetLoopCount(vgmplay->GetModifiedLoopCount(m_pluginConfig.Loops));
    }

    m_waveBuf.resize(BUFFER_LEN);
    /* need to call Start before calls like Tick2Sample or
     * checking any kind of timing info, because
     * Start updates the sample rate multiplier/divisors */
    m_player->Start();

    return true;
}

bool LibVgmPlayCtrl::ReadAllStreamData()
{
    if (m_stream == nullptr) {
        return false;
    }

    const std::size_t totalSize = m_stream->GetLength();
    if (totalSize == 0) {
        return false;
    }

    m_fileData.resize(totalSize);
    m_stream->Seek(SeekBase::Begin, 0);

    std::size_t readed = 0;
    while (readed < totalSize)
    {
        uint32_t once = m_stream->Read(m_fileData.data() + readed, static_cast<uint32_t>(totalSize - readed));
        if (once == 0) {
            break;
        }
        readed += once;
    }

    if (readed != totalSize) {
        m_fileData.resize(readed);
    }
    return !m_fileData.empty();
}

void LibVgmPlayCtrl::RefreshDurationInfo()
{
    const PlayerBase* plrEngine = GetPlayerEngine();
    if (plrEngine == nullptr) {
        m_totalFrames = 0;
        return;
    }

    m_totalFrames = plrEngine->Tick2Sample(plrEngine->GetTotalPlayTicks(m_pluginConfig.Loops));
    /* we only want to fade if there's a looping section. Assumption is
     * if the VGM doesn't specify a loop, it's a song with an actual ending */
    if (plrEngine->GetLoopTicks() > 0) {
        m_totalFrames += m_player->GetFadeSamples();
    }

    //Strange code of AI
    m_srcAudioFmt.blockAlign = (m_srcAudioFmt.bitsPerSample / 8) * m_srcAudioFmt.numChannels;
}

PlayerBase* LibVgmPlayCtrl::GetPlayerEngine()
{
    return (m_player == nullptr) ? nullptr : m_player->GetPlayer();
}

const PlayerBase* LibVgmPlayCtrl::GetPlayerEngine() const
{
    return (m_player == nullptr) ? nullptr : m_player->GetPlayer();
}

void LibVgmPlayCtrl::InitSourceAudioFormatByConfig()
{
    const uint32_t sampleRate = (m_pluginConfig.SampleRate == 0) ? 44100 : m_pluginConfig.SampleRate;
    uint32_t bitsPerSample = m_pluginConfig.BitsPerSample;
    if ((bitsPerSample != 16) && (bitsPerSample != 24) && (bitsPerSample != 32)) {
        bitsPerSample = 24;
    }

    InitAudioFormat(&m_srcAudioFmt, AudioFormatByBitsPerSample(bitsPerSample), 2, sampleRate, bitsPerSample);
}

int32_t LibVgmPlayCtrl::Clamp32(int32_t sample)
{
    return std::clamp(sample, (int32_t)-0x80000000, (int32_t)+0x7FFFFFFF);
}

int16_t LibVgmPlayCtrl::ToPcmS16(int32_t sample)
{
    int32_t value = Clamp32(sample) >> 16;
    value = std::clamp(value, -0x8000, +0x7FFF);
    return static_cast<int16_t>(value);
}

int32_t LibVgmPlayCtrl::ToPcmS32(int32_t sample)
{
    return Clamp32(sample);
}

float LibVgmPlayCtrl::ToFloat32(int32_t sample)
{
    return static_cast<float>(sample) / static_cast<float>(0x80000000);
}
