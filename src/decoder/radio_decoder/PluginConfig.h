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

#include <cstdint>

#include "AudioInfo.h"

typedef struct tagPluginConfig
{
    uint32_t DefaultSampleRate = 48000;
    uint32_t DefaultBitsPerSample = 32;
    uint32_t DefaultChannels = 2;
    uint32_t DefaultPcmFormat = static_cast<uint32_t>(AudioDataFormat::Float32);
} PluginConfig;
