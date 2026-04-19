#ifndef TUI_PLAYER_UI_H
#define TUI_PLAYER_UI_H

#include <memory>
#include <atomic>
#include <functional>
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "Playback.h"

class TUIPlayerUI : public PlaybackCallback
{
public:
    explicit TUIPlayerUI();
    ~TUIPlayerUI();

    bool Init(std::unique_ptr<CAudioDevice> audioDevice, const std::string& deviceId, const std::string& filename, bool bPlaylist, const std::string& speakerLayout);

    // Start the TUI main loop
    void Run();

    // Stop the TUI
    void Stop();

    // PlaybackCallback interface implementation
    void OnAudioBegin(const AudioFormat& audioFmt, const std::string& extraInfo, const std::wstring& name, float totalSeconds) override;
    void OnAudioUpdate(float curSeconds, float* powerBands, int bands) override;
    bool OnAudioEnd() override;
    void OnControlEvent(PlayControl ctrl) override;
    void OnVolumeChanged(BOOL bMute, int vol) override;

    // Thread-safe getter methods for UI to read playback state
    float GetCurrentPosition();
    float GetTotalSeconds();
    std::wstring GetSongName();
    AudioFormat GetAudioFormat();
    std::string GetExtraInfo();
    PlaybackStatus GetStatus();
    std::string GetTitle();
    std::string GetArtist();
    std::string GetAlbum();

private:
    // UI component creation methods
    ftxui::Component CreateSongInfoSection();
    ftxui::Component CreateProgressBar();
    ftxui::Component CreatePlaybackControls();
    ftxui::Component CreateVolumeSlider();

    // UI update methods
    std::string FormatTime(float seconds);
    ftxui::Element BuildSongInfoDisplay();

    // Playback control handlers
    void OnPlayPause();
    void OnStop();
    void OnSeekForward();
    void OnSeekBackward();

    // Member variables
    std::shared_ptr<CPlayback> m_playback;

    // UI state
    std::atomic<bool> m_running;
    int m_volume;
    float m_seekPosition;
    std::atomic<bool> m_showVolume;

    // Thread safety - protects all member variables below
    mutable std::mutex m_mutex;

    // Playback state
    float m_currentSeconds;
    float m_totalSeconds;
    std::wstring m_songName;
    AudioFormat m_audioFormat;
    std::string m_extraInfo;
    PlaybackStatus m_status;
    
    // Parsed metadata from extraInfo JSON
    std::string m_title;
    std::string m_artist;
    std::string m_album;    
};

#endif // TUI_PLAYER_UI_H
