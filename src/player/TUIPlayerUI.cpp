#include "framework.h"
#include "UnicodeConvert.h"
#include "TUIPlayerUI.h"
#include "StringEx.h"
#include <format>
#include <cmath>

using namespace ftxui;

TUIPlayerUI::TUIPlayerUI()
    : m_screen(ScreenInteractive::Fullscreen())
    , m_running(false)
    , m_volume(50)
    , m_seekPosition(0.0f)
    , m_showVolume(false)
    , m_currentSeconds(0.0f)
    , m_totalSeconds(0.0f)
    , m_status(PlaybackStatus::Stoped){
}

TUIPlayerUI::~TUIPlayerUI()
{
    Exit();
}

bool TUIPlayerUI::Init(std::unique_ptr<CAudioDevice> audioDevice, const std::string& deviceId, const std::string& filename, bool bPlaylist, const std::string& speakerLayout)
{
    m_playback = CPlayback::Create(this, std::move(audioDevice));
    m_playback->SetOutputDeviceId(deviceId);
   
    std::unique_ptr<CSpeakerConfig> speakCfg = LoadSpeakerConfig(speakerLayout);
    m_playback->SetSpeakerConfig(std::move(speakCfg));
    
    std::wstring wfilename = LocalMBCSToUtf16Le(filename);
    std::unique_ptr<CAudioSource> audioSource = MakeFileAudioSource(wfilename);    
    m_playback->SetAudioSource(std::move(audioSource), true);

    return true;
}

std::wstring MakeNameInfoText(const std::wstring& name, float totalSeconds)
{    
    std::chrono::duration<double> totalDuration(totalSeconds);
    std::wstring infoStr;
    
    std::wstring stemName = GetFileNamePart(name);
    if(totalSeconds >= 3600.0)
        infoStr = std::format(L"{} ({:%T})", stemName, totalDuration);
    else
        infoStr = std::format(L"{} ({:%M:%S})", stemName, totalDuration);

    return infoStr;
}

void TUIPlayerUI::OnAudioBegin(uint32_t streamIdx, const CMediaTag& metaInfo, const std::wstring& name, float totalSeconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);

     try
    {
        uint32_t streamCount = metaInfo.QueryTagInteger(MediaTag_Streams).value_or(1);
        if (streamCount > 1)
        {
            m_infoStr = MakeNameInfoText(name, totalSeconds);
            std::wstring postfix = std::format(L" - {}/{}", (streamIdx + 1), streamCount);
            m_infoStr += postfix;
            //SetWindowTitleWithSongInfo(infoStr.c_str());
        }
        else
        {
            m_infoStr = MakeNameInfoText(name, totalSeconds);
            //SetWindowTitleWithSongInfo(infoStr.c_str());
        }

        m_title = metaInfo.QueryTagString(MediaTag_Title).value_or("");
        m_brief = metaInfo.QueryTagString(MediaTag_Brief).value_or("");
        std::string artist = metaInfo.QueryTagString(MediaTag_Artists).value_or("");
        m_album = metaInfo.QueryTagString(MediaTag_Album).value_or("");
        if(m_title.empty())
            m_title = Utf16ToUtf8(GetFileNamePart(name));
        if(!artist.empty()) 
        {
            m_title += " - ";
            m_title += artist;
        }

        m_totalSeconds = totalSeconds;
        m_currentSeconds = 0.0f;
        m_status = PlaybackStatus::Playing;
    }  
    catch (std::exception& e)
    {
    }
}

void TUIPlayerUI::OnAudioUpdate(float curSeconds, float* powerBands, int bands)
{
    (void)powerBands;
    (void)bands;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentSeconds = curSeconds;
}

bool TUIPlayerUI::OnAudioEnd(bool lastStream)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status = PlaybackStatus::Stoped;
    m_currentSeconds = m_totalSeconds;

    // Return false to indicate no new audio source is being started
    // The playback system will close the current source
    return false;
}

void TUIPlayerUI::OnControlEvent(PlayControl ctrl)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    switch (ctrl)
    {
        case PlayControl::Play:
            m_status = PlaybackStatus::Playing;
            break;

        case PlayControl::Pause:
            m_status = PlaybackStatus::Paused;
            break;

        case PlayControl::Stop:
            m_status = PlaybackStatus::Stoped;
            m_currentSeconds = 0.0f;
            break;

        default:
            break;
    }
}

void TUIPlayerUI::OnVolumeChanged(BOOL bMute, int vol)
{
    (void)bMute;
    (void)vol;

    // Store volume/mute state if needed for UI display
    // For now, this is a no-op as the callback focuses on playback state
}

float TUIPlayerUI::GetCurrentPosition()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSeconds;
}

float TUIPlayerUI::GetTotalSeconds()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalSeconds;
}

PlaybackStatus TUIPlayerUI::GetStatus()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

void TUIPlayerUI::BuildUI()
{
    // ---------- Buttons ----------
    m_btn_close = Button(" ✕ ", [&] { Exit(); }, ButtonOption::Animated(Color::RedLight)) | size(WIDTH, EQUAL, 6);
    m_btn_prev = Button(" << ", [&] { OnSeekBackward(); }, ButtonOption::Animated(Color::Green)) | size(WIDTH, EQUAL, 6) ;
    m_btn_play = Button("  > ", [&] { OnPlayPause(); }, ButtonOption::Animated(Color::Green)) | size(WIDTH, EQUAL, 6);
    m_btn_pause = Button(" || ", [&] { OnPlayPause(); }, ButtonOption::Animated(Color::Green)) | size(WIDTH, EQUAL, 6);
    m_btn_stop = Button(" ▀ ", [&] { OnStop(); }, ButtonOption::Animated(Color::Green)) | size(WIDTH, EQUAL, 6);
    m_btn_next = Button(" >> ", [&] { OnSeekForward(); }, ButtonOption::Animated(Color::Green)) | size(WIDTH, EQUAL, 6);

    auto controls =
        Container::Horizontal({
            m_btn_prev,
            m_btn_play,
            m_btn_pause,
            m_btn_stop,
            m_btn_next,
            });

    // ⭐ 唯一 container（防止焦点抖动）
    m_main_container =
        Container::Vertical({
            m_btn_close,
            controls,
            });

    //
    // ---------- Renderer ----------
    //
    m_root = Renderer(m_main_container, [&] {
        auto progStatus = GetProgressStatus();
        auto metsInfo = GetMusicMetaInfo();
        auto mediaFmt = GetMusicFormatStatus();

        // ===== Title bar =====
        auto title_bar =
            hbox({
                text(" iPlayer v0.1") | bold,
                filler(),
                m_btn_close->Render(),
                }) | bgcolor(Color::Blue) | color(Color::White) | size(HEIGHT, EQUAL, 2);

        // ===== Info =====
        auto info =
            vbox({
                text("Title : " + metsInfo.first),
                text("Album : " + metsInfo.second),
                text("Format : " + mediaFmt),
                })
            | bgcolor(Color::RGB(40, 40, 40))
            | color(Color::White)
            | border;

        // ===== Progress =====
        auto progress =
            hbox({
                gauge(progStatus.first) | flex,
                text(" " + progStatus.second),
                })
                | bgcolor(Color::Black)
            | border;

        // ===== Controls =====
        auto control_bar =
            hbox({
                filler(),
                m_btn_prev->Render(),
                text("  "),
                m_btn_play->Render(),
                text("  "),
                m_btn_pause->Render(),
                text("  "),
                m_btn_stop->Render(),
                text("  "),
                m_btn_next->Render(),
                filler(),
                })
                | bgcolor(Color::Gold3) | size(HEIGHT, EQUAL, 3) | border;

        return vbox({
            title_bar,
            info,
            progress,
            control_bar,
            });
        });
}

std::pair<float, std::string> TUIPlayerUI::GetProgressStatus()
{
    std::lock_guard lock(m_mutex);

    std::chrono::duration<float> totalDuration(m_totalSeconds);
    std::chrono::duration<float> curSeconds(m_currentSeconds);
    float prog = m_currentSeconds / m_totalSeconds;
    std::string status = std::format("{:%M:%S}/{:%M:%S}", curSeconds, totalDuration);

    return {prog, status};
}

std::pair<std::string, std::string> TUIPlayerUI::GetMusicMetaInfo()
{
    std::lock_guard lock(m_mutex);

    return {m_title, m_album};
}

std::string TUIPlayerUI::GetMusicFormatStatus()
{
    std::lock_guard lock(m_mutex);

    return m_brief;
}  

void TUIPlayerUI::OnPlayPause()
{
    if (!m_playback)
        return;

    PlaybackStatus status = GetStatus();
    if (status == PlaybackStatus::Playing)
    {
        m_playback->Pause();
        //PlaybackStatus status =m_playback->GetPlaybackStatus();        
    }
    else
    {
        if (m_playback->HasAudioSource())
        {
            //m_playSeconds = 0.0f;
            m_playback->Play();
        }        
    }
}

void TUIPlayerUI::OnStop()
{
    if (m_playback)
    {
        m_playback->Stop();
    }
}

void TUIPlayerUI::OnSeekForward()
{
    if (!m_playback)
        return;

    float currentPos = GetCurrentPosition();
    float totalSeconds = GetTotalSeconds();
    float newPos = currentPos + 10.0f; // Seek forward 10 seconds

    if (newPos > totalSeconds)
        newPos = totalSeconds;

    m_playback->SeekPosition(newPos);
}

void TUIPlayerUI::OnSeekBackward()
{
    if (!m_playback)
        return;

    float currentPos = GetCurrentPosition();
    float newPos = currentPos - 10.0f; // Seek backward 10 seconds

    if (newPos < 0.0f)
        newPos = 0.0f;

    m_playback->SeekPosition(newPos);
}

void TUIPlayerUI::Run()
{
    if (!m_playback)
        return;

    m_running = true;

    //m_screen = ScreenInteractive::TerminalOutput();
    BuildUI();
    // Main event loop
    m_screen.Loop(m_root);

    m_running = false;
}

void TUIPlayerUI::Exit() 
{
    if(!m_running)
        return;
        
    if(m_playback)
    {
        m_playback->Shutdown();
    }
    m_running = false;
    m_screen.Exit();
}
