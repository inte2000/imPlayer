#ifndef ENCODER_PARAMTER_DEFINE_UTILS_H
#define ENCODER_PARAMTER_DEFINE_UTILS_H

#include <string>
#include <vector>

#include "core/EncodingParams.h"
#include "encoder/EncoderParamterDefine.h"

inline std::vector<EncoderParamter> BuildDefaultEncoderParamters(const std::vector<EncoderParamterDefine>& defines)
{
    std::vector<EncoderParamter> defaults;
    defaults.reserve(defines.size());
    for (const auto& define : defines) {
        defaults.emplace_back(define.GetName(), define.GetType(), define.GetDefaultValue());
    }

    return defaults;
}

inline bool HasEncoderParamterDefine(const std::vector<EncoderParamterDefine>& defines, const std::string& name)
{
    for (const auto& define : defines) {
        if (define.GetName() == name) {
            return true;
        }
    }

    return false;
}

#endif //ENCODER_PARAMTER_DEFINE_UTILS_H
