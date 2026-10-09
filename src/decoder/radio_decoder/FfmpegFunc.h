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

uint32_t StreamFormatFromFfmpeg(const char* inputFmtName, const char* filenameUtf8, int audioCodecId);
uint32_t ParseStreamFormatByFfmpeg(const char* filenameUtf8);
AudioDataFormat AudioDataFormatFromFfmpegCodec(int codecId);
const char* FfmpegFormatName(uint32_t streamFmt);
