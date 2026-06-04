#include <iostream>
#include <thread>
#include <mutex>
#include <conio.h>
#include <algorithm>
#include <cctype>
#include <format>
#include <optional>
#include <vector>
#include "AudioSource.h"
#include "AudioTarget.h"
#include "WasapiAudioDevice.h"
#include "DsAudioDevice.h"
#include "UnicodeConvert.h"
#include "AudioDeviceMgmt.h"
#include "Playback.h"
#include "EncodingParams.h"
#include "encoder/EncoderFactory.h"
#include "encoder/EncoderParamName.h"
#include "encoder/EncoderParamterDefineUtils.h"
#include "encoder/WavEncoder.h"
#include "ScopeGuard.h"
#include "TUIPlayerUI.h"
#include "PlayInterface.h"

static std::string s_deviceId, s_devideName, s_deviceType;

struct ConvertFormatItem
{
    std::string name;
    std::string encoderName;
    uint32_t streamFmt;
};

static const ConvertFormatItem s_convertFormats[] =
{
    { "wav", CWavEncoder::GetName(), StreamFormatWav }
};

static std::optional<ConvertFormatItem> FindConvertFormat(const std::string& ffmt)
{
    std::string lowerName = ffmt;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    for (const auto& item : s_convertFormats)
    {
        if (item.name == lowerName) {
            return item;
        }
    }

    return std::nullopt;
}

static std::optional<std::pair<AudioDataFormat, uint32_t>> ParseConvertCfmt(const std::string& cfmt)
{
    std::string upperFmt = cfmt;
    std::transform(upperFmt.begin(), upperFmt.end(), upperFmt.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    if (upperFmt == "S16") {
        return std::make_pair(AudioDataFormat::PCM_S16, 16U);
    }
    if (upperFmt == "S24") {
        return std::make_pair(AudioDataFormat::PCM_S24, 24U);
    }
    if (upperFmt == "S32") {
        return std::make_pair(AudioDataFormat::PCM_S32, 32U);
    }
    if (upperFmt == "U8") {
        return std::make_pair(AudioDataFormat::PCM_U8, 8U);
    }
    if (upperFmt == "F32") {
        return std::make_pair(AudioDataFormat::Float32, 32U);
    }
    if (upperFmt == "F64") {
        return std::make_pair(AudioDataFormat::Float64, 64U);
    }

    return std::nullopt;
}

static const EncoderParamterDefine* FindDefineByName(const std::vector<EncoderParamterDefine>& defs, const std::string& name)
{
    for (const auto& def : defs)
    {
        if (def.GetName() == name) {
            return &def;
        }
    }

    return nullptr;
}

static bool ValidateDefineValue(const EncoderParamterDefine& def, const EncoderParamter::ValueType& value)
{
    const auto& options = def.GetOptionValues();
    if (options.empty()) {
        return true;
    }

    return std::find(options.begin(), options.end(), value) != options.end();
}

static void SetEncoderParamByDefine(std::vector<EncoderParamter>& params, const EncoderParamterDefine& def, EncoderParamter::ValueType value)
{
    if (!ValidateDefineValue(def, value)) {
        throw std::runtime_error(std::format("invalid value for encoder parameter: {}", def.GetName()));
    }

    EncoderParamter* param = FindEncoderParamter(params, def.GetName());
    if (param != nullptr) {
        param->SetType(def.GetType());
        param->SetValue(std::move(value));
        return;
    }

    AddEncoderParamter(params, def.GetName(), def.GetType(), std::move(value));
}

class PlaybackInterface : public PlaybackCallback
{
public:
    void OnAudioBegin(uint32_t streamIdx, const CMediaTag& metaInfo, const std::wstring& name, float totalSeconds) override 
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
    
    bool OnAudioEnd(bool lastStream) override
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

int ConvertMediaFileInterface(const std::string& srcFilename,
    const std::string& outFilename,
    const std::string& formatName,
    uint32_t sampleRate,
    const std::string& cfmt,
    uint32_t channels)
{
    if (srcFilename.empty()) {
        throw std::runtime_error("missing source filename (-f)");
    }
    if (outFilename.empty()) {
        throw std::runtime_error("missing output filename (-out)");
    }
    if (sampleRate == 0) {
        throw std::runtime_error("invalid sample rate");
    }
    if (channels == 0) {
        throw std::runtime_error("invalid channels");
    }

    const auto fmtItem = FindConvertFormat(formatName);
    if (!fmtItem.has_value()) {
        throw std::runtime_error(std::format("unsupported convert format: {}", formatName));
    }

    const auto cfmtPair = ParseConvertCfmt(cfmt);
    if (!cfmtPair.has_value()) {
        throw std::runtime_error(std::format("unsupported cfmt: {}", cfmt));
    }

    const auto encoderItemOpt = CEncoderFactory::GetInstance().GetEncoderItem(fmtItem->encoderName);
    if (!encoderItemOpt.has_value()) {
        throw std::runtime_error(std::format("encoder not found for format {}: {}", formatName, fmtItem->encoderName));
    }

    const auto defs = encoderItemOpt->QueryParamters();
    std::vector<EncoderParamter> params = BuildDefaultEncoderParamters(defs);

    const auto* codeFormatDef = FindDefineByName(defs, EncoderParamName::CodeFormat);
    const auto* sampleRateDef = FindDefineByName(defs, EncoderParamName::SampleRates);
    const auto* bitsDef = FindDefineByName(defs, EncoderParamName::BitsPerSample);
    const auto* channelDef = FindDefineByName(defs, EncoderParamName::Channels);
    if ((codeFormatDef == nullptr) || (sampleRateDef == nullptr) || (bitsDef == nullptr) || (channelDef == nullptr)) {
        throw std::runtime_error("encoder parameter definition incomplete");
    }

    SetEncoderParamByDefine(params, *codeFormatDef, static_cast<uint32_t>(cfmtPair->first));
    SetEncoderParamByDefine(params, *sampleRateDef, sampleRate);
    SetEncoderParamByDefine(params, *bitsDef, cfmtPair->second);
    SetEncoderParamByDefine(params, *channelDef, channels);

    const std::wstring srcFilenameW = LocalMBCSToUtf16Le(srcFilename);
    const std::wstring outFilenameW = LocalMBCSToUtf16Le(outFilename);

    auto source = MakeFileAudioSource(srcFilenameW);
    auto target = MakeFileAudioTarget(outFilenameW, fmtItem->streamFmt, encoderItemOpt->name);
    if (!target) {
        throw std::runtime_error("failed to create audio target");
    }

    target->SetMetaInformation(source->GetMetaInformation());

    if (!target->InitEncoder(params)) {
        throw std::runtime_error("failed to initialize encoder");
    }

    const AudioFormat outFmt = target->GetAudioFormat();
    const uint32_t totalStreams = source->GetTotalAudioStreams();
    for (uint32_t streamIdx = 0; streamIdx < totalStreams; ++streamIdx)
    {
        if (!source->StartAudioStream(streamIdx)) {
            throw std::runtime_error(std::format("failed to start stream {}", streamIdx));
        }
        auto stopGuard = MakeGuard([&]() {
            source->StopAudioStream(streamIdx);
        });

        if (!source->SetOutputFormat(outFmt, false)) {
            throw std::runtime_error(std::format("failed to negotiate output format for stream {}", streamIdx));
        }

        const AudioFormat audioFmt = source->GetOutputFormat();
        const uint32_t bufSize = audioFmt.blockAlign * DECODE_BUF_FRAMES;
        if (bufSize == 0) {
            throw std::runtime_error("invalid negotiated output buffer size");
        }

        std::vector<uint8_t> buffer(bufSize);
        for (;;)
        {
            const uint32_t readFrames = source->ReadBuffer(buffer.data(), bufSize, DECODE_BUF_FRAMES);
            if (readFrames == 0) {
                break;
            }

            const uint32_t writeFrames = target->WriteBuffer(buffer.data(), readFrames, audioFmt);
            if (writeFrames != readFrames) {
                throw std::runtime_error("target write frames mismatch");
            }
        }

        if (target->FlushBuffer() == 0) {
            throw std::runtime_error("failed to flush encoder output");
        }
    }

    return 0;
}
