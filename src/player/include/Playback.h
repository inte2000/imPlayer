#ifndef AUDIO_PLAY_BACK_H
#define AUDIO_PLAY_BACK_H

#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <semaphore>
#include <atomic>
#include <optional>
#include "AudioBuffer.h"
#include "AudioDevice.h"
#include "AudioSource.h"
#include "PlaybackCallback.h"
#include "SpeakerConfig.h"



enum class PlaybackStatus
{
    Playing = 1,
    PlayingEnd = 2,
    Paused = 3,
    Stoped = 4
};


class CPlayback : public std::enable_shared_from_this<CPlayback>
{
public:
    static std::shared_ptr<CPlayback> Create(PlaybackCallback *pCallback, std::unique_ptr<CAudioDevice> audioDevice);
    ~CPlayback();

    std::binary_semaphore m_pbSemaphore, m_quitSemaphore, m_audioEndSemaphore;
    std::atomic<bool> m_quitSign, m_pbThreadRunning;
    
    AudioBuffer& GetAudioBuffer() { return m_audioBuf; }
    void UpdataPlayback(void *audioBuf, uint32_t frames, std::size_t framesInBuffer);
    void MockAudioEndCallback();
    void NotifyAudioStreamBegin(const CAudioSource* pAudioSource, uint32_t streamIdx);
    bool SetAudioSource(std::unique_ptr<CAudioSource> ds, bool autostart = false);
    CAudioSource* GetAudioSource() { return m_dataSource.get(); }
    const CAudioSource* GetAudioSource() const { return m_dataSource.get(); }
    bool SwitchAudioDevice(std::unique_ptr<CAudioDevice> audioDevice);
    bool RestartAudioDevice();
    CAudioDevice* GetAudioDevice() { return m_audioDevice.get(); }
    const CAudioDevice* GetAudioDevice() const { return m_audioDevice.get(); }
    void SetSpeakerConfig(std::unique_ptr<CSpeakerConfig> speaker) { m_speaker = std::move(speaker); }
    CSpeakerConfig* GetSpeakerConfig() { return m_speaker.get(); }
    const CSpeakerConfig* GetSpeakerConfig() const { return m_speaker.get(); }
    void OnDefaultDeviceChanged(const std::wstring& deviceId);

    void Shutdown();
    void SetOutputDeviceId(const std::string& deviceId) { m_deviceId = deviceId; }
    void SetPreferBufferLength(uint32_t length) { m_PreferBufferLength = length; }

    std::optional<int> GetCurrentDeviceVolume();
    bool SetCurrentDeviceVolume(int vol);
    PlaybackStatus GetPlaybackStatus() const { return m_status; }
    bool HasAudioSource() const { return (m_dataSource != nullptr); }

    void Play();
    void Stop();
    void Pause();
    void SeekPosition(float seconds_pos);
    float GetCurrentPosition() const;
protected:
    void OnDeviceVolumeChanged(BOOL bMute, int vol);
    bool GetAudioDeviceNegoFormat(const std::string& deviceId, AudioFormat& negoFmt);

    bool InitPlayback(PlaybackCallback* pCallback, std::unique_ptr<CAudioDevice> audioDevice);
    void SetAudioDevice(std::unique_ptr<CAudioDevice> audioDevice);
    void ShutdownPlayback();
    void StartPlaybackThread();
    void StopPlaybackThread();
    void OpenAudioDevice(const AudioFormat& audioFmt);
    void CloseAudioDevice();
    void StartCurrentAudioSource();
    void StopCurrentAudioSource(bool bNotify = true);
    
private:
    CPlayback() : m_status(PlaybackStatus::Stoped), m_quitSign(false), m_pbThreadRunning(false), 
                  m_pbSemaphore(0), m_quitSemaphore(0), m_audioEndSemaphore(0), m_pCallback(nullptr), m_PreferBufferLength(8)
    {
    }

    std::thread m_pbThread;
    AudioBuffer m_audioBuf;
    std::unique_ptr<CAudioDevice> m_audioDevice;
    std::unique_ptr<CAudioSource> m_dataSource;
    PlaybackStatus m_status;
    std::string m_deviceId;
    std::unique_ptr<CSpeakerConfig> m_speaker;
    uint32_t m_PreferBufferLength;

    //Event callback hanler
    PlaybackCallback* m_pCallback;
    std::mutex m_defDeviceChange;
};


#endif //AUDIO_PLAY_BACK_H
