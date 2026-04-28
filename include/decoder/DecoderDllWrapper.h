/*
本文件内容有 AI 生成，未作手工修改
大模型：GPT 5.3 Codex
任务说明：todo_task_1.txt
*/
#pragma once

#include <memory>
#include <string>
#include <windows.h>

#include "iplugsdk.h"

typedef int (WINAPI* OnRegisterFuncPtr)(const ApplicationConfig* app, PluginRegister* regInfo);
typedef void (WINAPI* GetErrMessageFuncPtr)(char* msgBuf, uint32_t bufSize);
typedef uint32_t (WINAPI* ParseFileTypeIDFuncPtr)(const char* filename);
typedef int (WINAPI* GetPluginInformationFuncPtr)(PluginInfo* info);
typedef void* (WINAPI* OnInitializeFuncPtr)(const PluginInitialize* init);
typedef int (WINAPI* StartStreamFuncPtr)(void* ctx, const PluginStart* param);
typedef int (WINAPI* StopStreamFuncPtr)(void* ctx, uint32_t mediaStreamIdx);
typedef int (WINAPI* IsCanSeekingFuncPtr)(void* ctx, uint32_t mediaStreamIdx);
typedef int (WINAPI* IsSupportOutputFuncPtr)(void* ctx, uint32_t mediaStreamIdx, const AudioFormat* audioFmt);
typedef void (WINAPI* OnUninitializeFuncPtr)(void* ctx);
typedef uint32_t (WINAPI* DecodeFramesFuncPtr)(void* ctx, void* pBuf, uint32_t frames, const AudioFormat* audioFmt);
typedef void (WINAPI* SeekToFrameFuncPtr)(void* ctx, std::size_t frames);
typedef int (WINAPI* QueryMetaInfoFuncPtr)(void* ctx, uint32_t streamIdx, AudioMetaTags* metaTags);
typedef int (WINAPI* GetAudioStatusInfoFuncPtr)(void* ctx, PluginAudioInfo* info);
typedef void (WINAPI* ResetDecoderFuncPtr)(void* ctx);
typedef void (WINAPI* ConfigPluginFuncPtr)(HWND hWnd);

class CDecoderDllWrapper final
{
public:
    CDecoderDllWrapper();
    explicit CDecoderDllWrapper(const std::wstring& dllname);
    CDecoderDllWrapper(const CDecoderDllWrapper&) = delete;
    CDecoderDllWrapper(CDecoderDllWrapper&& ddr) noexcept;
    CDecoderDllWrapper& operator=(const CDecoderDllWrapper&) = delete;
    CDecoderDllWrapper& operator=(CDecoderDllWrapper&& ddr) noexcept;
    ~CDecoderDllWrapper();

    bool Load(const std::wstring& dllname);
    void Unload();

    std::wstring GetDllHostName() const { return m_dllHost; }

    explicit operator bool() const;

    int OnRegister(const ApplicationConfig* app, PluginRegister* regInfo);
    std::string GetErrorMessage();
    uint32_t ParseFileTypeID(const std::wstring& filename);
    int GetPluginInformation(PluginInfo* info);
    void* OnInitialize(const PluginInitialize* init);
    int StartStream(void* ctx, const PluginStart* param);
    int StopStream(void* ctx, uint32_t mediaStreamIdx);
    int IsCanSeeking(void* ctx, uint32_t mediaStreamIdx);
    int IsSupportOutput(void* ctx, uint32_t mediaStreamIdx, const AudioFormat* audioFmt);
    void OnUninitialize(void* ctx);
    uint32_t DecodeFrames(void* ctx, void* pBuf, uint32_t frames, const AudioFormat* audioFmt);
    void SeekToFrame(void* ctx, std::size_t frames);
    int QueryMetaInfo(void* ctx, uint32_t streamIdx, AudioMetaTags* metaTags);
    int GetAudioStatusInfo(void* ctx, PluginAudioInfo* info);
    void ResetDecoder(void* ctx);
    void ConfigPlugin(HWND hWnd);

private:
    void ResetFunctionPointers();

    HMODULE m_hModule;
    std::wstring m_dllHost;
    OnRegisterFuncPtr m_OnRegisterFunc;
    GetErrMessageFuncPtr m_GetErrMessage;
    ParseFileTypeIDFuncPtr m_ParseFileTypeID;
    GetPluginInformationFuncPtr m_GetPluginInformation;
    OnInitializeFuncPtr m_OnInitialize;
    StartStreamFuncPtr m_StartStream;
    StopStreamFuncPtr m_StopStream;
    IsCanSeekingFuncPtr m_IsCanSeeking;
    IsSupportOutputFuncPtr m_IsSupportOutput;
    OnUninitializeFuncPtr m_OnUninitialize;
    DecodeFramesFuncPtr m_DecodeFrames;
    SeekToFrameFuncPtr m_SeekToFrame;
    QueryMetaInfoFuncPtr m_QueryMetaInfo;
    GetAudioStatusInfoFuncPtr m_GetAudioStatusInfo;
    ResetDecoderFuncPtr m_ResetDecoder;
    ConfigPluginFuncPtr m_ConfigPlugin;
};

std::shared_ptr<CDecoderDllWrapper> MakeDecoderWrapper(const std::wstring& dllname);
