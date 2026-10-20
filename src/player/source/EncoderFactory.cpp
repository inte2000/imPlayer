#include <algorithm>
#include <format>
#include <stdexcept>
#include <utility>

#include "EncoderFactory.h"
#include "WavEncoder.h"
#include "PluginEncoder.h"
#include "UnicodeConvert.h"

namespace {

static const EncoderItem s_nativeEncoders[] =
{
    {
        CWavEncoder::GetName(),
        "imPlayer group",
        ENCODE_TYPE_NATIVE,
        [](uint32_t streamFmt) { return std::make_unique<CWavEncoder>(streamFmt); },
        []() { return CWavEncoder::GetFormatDefine(); },
        [](uint32_t) { return CWavEncoder::GetParameterDefine(); },
        ""
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
    const auto it = std::find_if(m_encoderItems.begin(), m_encoderItems.end(), [&name](const EncoderItem& item) {
        return item.name == name;
    });
    if (it == m_encoderItems.end()) {
        return std::nullopt;
    }

    return *it;
}

std::optional<EncoderItem> CEncoderFactory::GetEncoderPlugin(const std::string& name) const
{
    auto item = GetEncoderItem(name);
    return item && item->type == ENCODE_TYPE_PLUGIN ? item : std::nullopt;
}

std::vector<EncoderItem> CEncoderFactory::GetEncoders() const
{
    return m_encoderItems;
}

std::unique_ptr<CAudioEncoder> CEncoderFactory::MakeAudioEncoder(uint32_t streamFmt, const std::string& name) const
{
    for (const auto& item : m_encoderItems) {
        if (!name.empty() && item.name != name) {
            continue;
        }
        const auto formats = item.QueryFormats();
        if (std::any_of(formats.begin(), formats.end(), [streamFmt](const EncoderFormatDefine& format) {
            return format.streamFmt == streamFmt;
        })) {
            return item.Creator(streamFmt);
        }
    }
    return nullptr;
}

void CEncoderFactory::SetAppVersion(uint32_t major, uint32_t minor)
{
    m_verMajor = major;
    m_verMinor = minor;
}

std::tuple<bool, std::string> CEncoderFactory::AddPluginObject(const PluginDllObject& dllObj)
{
    if (dllObj.type != "Encoder" || !dllObj.encoderDll || !static_cast<bool>(*dllObj.encoderDll)) {
        return { false, "Invalid encoder plugin object." };
    }
    if (dllObj.name.empty() || GetEncoderItem(dllObj.name)) {
        return { false, std::format("encoder name is empty or already registered: {}", dllObj.name) };
    }

    auto dll = dllObj.encoderDll;
    PluginInfo info{};
    info.size = sizeof(info);
    if (dll->GetPluginInformation(&info) != 0 || info.plug_type != PluginType::Encoder) {
        return { false, "Plugin is not an encoder." };
    }
    ApplicationConfig app{};
    app.major_ver = m_verMajor;
    app.minor_ver = m_verMinor;
    app.FileTypeIdBegin = m_availableStreamFormatID;
    PluginRegister registration{};
    registration.size = sizeof(registration);
    if (dll->OnRegister(&app, &registration) != 0) {
        return { false, std::format("register plugin encoder failed: {}: {}", dllObj.name, dll->GetErrorMessage()) };
    }
    PluginEncodingFormatsDefine formats{};
    formats.size = sizeof(formats);
    if (dll->GetEncodingFormatsDefine(&formats) != 0) {
        return { false, std::format("query plugin encoder formats failed: {}", dll->GetErrorMessage()) };
    }
    if (formats.fmtDef.empty() || formats.fmtDef.size() != registration.format_count
        || registration.format_count > PLUG_FORMAT_MAX_LIMIT) {
        return { false, "Invalid encoder plugin format count." };
    }
    std::vector<uint32_t> formatIds;
    for (const auto& format : formats.fmtDef) {
        if (format.streamFmt == StreamFormatUnknown || format.extname.empty()
            || std::find(formatIds.begin(), formatIds.end(), format.streamFmt) != formatIds.end()
            || std::none_of(registration.fmt_reg, registration.fmt_reg + registration.format_count,
                [&format](const FileFormatReg& registered) { return registered.id == format.streamFmt; })) {
            return { false, "Invalid or inconsistent encoder plugin format definition." };
        }
        formatIds.push_back(format.streamFmt);
    }

    EncoderItem item;
    item.name = dllObj.name;
    item.publisher = dllObj.publisher;
    item.type = ENCODE_TYPE_PLUGIN;
    item.hostfile = Utf16ToUtf8(dll->GetDllHostName());
    item.Creator = [dll](uint32_t streamFmt) -> std::unique_ptr<CAudioEncoder> {
        auto encoder = std::make_unique<CPluginEncoder>(streamFmt);
        encoder->AttachModule(dll);
        return encoder;
    };
    item.QueryFormats = [definitions = std::move(formats.fmtDef)] { return definitions; };
    item.QueryParamters = [dll, formatIds = std::move(formatIds)](uint32_t streamFmt) {
        if (std::find(formatIds.begin(), formatIds.end(), streamFmt) == formatIds.end()) {
            throw std::runtime_error("Encoder plugin does not support the requested format.");
        }
        PluginEncodingParamtersDefine parameters{};
        parameters.size = sizeof(parameters);
        if (dll->GetEncodingParamtersDefine(streamFmt, &parameters) != 0) {
            throw std::runtime_error("Query plugin encoder parameters failed: " + dll->GetErrorMessage());
        }
        return parameters.paramDef;
    };
    if (!AddEncoderItem(item)) {
        return { false, "Unable to register encoder plugin." };
    }
    m_availableStreamFormatID += PLUG_FORMAT_MAX_LIMIT;
    return { true, "" };
}

std::tuple<bool, std::string> CEncoderFactory::RemovePluginObject(const std::string& name)
{
    const auto item = GetEncoderItem(name);
    if (!item) {
        return { false, std::format("encoder not registered: {}", name) };
    }
    if (item->type != ENCODE_TYPE_PLUGIN) {
        return { false, std::format("encoder is not a plugin: {}", name) };
    }
    RemoveEncoderItem(name);
    return { true, "" };
}

bool CEncoderFactory::AddEncoderItem(const EncoderItem& item)
{
    if (item.name.empty()) {
        return false;
    }
    if (!item.Creator || !item.QueryFormats || !item.QueryParamters) {
        return false;
    }

    if (GetEncoderItem(item.name)) {
        return false;
    }
    m_encoderItems.push_back(item);
    return true;
}

bool CEncoderFactory::RemoveEncoderItem(const std::string& name)
{
    return std::erase_if(m_encoderItems, [&name](const EncoderItem& item) { return item.name == name; }) > 0;
}
