/*
20260508 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_35.txt
*/
#pragma once

#include <cstdint>

void SetLibvgmCustomFormatBase(uint32_t formatIdBase);
uint32_t LibvgmFormatS98();
uint32_t LibvgmFormatGym();
uint32_t ParseStreamFormatByLibvgm(const char* filenameUtf8);
const char* LibvgmFormatName(uint32_t streamFmt);
