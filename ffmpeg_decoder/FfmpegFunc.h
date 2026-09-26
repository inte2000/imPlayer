/*
20260523 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_53.txt

�޸ļ�¼��
��ģ�ͣ�ChatGPT 5.3 Codex
todo_task_54.txt
todo_task_57.txt
todo_task_58.txt
todo_task_59.txt
*/
#pragma once

#include <cstdint>

#include "AudioInfo.h"
#include "DataStream.h"

uint32_t StreamFormatFromFfmpeg(const char* inputFmtName, const char* filenameUtf8, int audioCodecId);
uint32_t ParseStreamFormatByFfmpeg(const char* filenameUtf8, CDataStream* pStream = nullptr);
AudioDataFormat AudioDataFormatFromFfmpegCodec(int codecId);
const char* FfmpegFormatName(uint32_t streamFmt);
