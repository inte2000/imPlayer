/*
此文件内容为 AI 生成
大模型：GPT 5.3 Codex
任务说明：todo_task_5.txt
*/
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "PluginConfig.h"

using json = nlohmann::json;

bool LoadPluginConfigJsonFile(const std::string& filename, json& pluginJson)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open())
    {
        return false;
    }

    try
    {
        ifs >> pluginJson;
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool ParsePluginConfigJson(const json& pluginJson, std::vector<PluginConfig>& plusItems)
{
    if (!pluginJson.contains("plugins") || !pluginJson["plugins"].is_array())
    {
        return false;
    }

    try
    {
        plusItems.clear();

        for (const auto& item : pluginJson["plugins"])
        {
            PluginConfig pluginItem;
            pluginItem.hostfile = item.value("hostfile", "");
            pluginItem.name = item.value("name", "");
            pluginItem.publisher = item.value("publisher", "");
            pluginItem.type = item.value("type", "");
            plusItems.push_back(std::move(pluginItem));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool BuildPluginConfigJson(const std::vector<PluginConfig>& plusItems, json& pluginJson)
{
    try
    {
        pluginJson = json::object();
        pluginJson["plugins"] = json::array();

        for (const auto& item : plusItems)
        {
            json node;
            node["hostfile"] = item.hostfile;
            node["name"] = item.name;
            node["publisher"] = item.publisher;
            node["type"] = item.type;

            pluginJson["plugins"].push_back(std::move(node));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool SavePluginConfigJsonFile(const std::string& filename, const json& pluginJson)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open())
    {
        return false;
    }

    try
    {
        ofs << pluginJson.dump(4);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool LoadPluginConfigFile(const std::string& filename, std::vector<PluginConfig>& plusItems)
{
    json pluginJson;
    if (!LoadPluginConfigJsonFile(filename, pluginJson))
    {
        return false;
    }

    return ParsePluginConfigJson(pluginJson, plusItems);
}

bool SavePluginConfigFile(const std::string& filename, const std::vector<PluginConfig>& plusItems)
{
    json pluginJson;
    if (!BuildPluginConfigJson(plusItems, pluginJson))
    {
        return false;
    }

    return SavePluginConfigJsonFile(filename, pluginJson);
}
