#include <iostream>
#include <thread>
#include <mutex>
#include <conio.h>
#include "AudioSource.h"
#include "WasapiAudioDevice.h"
#include "DsAudioDevice.h"
#include "UnicodeConvert.h"
#include "AudioDeviceMgmt.h"
#include "Playback.h"
#include "ScopeGuard.h"
#include "TUIPlayerUI.h"
#include "PlayInterface.h"

static std::string s_deviceId, s_devideName, s_deviceType;

class PlaybackInterface : public PlaybackCallback
{
public:
    void OnAudioBegin(const AudioFormat& audioFmt, const std::string& extraInfo, const std::wstring& name, float totalSeconds) override 
    {
        m_curSeconds = 0.0f;
        std::wcout << L"Audio playing start: " << name << std::endl;
    }
    void OnAudioUpdate(float curSeconds, float *powerBands, int bands) override
    {
        if(std::fabs(m_curSeconds - curSeconds) > 1.0f)
        {
            m_curSeconds = curSeconds;
            std::cout << "\rAudio playing seconds: " << m_curSeconds << std::flush;
        }
    }
    
    bool OnAudioEnd() override
    {
        std::cout << "Audio playing end: " << std::endl;
        return true;
    }
    void OnControlEvent(PlayControl ctrl) override
    {

    }
    void OnVolumeChanged(BOOL bMute, int vol) override
    {

    }
protected:
    float m_curSeconds;
};

static std::unique_ptr<CAudioDevice> MakeAudioDevice(const std::string& type, const std::string& name, const std::string& deviceId)
{
    if (type.compare("Native") == 0)
    {
        if (name.compare("Wasapi (Share mode)") == 0)
        {
            return std::make_unique<CWasapiAudioDevice>(deviceId, WasapiMode::Share);
        }
        else if (name.compare("Wasapi (Exclusive mode)") == 0)
        {
            return std::make_unique<CWasapiAudioDevice>(deviceId, WasapiMode::Exclusive);
        }
        else if (name.compare("DirectSound") == 0)
        {
            return std::make_unique<CDsAudioDevice>(deviceId);
        }
        else
            throw std::runtime_error("Invalid audio device name, check the config file and correct it!");
    }
    else if (type.compare("Plugin") == 0)
    {
        throw std::runtime_error("Invalid audio device type, current not support!");
    }
    else
        throw std::runtime_error("Invalid audio device type, check the config file and correct it!");
}

void SetupDevice(const std::string& type, const std::string& deviceName, const std::string& deviceId)
{
    s_deviceType = type;
    s_devideName = deviceName;
    s_deviceId = deviceId;
}

void StartPlayingInterface(const std::string& filename, bool bPlaylist, const std::string& speakerLayout)
{
    CAudioDeviceMgmt devMgmt; 

    PlaybackInterface pif;
    std::unique_ptr<CAudioDevice> audioDevice = MakeAudioDevice(s_deviceType, s_devideName, s_deviceId);
    std::shared_ptr<CPlayback> playback = CPlayback::Create(&pif, std::move(audioDevice));
    playback->SetOutputDeviceId(s_deviceId);

    //std::string utf8Speakerfile = GetSpeakerConfigFilePathname();
    std::unique_ptr<CSpeakerConfig> speakCfg = LoadSpeakerConfig(speakerLayout);
    playback->SetSpeakerConfig(std::move(speakCfg));
    
    //std::wstring wfilename = UTtf8ToUtf16Le(filename);
    std::wstring wfilename = LocalMBCSToUtf16Le(filename);
    std::unique_ptr<CAudioSource> audioSource = MakeFileAudioSource(wfilename);    
    playback->SetAudioSource(std::move(audioSource), true);
    
    char userKey = ' ';
    while(userKey != 'c') 
    {
        if(userKey == 's')
        {
            if (playback->HasAudioSource())
            {
                //m_playSeconds = 0.0f;
                playback->Play();
            }
        }
        else if(userKey == 'p')
        {
            if (playback->HasAudioSource())
            {
                playback->Pause();
            } 
        }
        else if(userKey == 'e')
        {
            if (playback->HasAudioSource())
            {
                playback->Stop();
            } 
        }
        
        std::cout << "Press c to exit:" << std::endl;
        userKey = _getch();
    }

    playback->Shutdown();
}

void StartPlayingTuiInterface(const std::string& filename, bool bPlaylist, const std::string& speakerLayout)
{
    std::unique_ptr<CAudioDevice> audioDevice = MakeAudioDevice(s_deviceType, s_devideName, s_deviceId);
    TUIPlayerUI tuiUI;
    if(!tuiUI.Init(std::move(audioDevice), s_deviceId, filename, bPlaylist, speakerLayout))
        throw std::runtime_error("Fail to init tui object!");
                
    tuiUI.Run();        
}