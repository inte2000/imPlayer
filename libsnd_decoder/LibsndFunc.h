/*
20260424 AI 生成内容：
uint32_t StreamFormatFromLibsndfileFormat(int format);
uint32_t ParseStreamFormatByLibsndfile(const char* filenameUtf8);

大模型：GPT 5.3 Codex
任务：todo_task_12.txt


20260425 AI 生成内容：
std::string SndfileGetFileTypeName(uint32_t type);
AudioDataFormat SndfileTransSubType(int sndsubtype);

大模型：GPT 5.3 Codex
任务：todo_task_19.txt
*/
#pragma once

#include <cstdint>
#include <string>
#include "AudioInfo.h"

uint32_t StreamFormatFromLibsndfileFormat(int format);
uint32_t ParseStreamFormatByLibsndfile(const char* filenameUtf8);
std::string SndfileGetFileTypeName(uint32_t type);
AudioDataFormat SndfileTransSubType(int sndsubtype);
