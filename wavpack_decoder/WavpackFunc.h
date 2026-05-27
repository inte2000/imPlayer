/*
20260527 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_84.txt
*/
#pragma once

#include <cstdint>

void SetWavpackCustomFormatBase(uint32_t formatIdBase);
uint32_t WavpackFormatWv();
uint32_t ParseStreamFormatByWavpack(const char* filenameUtf8);
