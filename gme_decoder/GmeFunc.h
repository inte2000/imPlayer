/*
20260522 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_66.txt
*/
#pragma once

#include <cstdint>

#include "DataStream.h"

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

uint32_t ParseStreamFormatByGme(const char* filenameUtf8, CDataStream* pStream = nullptr);
const char* GmeStreamFormatName(uint32_t streamFmt);
