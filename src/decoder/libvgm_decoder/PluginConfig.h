/*
20260519 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_37.txt
*/
#pragma once

#include <cstdint>

typedef struct tagPluginConfig
{
    uint32_t SampleRate = 44100;
    uint32_t BitsPerSample = 32;
    uint32_t Loops = 1;
    float FadeLen = 2.0f;
} PluginConfig;
