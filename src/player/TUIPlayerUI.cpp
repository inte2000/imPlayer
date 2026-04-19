#include "framework.h"
#include "UnicodeConvert.h"
#include "TUIPlayerUI.h"
#include <format>
#include <cmath>

using namespace ftxui;

TUIPlayerUI::TUIPlayerUI()
    : m_running(false)
    , m_volume(50)
    , m_seekPosition(0.0f)
    , m_showVolume(false)
    , m_currentSeconds(0.0f)
    , m_totalSeconds(0.0f)
    , m_status(PlaybackStatus::Stoped){
}

TUIPlayerUI::~TUIPlayerUI()
{
    Stop();
}

bool TUIPlayerUI::Init(std::unique_ptr<CAudioDevice> audioDevice, const std::string& deviceId, const std::string& filename, bool bPlaylist, const std::string& speakerLayout)
{
    m_playback = CPlayback::Create(nullptr, std::move(audioDevice));
    m_playback->SetOutputDeviceId(deviceId);
   
    std::unique_ptr<CSpeakerConfig> speakCfg = LoadSpeakerConfig(speakerLayout);
    m_playback->SetSpeakerConfig(std::move(speakCfg));
    
    std::wstring wfilename = LocalMBCSToUtf16Le(filename);
    std::unique_ptr<CAudioSource> audioSource = MakeFileAudioSource(wfilename);    
    m_playback->SetAudioSource(std::move(audioSource), true);

    return true;
}

void TUIPlayerUI::OnAudioBegin(const AudioFormat& audioFmt, const std::string& extraInfo, const std::wstring& name, float totalSeconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_audioFormat = audioFmt;
    m_extraInfo = extraInfo;
    m_songName = name;
    m_totalSeconds = totalSeconds;
    m_currentSeconds = 0.0f;
    m_status = PlaybackStatus::Playing;
    
    // Parse metadata from extraInfo JSON
    //m_title = ExtractJsonField(extraInfo, "title");
    //m_artist = ExtractJsonField(extraInfo, "artist");
    //m_album = ExtractJsonField(extraInfo, "album");
}

void TUIPlayerUI::OnAudioUpdate(float curSeconds, float* powerBands, int bands)
{
    (void)powerBands;
    (void)bands;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentSeconds = curSeconds;
}

bool TUIPlayerUI::OnAudioEnd()
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

std::wstring TUIPlayerUI::GetSongName()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_songName;
}

AudioFormat TUIPlayerUI::GetAudioFormat()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_audioFormat;
}

std::string TUIPlayerUI::GetExtraInfo()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_extraInfo;
}

PlaybackStatus TUIPlayerUI::GetStatus()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

std::string TUIPlayerUI::GetTitle()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_title;
}

std::string TUIPlayerUI::GetArtist()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_artist;
}

std::string TUIPlayerUI::GetAlbum()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_album;
}

std::string TUIPlayerUI::FormatTime(float seconds)
{
    if (seconds < 0.0f)
        seconds = 0.0f;
    int totalSeconds = static_cast<int>(seconds);
    int minutes = totalSeconds / 60;
    int secs = totalSeconds % 60;
    return std::format("{:02d}:{:02d}", minutes, secs);
}

Element TUIPlayerUI::BuildSongInfoDisplay()
{
    std::wstring songName = GetSongName();
    AudioFormat format = GetAudioFormat();
    std::string extraInfo = GetExtraInfo();

    // Format audio information
    std::string formatInfo = std::format("{} Hz, {} ch, {} bit",
        format.sampleRate,
        format.numChannels,
        format.bytesPerSample * 8);

    return vbox({
        text("Title: ") | bold,
        text(songName),
        text(""),
        text("Format: ") | bold,
        text(formatInfo),
        text(extraInfo.empty() ? "" : "Info: " + extraInfo),
    });
}

Component TUIPlayerUI::CreateSongInfoSection()
{
    return Renderer([this] {
        auto title = text("Now Playing") | bold | color(Color::Blue);
        return window(title, BuildSongInfoDisplay()) | flex;
    });
}

Component TUIPlayerUI::CreateProgressBar()
{
    return Renderer([this] {
        float currentPos = GetCurrentPosition();
        float totalSeconds = GetTotalSeconds();

        std::string currentTime = FormatTime(currentPos);
        std::string totalTime = FormatTime(totalSeconds);

        float progress = 0.0f;
        if (totalSeconds > 0.0f)
        {
            progress = currentPos / totalSeconds;
            if (progress > 1.0f)
                progress = 1.0f;
        }

        return hbox({
            text(currentTime),
            filler(),
            gauge(progress) | size(WIDTH, GREATER_THAN, 20),
            filler(),
            text(totalTime),
        });
    });
}

void TUIPlayerUI::OnPlayPause()
{
    if (!m_playback)
        return;

    PlaybackStatus status = GetStatus();
    if (status == PlaybackStatus::Playing)
    {
        m_playback->Pause();
    }
    else
    {
        m_playback->Play();
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

Component TUIPlayerUI::CreatePlaybackControls()
{
    return Renderer([this]() {
        // Play/Pause button text based on status
        std::string playPauseText = "▶"; // Play icon
        PlaybackStatus status = GetStatus();
        if (status == PlaybackStatus::Playing)
            playPauseText = "⏸"; // Pause icon

        return hbox(
            Button("◀◀", [this] {
            }) | size(WIDTH, EQUAL, 5),

            Button(playPauseText, [this] { OnPlayPause(); }) | size(WIDTH, EQUAL, 5),

            Button("⏹", [this] { OnStop(); }) | size(WIDTH, EQUAL, 5),

            Button("▶▶", [this] {
            }) | size(WIDTH, EQUAL, 5),

            Button("🔊", [this] { m_showVolume = !m_showVolume; }) | size(WIDTH, EQUAL, 5)
        ) | center;
    });
}

Component TUIPlayerUI::CreateVolumeSlider()
{
    return Renderer([this] {
        if (!m_showVolume)
            return text("");

        return vbox(
            text("Volume: " + std::to_string(m_volume) + "%"),
            Slider("Volume", &m_volume, 0, 100, 1) | size(WIDTH, GREATER_THAN, 20)
        );
    });
}

void TUIPlayerUI::Run()
{
    if (!m_playback)
        return;

    m_running = true;

    auto screen = ScreenInteractive::TerminalOutput();

    // Create volume slider component with update handler
    auto volumeSlider = Slider("Volume", &m_volume, 0, 100, 1);
#if 0    
    volumeSlider = Renderer(volumeSlider, [this, volumeSlider] {
            if (m_showVolume && m_playback)
            {
                m_playback->SetCurrentDeviceVolume(m_volume);
            }
            
            if (m_showVolume) {
                return vbox({
                    text("Volume: " + std::to_string(m_volume) + "%"),
                    volumeSlider->Render() | size(WIDTH, GREATER_THAN, 20)
                });
            } else {
                return text("");  // 或 nothing
            }
        });   
#endif 
 #if 1   
    volumeSlider = Renderer(volumeSlider, [this] {
        if (m_showVolume && m_playback)
        {
            // Update the actual volume
            m_playback->SetCurrentDeviceVolume(m_volume);
        }
        return text("");
    });
#endif
    // Volume display component
    auto volumeDisplay = Renderer([this, volumeSlider] {
        if (!m_showVolume)
            return text("");

        return vbox({
            text("Volume: " + std::to_string(m_volume) + "%"),
            volumeSlider->Render(),
        });
    });

    // Main layout
    auto layout = Container::Vertical({
        CreateSongInfoSection(),
        CreateProgressBar(),
        CreatePlaybackControls(),
        volumeDisplay,
    });

    // Main renderer
    auto component = Renderer(layout, [&] {
        return vbox({
            layout->Render() | border,
        });
    });

    // Main event loop
    screen.Loop(component);

    m_running = false;
}

void TUIPlayerUI::Stop()
{
    m_running = false;
}
