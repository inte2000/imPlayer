/*
20260522 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_66.txt
*/
#pragma once

#include <cstdint>

void SetGmeCustomFormatBase(uint32_t formatIdBase);

uint32_t GmeFormatAy();
uint32_t GmeFormatGbs();
uint32_t GmeFormatGym();
uint32_t GmeFormatHes();
uint32_t GmeFormatKss();
uint32_t GmeFormatNsf();
uint32_t GmeFormatNsfe();
uint32_t GmeFormatSap();
uint32_t GmeFormatSpc();
uint32_t GmeFormatVgm();

uint32_t ParseStreamFormatByGme(const char* filenameUtf8);
const char* GmeStreamFormatName(uint32_t streamFmt);
