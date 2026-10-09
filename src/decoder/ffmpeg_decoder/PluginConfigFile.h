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
#pragma once

#include <filesystem>

#include "PluginConfig.h"

std::filesystem::path GetFfmpegPluginConfigPath();
bool LoadFfmpegPluginConfigFile(const std::filesystem::path& configPath, PluginConfig& config);
bool SaveFfmpegPluginConfigFile(const std::filesystem::path& configPath, const PluginConfig& config);
