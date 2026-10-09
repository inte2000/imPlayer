/*
20260522 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_66.txt
*/
#pragma once

#include <filesystem>

#include "PluginConfig.h"

std::filesystem::path GetGmePluginConfigPath();
bool LoadGmePluginConfigFile(const std::filesystem::path& configPath, PluginConfig& config);
bool SaveGmePluginConfigFile(const std::filesystem::path& configPath, const PluginConfig& config);
