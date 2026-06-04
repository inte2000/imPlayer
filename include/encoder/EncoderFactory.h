#ifndef ENCODER_FACTORY_H
#define ENCODER_FACTORY_H

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "AudioEncoder.h"
#include "encoder/EncoderFormatDefine.h"
#include "encoder/EncoderParamterDefine.h"

struct EncoderItem
{
    std::string name;
    std::string publisher;
    uint32_t type = ENCODE_TYPE_UNKNOWN;
    std::function<std::unique_ptr<CAudioEncoder>(uint32_t)> Creator;
    std::function<std::vector<EncoderFormatDefine>()> QueryFormats;
    std::function<std::vector<EncoderParamterDefine>()> QueryParamters;
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
    bool AddEncoderItem(const EncoderItem& item);
    bool RemoveEncoderItem(const std::string& name);

private:
    CEncoderFactory();

private:
    std::unordered_map<std::string, EncoderItem> m_encoderItems;
};

#endif //ENCODER_FACTORY_H
