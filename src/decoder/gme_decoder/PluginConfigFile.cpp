/*
20260522 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_66.txt
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
            reinterpret_cast<LPCWSTR>(&GetGmePluginConfigPath),
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

std::filesystem::path GetGmePluginConfigPath()
{
    std::filesystem::path cfgPath = GetPluginModuleDirectory();
    cfgPath /= "config";
    cfgPath /= "gme_decoder.config";
    return cfgPath;
}

bool LoadGmePluginConfigFile(const std::filesystem::path& configPath, PluginConfig& config)
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
    }
    catch (...) {
        return false;
    }

    return true;
}

bool SaveGmePluginConfigFile(const std::filesystem::path& configPath, const PluginConfig& config)
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
        ofs << cfgJson.dump(4);
    }
    catch (...) {
        return false;
    }

    return true;
}
