/*
此文件内容为 AI 生成
大模型：GPT 5.3 Codex
任务说明：todo_task_7.txt
*/
#pragma once

#include <string>
#include "DecoderDllWrapper.h"

typedef struct tagPluginConfig
{
    std::string name;
    std::string publisher;
    std::string type;
    std::string hostfile;
} PluginConfig;

typedef struct tagPluginDllObject
{
    std::string name;
    std::string publisher;
    std::string type;
    std::shared_ptr<CDecoderDllWrapper> dll;
} PluginDllObject;
