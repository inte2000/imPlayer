/*
20260523 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_53.txt

修改记录：
大模型：ChatGPT 5.3 Codex
todo_task_54.txt
todo_task_57.txt
todo_task_58.txt
todo_task_59.txt
*/
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
            reinterpret_cast<LPCWSTR>(&GetFfmpegPluginConfigPath),
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

std::filesystem::path GetFfmpegPluginConfigPath()
{
    std::filesystem::path cfgPath = GetPluginModuleDirectory();
    cfgPath /= "config";
    cfgPath /= "ffmpeg_decoder.config";
    return cfgPath;
}

bool LoadFfmpegPluginConfigFile(const std::filesystem::path& configPath, PluginConfig& config)
{
    std::ifstream ifs(configPath);
    if (!ifs.is_open()) {
        return false;
    }

    try {
        json cfgJson;
        ifs >> cfgJson;
        config.DefaultSampleRate = cfgJson.value("DefaultSampleRate", config.DefaultSampleRate);
        config.DefaultBitsPerSample = cfgJson.value("DefaultBitsPerSample", config.DefaultBitsPerSample);
        config.DefaultChannels = cfgJson.value("DefaultChannels", config.DefaultChannels);
        config.DefaultPcmFormat = cfgJson.value("DefaultPcmFormat", config.DefaultPcmFormat);
    }
    catch (...) {
        return false;
    }

    return true;
}

bool SaveFfmpegPluginConfigFile(const std::filesystem::path& configPath, const PluginConfig& config)
{
    std::error_code ec;
    std::filesystem::create_directories(configPath.parent_path(), ec);

    std::ofstream ofs(configPath);
    if (!ofs.is_open()) {
        return false;
    }

    try {
        json cfgJson;
        cfgJson["DefaultSampleRate"] = config.DefaultSampleRate;
        cfgJson["DefaultBitsPerSample"] = config.DefaultBitsPerSample;
        cfgJson["DefaultChannels"] = config.DefaultChannels;
        cfgJson["DefaultPcmFormat"] = config.DefaultPcmFormat;
        ofs << cfgJson.dump(4);
    }
    catch (...) {
        return false;
    }

    return true;
}
