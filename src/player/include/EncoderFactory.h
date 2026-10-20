#ifndef ENCODER_FACTORY_H
#define ENCODER_FACTORY_H

#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "AudioEncoder.h"
#include "EncoderFormatDefine.h"
#include "EncoderParamterDefine.h"
#include "PluginObjects.h"

struct EncoderItem
{
    std::string name;
    std::string publisher;
    uint32_t type = ENCODE_TYPE_UNKNOWN;
    std::function<std::unique_ptr<CAudioEncoder>(uint32_t)> Creator;
    std::function<std::vector<EncoderFormatDefine>()> QueryFormats;
    std::function<std::vector<EncoderParamterDefine>(uint32_t)> QueryParamters;
    std::string hostfile;
};

class CEncoderFactory final
{
public:
    static CEncoderFactory& GetInstance()
    {
        static CEncoderFactory s_instance;
        return s_instance;
    }

    std::optional<EncoderItem> GetEncoderItem(const std::string& name) const;
    std::optional<EncoderItem> GetEncoderPlugin(const std::string& name) const;
    std::vector<EncoderItem> GetEncoders() const;
    std::unique_ptr<CAudioEncoder> MakeAudioEncoder(uint32_t streamFmt, const std::string& name = {}) const;
    void SetAppVersion(uint32_t major, uint32_t minor);
    std::tuple<bool, std::string> AddPluginObject(const PluginDllObject& dllObj);
    std::tuple<bool, std::string> RemovePluginObject(const std::string& name);
    bool AddEncoderItem(const EncoderItem& item);
    bool RemoveEncoderItem(const std::string& name);

private:
    CEncoderFactory();

private:
    // 按注册顺序选择编码器，内置编码器优先于后来加载的插件。
    std::vector<EncoderItem> m_encoderItems;
    uint32_t m_verMajor = 1;
    uint32_t m_verMinor = 0;
    uint32_t m_availableStreamFormatID = StreamFormatPlusBegin;
};

#endif //ENCODER_FACTORY_H
