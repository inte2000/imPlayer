#pragma once

#include <filesystem>

#include "PluginConfig.h"

std::filesystem::path GetVgmPluginConfigPath();
bool LoadVgmPluginConfigFile(const std::filesystem::path& configPath, PluginConfig& config);
bool SaveVgmPluginConfigFile(const std::filesystem::path& configPath, const PluginConfig& config);
