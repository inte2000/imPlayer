#include <utility>

#include "EncoderFactory.h"
#include "WavEncoder.h"

namespace {

static const EncoderItem s_nativeEncoders[] =
{
    {
        CWavEncoder::GetName(),
        "imPlayer group",
        ENCODE_TYPE_NATIVE,
        [](uint32_t streamFmt) { return std::make_unique<CWavEncoder>(streamFmt); },
        []() { return CWavEncoder::GetFormatDefine(); },
        []() { return CWavEncoder::GetParameterDefine(); }
    }
};

}

CEncoderFactory::CEncoderFactory()
{
    for (const auto& item : s_nativeEncoders) {
        AddEncoderItem(item);
    }
}

std::optional<EncoderItem> CEncoderFactory::GetEncoderItem(const std::string& name) const
{
    auto it = m_encoderItems.find(name);
    if (it == m_encoderItems.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool CEncoderFactory::AddEncoderItem(const EncoderItem& item)
{
    if (item.name.empty()) {
        return false;
    }
    if (!item.Creator || !item.QueryFormats || !item.QueryParamters) {
        return false;
    }

    auto [it, inserted] = m_encoderItems.try_emplace(item.name, item);
    return inserted;
}

bool CEncoderFactory::RemoveEncoderItem(const std::string& name)
{
    return m_encoderItems.erase(name) > 0;
}
