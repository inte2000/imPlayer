#include "framework.h"
#include <cassert>
#include "SpeakerConfig.h"


std::optional<DeviceFormat> CSpeakerConfig::GetDeviceFormat(const std::string& deviceId)
{
    auto rtn = (deviceId.empty() || (deviceId == "Default")) ? 
               QueryDeviceFormat(m_curDefaultId) : QueryDeviceFormat(deviceId);
    
    return rtn;
}

void CSpeakerConfig::SetDeviceFormat(const std::string& deviceId, const std::string& name, const AudioFormat& deviceFmt)
{
    for (auto& devFmt : m_devFmtMap)
    {
        if (devFmt.deviceId == deviceId)
        {
            devFmt.deviceName = name;
            devFmt.deviceFmt = deviceFmt;
            return;
        }
    }

    m_devFmtMap.emplace_back(deviceId, name, deviceFmt);
}

std::optional<DeviceFormat> CSpeakerConfig::QueryDeviceFormat(const std::string& deviceId)
{
    for(const auto& devFmt : m_devFmtMap)
    {
        if(devFmt.deviceId == deviceId)
        {
            return devFmt;
        }
    }
    
    return std::nullopt;
}

std::unique_ptr<CSpeakerConfig> LoadSpeakerConfig(const std::string& filename)
{
    std::unique_ptr<CSpeakerConfig> cfg = std::make_unique<CSpeakerConfig>();
    if (cfg)
    {
    }

    return cfg;
}

bool SaveSpeakerConfig(const std::string& filename, const CSpeakerConfig* cfg)
{
    return true;
}

