/*
此文件内容为 AI 生成
大模型：GPT 5.3 Codex
任务说明：todo_task_5.txt
第一次修改：todo_task_7.txt
*/
#pragma once

#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>
#include "PluginObjects.h"

bool LoadPluginConfigJsonFile(const std::string& filename, nlohmann::json& pluginJson);
bool ParsePluginConfigJson(const nlohmann::json& pluginJson, std::vector<PluginConfig>& plusItems);

bool BuildPluginConfigJson(const std::vector<PluginConfig>& plusItems, nlohmann::json& pluginJson);
bool SavePluginConfigJsonFile(const std::string& filename, const nlohmann::json& pluginJson);

bool LoadPluginConfigFile(const std::string& filename, std::vector<PluginConfig>& plusItems);
bool SavePluginConfigFile(const std::string& filename, const std::vector<PluginConfig>& plusItems);
