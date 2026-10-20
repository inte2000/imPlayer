#pragma once

#include <memory>
#include <string>
#include <windows.h>

#include "oplugsdk.h"

class CEncoderDllWrapper final
{
public:
    CEncoderDllWrapper() = default;
    explicit CEncoderDllWrapper(const std::wstring& dllname);
    CEncoderDllWrapper(const CEncoderDllWrapper&) = delete;
    CEncoderDllWrapper& operator=(const CEncoderDllWrapper&) = delete;
    CEncoderDllWrapper(CEncoderDllWrapper&& other) noexcept;
    CEncoderDllWrapper& operator=(CEncoderDllWrapper&& other) noexcept;
    ~CEncoderDllWrapper();

    bool Load(const std::wstring& dllname);
    void Unload();
    std::wstring GetDllHostName() const { return m_dllHost; }
    explicit operator bool() const;

    int OnRegister(const ApplicationConfig* app, PluginRegister* regInfo);
    std::string GetErrorMessage();
    int GetPluginInformation(PluginInfo* info);
    int GetEncodingFormatsDefine(PluginEncodingFormatsDefine* fmt);
    int GetEncodingParamtersDefine(uint32_t streamFmt, PluginEncodingParamtersDefine* param);
    void* OnInitialize(const EncoderPluginInitialize* init);
    void OnRelease(void* ctx);
    int OpenEncoder(void* ctx, const EncoderPluginOpen* open);
    void CloseEncoder(void* ctx, const EncoderPluginClose* close);
    int SetMetaInfo(void* ctx, const AudioMetaTags* metaTags);
    int StartEncoding(void* ctx, const AudioFormat* audioFmt);
    uint32_t EncodeFrames(void* ctx, const void* pBuf, uint32_t frames);
    int StopEncoding(void* ctx);

private:
    struct Functions
    {
        int (WINAPI* onRegister)(const ApplicationConfig*, PluginRegister*) = nullptr;
        void (WINAPI* getErrMessage)(char*, uint32_t) = nullptr;
        int (WINAPI* getPluginInformation)(PluginInfo*) = nullptr;
        int (WINAPI* getEncodingFormatsDefine)(PluginEncodingFormatsDefine*) = nullptr;
        int (WINAPI* getEncodingParamtersDefine)(uint32_t, PluginEncodingParamtersDefine*) = nullptr;
        void* (WINAPI* onInitialize)(const EncoderPluginInitialize*) = nullptr;
        void (WINAPI* onRelease)(void*) = nullptr;
        int (WINAPI* openEncoder)(void*, const EncoderPluginOpen*) = nullptr;
        void (WINAPI* closeEncoder)(void*, const EncoderPluginClose*) = nullptr;
        int (WINAPI* setMetaInfo)(void*, const AudioMetaTags*) = nullptr;
        int (WINAPI* startEncoding)(void*, const AudioFormat*) = nullptr;
        uint32_t (WINAPI* encodeFrames)(void*, const void*, uint32_t) = nullptr;
        int (WINAPI* stopEncoding)(void*) = nullptr;
    };

    HMODULE m_hModule = nullptr;
    std::wstring m_dllHost;
    Functions m_functions;
};

std::shared_ptr<CEncoderDllWrapper> MakeEncoderWrapper(const std::wstring& dllname);
