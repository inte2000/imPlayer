#ifndef ENCODER_FORMAT_DEFINE_H
#define ENCODER_FORMAT_DEFINE_H

#include <cstdint>
#include <string>

// Shared by native encoders and encoder plugins.
struct EncoderFormatDefine
{
    uint32_t streamFmt;
    std::string desc;
    std::string extname;
};

#endif //ENCODER_FORMAT_DEFINE_H
