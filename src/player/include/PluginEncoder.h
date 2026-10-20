#pragma once

#include <memory>

#include "AudioEncoder.h"
#include "EncoderDllWrapper.h"

class CPluginEncoder final : public CAudioEncoder
{
public:
    explicit CPluginEncoder(uint32_t streamFmt);
    ~CPluginEncoder() override;

    void AttachModule(const std::shared_ptr<CEncoderDllWrapper>& dll);
    static std::string Name();

    void Init() override;
    bool Open(const std::vector<EncoderParamter>& params) override;
    AudioFormat GetTransFormat() const override { return m_transFmt; }
    bool SetMetaInfo(const CMediaTag& metaTags) override;
    bool Start(const AudioFormat* audioFmt) override;
    uint32_t Encode(const void* pData, uint32_t frames) override;
    bool Stop() override;
    void Close() override;

private:
    void ReleasePlugin();
    void ThrowPluginError(const char* action, int code) const;

    void* m_pContext = nullptr;
    uint32_t m_streamFmt;
    AudioFormat m_transFmt{};
    std::shared_ptr<CEncoderDllWrapper> m_dllWrapper;
    bool m_opened = false;
    bool m_started = false;
    bool m_stopped = false;
    bool m_stopResult = true;
};
