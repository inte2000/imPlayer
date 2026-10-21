#ifndef ENCODER_FORMAT_DEFINE_H
#define ENCODER_FORMAT_DEFINE_H

#include <cstdint>
#include <string>
#include "EncoderValue.h"

constexpr uint32_t EncoderFormatDescLimit = 256;
constexpr uint32_t EncoderFormatExtnameLimit = 64;

// Shared by native encoders and encoder plugins.
#pragma pack(push, 4)
struct EncoderFormatDefine
{
    uint32_t streamFmt;
    char desc[EncoderFormatDescLimit];
    char extname[EncoderFormatExtnameLimit];

    EncoderFormatDefine() = default;
    EncoderFormatDefine(uint32_t format, const std::string& description, const std::string& extensions)
        : streamFmt(format), desc{}, extname{}
    {
        CopyEncoderString(desc, description);
        CopyEncoderString(extname, extensions);
    }
};
#pragma pack(pop)

#endif //ENCODER_FORMAT_DEFINE_H
