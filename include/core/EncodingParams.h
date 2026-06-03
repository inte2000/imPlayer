#ifndef ENCODING_PARAMS_H
#define ENCODING_PARAMS_H

#include <cstdint>
#include <vector>

struct EncoderParamter
{
    uint32_t type = 0;
    std::vector<uint8_t> value;
};

#endif //ENCODING_PARAMS_H
