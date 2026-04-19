#ifndef SPEAKER_CONFIG_H
#define SPEAKER_CONFIG_H

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include "AudioInfo.h"

typedef struct tagDeviceFormat
{
    std::string deviceId;
    std::string deviceName;
    AudioFormat deviceFmt;
}DeviceFormat;

class CSpeakerConfig
{
    friend std::unique_ptr<CSpeakerConfig> LoadSpeakerConfig(const std::string& filename);
    friend bool SaveSpeakerConfig(const std::string& filename, const CSpeakerConfig* cfg);
public:
    CSpeakerConfig() {};

    std::optional<DeviceFormat> GetDeviceFormat(const std::string& deviceId);
    void SetDefaultDeviceId(const std::string& deviceId) { m_curDefaultId = deviceId; }
    void SetDeviceFormat(const std::string& deviceId, const std::string& name, const AudioFormat& deviceFmt);
protected:
    std::optional<DeviceFormat> QueryDeviceFormat(const std::string& deviceId);
private:
    std::string m_curDefaultId;
    std::vector<DeviceFormat> m_devFmtMap;
};

std::unique_ptr<CSpeakerConfig> LoadSpeakerConfig(const std::string& filename);
bool SaveSpeakerConfig(const std::string& filename, const CSpeakerConfig* cfg);

#endif //SPEAKER_CONFIG_H

