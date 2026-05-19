/*
20260310 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4/DeepSeek V2
*/
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
    void Exit();

    // PlaybackCallback interface implementation
    void OnAudioBegin(uint32_t streamIdx, const CMediaTag& metaInfo, const std::wstring& name, float totalSeconds) override;
    void OnAudioUpdate(float curSeconds, float* powerBands, int bands) override;
    bool OnAudioEnd(bool lastStream) override;
    void OnControlEvent(PlayControl ctrl) override;
    void OnVolumeChanged(BOOL bMute, int vol) override;

    // Thread-safe getter methods for UI to read playback state
    float GetCurrentPosition();
    float GetTotalSeconds();
    PlaybackStatus GetStatus();

private:
    void BuildUI();
    std::pair<float, std::string> GetProgressStatus();
    std::pair<std::string, std::string> GetMusicMetaInfo();
    std::string GetMusicFormatStatus();    

    // screen
    ftxui::ScreenInteractive m_screen;
    // root component
    ftxui::Component m_root;
    ftxui::Component m_main_container;

    // buttons
    ftxui::Component m_btn_close;
    ftxui::Component m_btn_prev;
    ftxui::Component m_btn_play;
    ftxui::Component m_btn_pause;
    ftxui::Component m_btn_stop;
    ftxui::Component m_btn_next;


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
    std::wstring m_infoStr;
    PlaybackStatus m_status;  
    std::string m_title;
    std::string m_album;
    std::string m_brief;    
};

#endif // TUI_PLAYER_UI_H
