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

extern "C" {
struct AVFormatContext;
struct AVIOContext;
}

#include "AudioInfo.h"
#include "DataStream.h"

bool FfmpegOpenStreamInput(CDataStream* stream, AVFormatContext*& fmtCtx, AVIOContext*& ioCtx, uint8_t*& ioBuffer);
uint32_t StreamFormatFromFfmpeg(const char* inputFmtName, const char* filenameUtf8, int audioCodecId);
uint32_t ParseStreamFormatByFfmpegFile(const char* filenameUtf8);
uint32_t ParseStreamFormatByFfmpegStream(CDataStream* pStream);
AudioDataFormat AudioDataFormatFromFfmpegCodec(int codecId);
const char* FfmpegFormatName(uint32_t streamFmt);
