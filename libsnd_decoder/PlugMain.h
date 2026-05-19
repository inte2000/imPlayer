/*
原始函数声明是手工写的，extern “C” 和 WINAPI 声明部分是 AI 修改的。
大模型：GPT 5.3 Codex
*/
#pragma once

#include <cstddef>
#include <windows.h>
#include "iplugsdk.h"


extern "C" {
int WINAPI Plus_OnRegister(const ApplicationConfig* app, PluginRegister* regInfo);
void WINAPI Plus_GetErrMessage(char* msgBuf, uint32_t bufSize);
uint32_t WINAPI Plus_ParseFileTypeID(const char* filename);
int WINAPI Plus_GetPluginInformation(PluginInfo* info);
void* WINAPI Plus_OnInitialize(const PluginInitialize* init);
int WINAPI Plus_StartStream(void* ctxhdr, const PluginStart* param);
int WINAPI Plus_StopStream(void* ctxhdr, uint32_t mediaStreamIdx);
int WINAPI Plus_IsSupportOutput(void* ctxhdr, uint32_t mediaStreamIdx, const AudioFormat* audioFmt);
int WINAPI Plus_IsCanSeeking(void* ctxhdr, uint32_t mediaStreamIdx);
void WINAPI Plus_OnUninitialize(void* ctxhdr);
uint32_t WINAPI Plus_DecodeFrames(void* ctx, void* pBuf, uint32_t frames, const AudioFormat* audioFmt);
void WINAPI Plus_SeekToFrame(void* ctx, std::size_t frames);
int WINAPI Plus_QueryMetaInfo(void* ctxhdr, uint32_t streamIdx, AudioMetaTags* metaTags);
int WINAPI Plus_GetAudioStatusInfo(void* ctxhdr, PluginAudioInfo* info);
void WINAPI Plus_ResetDecoder(void* ctxhdr);
void WINAPI Plus_ConfigPlugin(HWND hWnd);
}
