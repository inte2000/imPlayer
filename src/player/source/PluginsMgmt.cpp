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
    const std::wstring hostFile = UTtf8ToUtf16Le(plugCfg.hostfile);
    auto dll = MakeDecoderWrapper(hostFile);
    if (!dll)
    {
        return std::nullopt;
    }

    PluginDllObject obj;
    obj.name = plugCfg.name;
    obj.publisher = plugCfg.publisher;
    obj.type = plugCfg.type;
    obj.dll = dll;

    return obj;
}

std::optional<PluginDllObject> CreatePluginObjectByFile(const std::wstring& plugFile)
{
    auto dll = MakeDecoderWrapper(plugFile);
    if (!dll)
    {
        return std::nullopt;
    }

    PluginInfo info = {};
    info.size = sizeof(PluginInfo);

    if (dll->GetPluginInformation(&info) != 0)
    {
        return std::nullopt;
    }

    if (info.plug_type != PluginType::Decoder)
    {
        return std::nullopt;
    }

    PluginDllObject obj;
    obj.name = info.name;
    obj.publisher = info.publisher;
    obj.type = "Decoder";
    obj.dll = dll;

    return obj;
}
