/*
此文件内容为 AI 生成
大模型：GPT 5.3 Codex
任务说明：todo_task_6.txt
第一次修改：todo_task_7.txt
*/
#pragma once

#include <optional>
#include <string>

#include "PluginObjects.h"

std::optional<PluginDllObject> CreatePluginObjectByConfig(const PluginConfig& plugCfg);
std::optional<PluginDllObject> CreatePluginObjectByFile(const std::wstring& plugFile);

