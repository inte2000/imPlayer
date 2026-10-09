/*
20260527 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_84.txt
*/
#pragma once

#include <cstdint>

#include "DataStream.h"

void SetWavpackCustomFormatBase(uint32_t formatIdBase);
uint32_t WavpackFormatWv();
uint32_t ParseStreamFormatByWavpack(const char* filenameUtf8, CDataStream* pStream = nullptr);
