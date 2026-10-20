/*
此文件内容为 AI 生成
大模型：GPT 5.3 Codex
任务说明：todo_task_6.txt
*/
#include <utility>
#include "UnicodeConvert.h"
#include "PluginsMgmt.h"

std::optional<PluginDllObject> CreatePluginObjectByConfig(const PluginConfig& plugCfg)
{
    if (plugCfg.type != "Decoder" && plugCfg.type != "Encoder") {
        return std::nullopt;
    }
    auto obj = CreatePluginObjectByFile(UTtf8ToUtf16Le(plugCfg.hostfile));
    if (!obj || obj->type != plugCfg.type) {
        return std::nullopt;
    }
    obj->name = plugCfg.name;
    obj->publisher = plugCfg.publisher;
    return obj;
}

std::optional<PluginDllObject> CreatePluginObjectByFile(const std::wstring& plugFile)
{
    if (auto dll = MakeDecoderWrapper(plugFile)) {
        PluginInfo info{};
        info.size = sizeof(info);
        if (dll->GetPluginInformation(&info) != 0 || info.plug_type != PluginType::Decoder) {
            return std::nullopt;
        }
        info.name[sizeof(info.name) - 1] = '\0';
        info.publisher[sizeof(info.publisher) - 1] = '\0';
        return PluginDllObject{ info.name, info.publisher, "Decoder", std::move(dll), nullptr };
    }

    auto dll = MakeEncoderWrapper(plugFile);
    if (!dll) {
        return std::nullopt;
    }
    PluginInfo info{};
    info.size = sizeof(info);
    if (dll->GetPluginInformation(&info) != 0 || info.plug_type != PluginType::Encoder) {
        return std::nullopt;
    }
    info.name[sizeof(info.name) - 1] = '\0';
    info.publisher[sizeof(info.publisher) - 1] = '\0';
    return PluginDllObject{ info.name, info.publisher, "Encoder", nullptr, std::move(dll) };
}
