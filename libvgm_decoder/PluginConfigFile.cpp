#include <fstream>
#include <windows.h>

#include <nlohmann/json.hpp>

#include "PluginConfigFile.h"

using json = nlohmann::json;

namespace {

std::filesystem::path GetPluginModuleDirectory()
{
    HMODULE moduleHandle = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&GetVgmPluginConfigPath),
            &moduleHandle)) {
        return {};
    }

    wchar_t modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(moduleHandle, modulePath, MAX_PATH);
    if (length == 0) {
        return {};
    }

    return std::filesystem::path(modulePath).parent_path();
}

} // namespace

std::filesystem::path GetVgmPluginConfigPath()
{
    std::filesystem::path cfgPath = GetPluginModuleDirectory();
    cfgPath /= "config";
    cfgPath /= "libvgm_decoder.config";
    return cfgPath;
}

bool LoadVgmPluginConfigFile(const std::filesystem::path& configPath, PluginConfig& config)
{
    std::ifstream ifs(configPath);
    if (!ifs.is_open()) {
        return false;
    }

    try {
        json cfgJson;
        ifs >> cfgJson;

        config.SampleRate = cfgJson.value("SampleRate", config.SampleRate);
        config.BitsPerSample = cfgJson.value("BitsPerSample", config.BitsPerSample);
        config.Loops = cfgJson.value("Loops", config.Loops);
        config.FadeLen = cfgJson.value("FadeLen", config.FadeLen);
    }
    catch (...) {
        return false;
    }

    return true;
}

bool SaveVgmPluginConfigFile(const std::filesystem::path& configPath, const PluginConfig& config)
{
    std::error_code ec;
    std::filesystem::create_directories(configPath.parent_path(), ec);

    std::ofstream ofs(configPath);
    if (!ofs.is_open()) {
        return false;
    }

    try {
        json cfgJson;
        cfgJson["SampleRate"] = config.SampleRate;
        cfgJson["BitsPerSample"] = config.BitsPerSample;
        cfgJson["Loops"] = config.Loops;
        cfgJson["FadeLen"] = config.FadeLen;
        ofs << cfgJson.dump(4);
    }
    catch (...) {
        return false;
    }

    return true;
}
