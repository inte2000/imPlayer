/*
20260522 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_66.txt
*/
#pragma once

#include <cstdint>

typedef struct tagPluginConfig
{
    uint32_t SampleRate = 44100;
    uint32_t BitsPerSample = 32;
	double EqTreble = 0; /* -50.0 = muffled, 0 = flat, +5.0 = extra-crisp */
	double EqBass = 90;   /* 1 = full bass, 90 = average, 16000 = almost no bass */
    bool EnableAccuracy = true; //gme_enable_accuracy 

} PluginConfig;
