
#include <utility>
#include <cmath>
#include <cassert>
#include <algorithm>
#include "ConvertFormat.h"
//#include "Dither.h"

bool ConvertFromPcmFloat32(const float* pFromData, void* pToBuf, AudioDataFormat toFmt, uint32_t samples)
{
    //Dither dither(DITHER_NONE);
    if (toFmt == AudioDataFormat::PCM_S8)
    {
        int8_t* pToData = reinterpret_cast<int8_t*>(pToBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = (int8_t)std::lroundf(float32_to_int8(pFromData[i]));

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_U8)
    {
        uint8_t* pToData = reinterpret_cast<uint8_t*>(pToBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = (uint8_t)std::lroundf(float32_to_uint8(pFromData[i]));

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S16)
    {
        int16_t* pToData = reinterpret_cast<int16_t*>(pToBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = (int16_t)std::lroundf(float32_to_int16(pFromData[i]));

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S24)
    {
        uint8_t* pToData = reinterpret_cast<uint8_t*>(pToBuf);
        uint32_t c = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t tmp24 = (int32_t)std::lroundf(float32_to_int24(pFromData[i]));
            auto unpacked = Unpack(tmp24); // Handles endian swap
            pToData[c] = unpacked[0];
            pToData[c + 1] = unpacked[1];
            pToData[c + 2] = unpacked[2];
            c += 3;
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S32)
    {
        int32_t* pToData = reinterpret_cast<int32_t*>(pToBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = (int32_t)std::lroundf(float32_to_int32(pFromData[i]));

        return true;
    }
    else if (toFmt == AudioDataFormat::Float32)
    {
        std::memcpy(pToBuf, pFromData, samples * sizeof(float));
        return true;
    }
    else
    {
        return false;
    }
}

bool ConverToPcmFloat32(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples)
{
    if (fromFmt == AudioDataFormat::PCM_S8)
    {
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = int8_to_float32(pFromData[i]);

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_U8)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = uint8_to_float32(pFromData[i]);

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S16)
    {
        const int16_t* pFromData = reinterpret_cast<const int16_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = int16_to_float32(Read16(pFromData[i]));

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S24)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        uint32_t c = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int24 = Pack(pFromData[c], pFromData[c + 1], pFromData[c + 2]);
            pToData[i] = int24_to_float32(int24); // Packed types don't need addtional endian helpers
            c += 3;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        const int32_t* pFromData = reinterpret_cast<const int32_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = int32_to_float32(Read32(pFromData[i]));

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        std::memcpy(pToData, pFromBuf, samples * sizeof(float));
        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        const double* pFromData = reinterpret_cast<const double*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = (float)Read64(pFromData[i]);

        return true;
    }
    else
    {
        return false;
    }
}

#if 0
// U8->16S 带抖动的转换（减少量化噪声）
static std::vector<int16_t> convertWithDither(const std::vector<uint8_t>& u8Data) {
    std::vector<int16_t> s16Data(u8Data.size());
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(-128, 127); // ±0.5 LSB

    for (size_t i = 0; i < u8Data.size(); ++i) {
        int32_t sample = static_cast<int32_t>(u8Data[i]) - 128;
        sample = sample * 256 + dist(gen); // 添加抖动
        s16Data[i] = static_cast<int16_t>(std::clamp(sample, -32768, 32767));
    }

    return s16Data;
}

// 24S->16S 带噪声整形的转换
static std::vector<int16_t> convertWithNoiseShaping(const std::vector<uint8_t>& s24Data) {
    if (s24Data.size() % 3 != 0) {
        throw std::invalid_argument("24位PCM数据大小必须是3的倍数");
    }

    std::vector<int16_t> s16Data(s24Data.size() / 3);
    double error = 0.0; // 误差累积

    for (size_t i = 0; i < s16Data.size(); ++i) {
        size_t offset = i * 3;

        // 重建24位样本
        int32_t sample24 = (static_cast<int32_t>(s24Data[offset]) << 0) |
            (static_cast<int32_t>(s24Data[offset + 1]) << 8) |
            (static_cast<int32_t>(s24Data[offset + 2]) << 16);

        if (sample24 & 0x00800000) {
            sample24 |= 0xFF000000;
        }

        // 转换为浮点进行精确处理
        double floatSample = static_cast<double>(sample24) / 8388608.0;

        // 应用噪声整形
        floatSample += error;

        // 缩放到16位范围
        double scaled = floatSample * 32768.0;

        // 量化和误差计算
        int32_t quantized = static_cast<int32_t>(std::round(scaled));
        quantized = std::clamp(quantized, -32768, 32767);

        error = floatSample - (static_cast<double>(quantized) / 32768.0);

        s16Data[i] = static_cast<int16_t>(quantized);
    }

    return s16Data;
}
#endif

bool ConverToPcmInt16S(const void* pFromBuf, AudioDataFormat fromFmt, int16_t* pToData, uint32_t samples)
{
    if (fromFmt == AudioDataFormat::PCM_S8)
    {
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = static_cast<int16_t>((static_cast<int32_t>(pFromData[i])) * 256);

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_U8)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = static_cast<int16_t>((static_cast<int32_t>(pFromData[i]) - 128) * 256);

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S16)
    {
        std::memcpy(pToData, pFromBuf, samples * sizeof(int16_t));
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S24)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        uint32_t c = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int24 = Pack(pFromData[c], pFromData[c + 1], pFromData[c + 2]);// Packed types don't need addtional endian helpers

            // 24位 [-8388608, 8388607] → 16位 [-32768, 32767]
            // 右移8位相当于除以256
            int32_t sample16 = int24 >> 8;
            pToData[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
            c += 3;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        constexpr double scale = 32768.0 / 2147483648.0;
        const int32_t* pFromData = reinterpret_cast<const int32_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int32 = Read32(pFromData[i]);
#if 0
            // 32位 [-2147483648, 2147483647] → 16位 [-32768, 32767]
            // 右移16位相当于除以65536
            int32_t sample16 = int32 >> 16;
            pToData[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
#endif
            // 使用浮点运算避免精度问题
            double scaled = static_cast<double>(int32) * scale;
            int32_t sample16 = static_cast<int32_t>(std::round(scaled));
            pToData[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        const float* pFromData = reinterpret_cast<const float*>(pFromBuf);
        const uint8_t* floatData = reinterpret_cast<const uint8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromData[i]);

            size_t offset = i * 4;
            // 重建IEEE 754浮点数（小端序）
            uint32_t floatBits = (static_cast<uint32_t>(floatData[offset]) << 0) |
                (static_cast<uint32_t>(floatData[offset + 1]) << 8) |
                (static_cast<uint32_t>(floatData[offset + 2]) << 16) |
                (static_cast<uint32_t>(floatData[offset + 3]) << 24);
            
            // 解释为浮点数
            float sampleFloat;
            std::memcpy(&sampleFloat, &floatBits, sizeof(float));
            assert(float32 == sampleFloat);

            // 限制范围并转换
            float32 = std::max(-1.0f, std::min(1.0f, float32));
            int32_t sample16 = static_cast<int32_t>(float32 * 32768.0f);
            pToData[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        const double* pFromData = reinterpret_cast<const double*>(pFromBuf);
        const uint8_t* doubleData = reinterpret_cast<const uint8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            double float64 = Read64(pFromData[i]);

            size_t offset = i * 8;
            // 重建IEEE 754双精度浮点数（小端序）
            uint64_t doubleBits = 0;
            for (int j = 0; j < 8; ++j) {
                doubleBits |= static_cast<uint64_t>(doubleData[offset + j]) << (j * 8);
            }

            // 解释为双精度浮点数
            double sampleDouble;
            std::memcpy(&sampleDouble, &doubleBits, sizeof(double));
            assert(float64 == sampleDouble);

            // 限制范围并转换
            float64 = std::max(-1.0, std::min(1.0, float64));
            int32_t sample16 = static_cast<int32_t>(float64 * 32768.0);
            pToData[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool ConvertAudioFormat(const void* pFromBuf, AudioDataFormat fromFmt, void* pToBuf, AudioDataFormat toFmt, uint32_t samples)
{
    auto floatPtr = std::make_unique<float[]>(samples);
    
    if (ConverToPcmFloat32(pFromBuf, fromFmt, floatPtr.get(), samples))
    {
        return ConvertFromPcmFloat32(floatPtr.get(), pToBuf, toFmt, samples);
    }

    return false;
}
