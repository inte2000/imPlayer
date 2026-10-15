#pragma once

#include <windows.h>

#include "oplugsdk.h"

extern "C" {
int WINAPI Plug_OnRegister(const ApplicationConfig* app, PluginRegister* regInfo);
void WINAPI Plug_GetErrMessage(char* msgBuf, uint32_t bufSize);
int WINAPI Plug_GetPluginInformation(PluginInfo* info);
int WINAPI Plug_GetEncodingFormatsDefine(PluginEncodingFormatsDefine* fmt);
int WINAPI Plug_GetEncodingParamtersDefine(uint32_t streamFmt, PluginEncodingParamtersDefine* param);
void* WINAPI Plug_OnInitialize(const EncoderPluginInitialize* init);
void WINAPI Plug_OnRelease(void* ctxhdr);
int WINAPI Plug_OpenEncoder(void* ctxhdr, const EncoderPluginOpen* open);
void WINAPI Plug_CloseEncoder(void* ctxhdr, const EncoderPluginClose* close);
int WINAPI Plug_SetMetaInfo(void* ctxhdr, const AudioMetaTags* metaTags);
int WINAPI Plug_StartEncoding(void* ctxhdr, const AudioFormat* audioFmt);
uint32_t WINAPI Plug_EncodeFrames(void* ctx, const void* pBuf, uint32_t frames);
int WINAPI Plug_StopEncoding(void* ctxhdr);
}
