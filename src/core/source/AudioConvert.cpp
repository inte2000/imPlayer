#include <format>
#include <utility>
#include <cmath>
#include <cassert>
#include <algorithm>
#include "ConvertFormat.h"
#include "AudioConvert.h"


bool CAudioConverter::InitConverter()
{
    std::random_device rd;
    m_gen.seed(rd());
    m_accError = 0.0;

    return true;
}

bool CAudioConverter::ToInt16S(const void* pFromBuf, AudioDataFormat fromFmt, int16_t* pToData, uint32_t samples)
{
    if(m_dither == DitherTypeT::None)
        return ToInt16SNoDither(pFromBuf, fromFmt, pToData, samples);
    else
        return ToInt16SNoDither(pFromBuf, fromFmt, pToData, samples);
}

bool CAudioConverter::ToInt24S(const void* pFromBuf, AudioDataFormat fromFmt, uint8_t* pToData, uint32_t samples)
{
    if (m_dither == DitherTypeT::None)
        return ToInt24SNoDither(pFromBuf, fromFmt, pToData, samples);
    else
        return ToInt24SWithDither(pFromBuf, fromFmt, pToData, samples);
}

bool CAudioConverter::ToFloat32(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples)
{
    if (m_dither == DitherTypeT::None)
        return ToFloat32NoDither(pFromBuf, fromFmt, pToData, samples);
    else
        return ToFloat32NoDither(pFromBuf, fromFmt, pToData, samples);
}

bool CAudioConverter::ToInt32S(const void* pFromBuf, AudioDataFormat fromFmt, int32_t* pToData, uint32_t samples)
{
    if (m_dither == DitherTypeT::None)
        return ToInt32SNoDither(pFromBuf, fromFmt, pToData, samples);
    else
        return ToInt32SNoDither(pFromBuf, fromFmt, pToData, samples);
}

bool CAudioConverter::Float32To(const float* pFromBuf, void* pToData, AudioDataFormat toFmt, uint32_t samples)
{
    if (m_dither == DitherTypeT::None)
        return Float32ToNoDither(pFromBuf, pToData, toFmt, samples);
    else
        return Float32ToWithDither(pFromBuf, pToData, toFmt, samples); //
}

bool CAudioConverter::ToInt16SNoDither(const void* pFromBuf, AudioDataFormat fromFmt, int16_t* pToData, uint32_t samples)
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
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
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
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromData[i]);
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
        for (uint32_t i = 0; i < samples; ++i)
        {
            double float64 = Read64(pFromData[i]);
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

bool CAudioConverter::ToInt16SWithDither(const void* pFromBuf, AudioDataFormat fromFmt, int16_t* pToData, uint32_t samples)
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
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
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

            // 转换为浮点进行精确处理
            double floatSample = static_cast<double>(int24) / 8388608.0;
            // 应用噪声整形
            floatSample += m_accError;
            // 缩放到16位范围
            double scaled = floatSample * 32768.0;
            // 量化和误差计算
            int32_t quantized = static_cast<int32_t>(std::round(scaled));
            quantized = std::clamp(quantized, -32768, 32767);
            m_accError = floatSample - (static_cast<double>(quantized) / 32768.0);

            pToData[i] = static_cast<int16_t>(quantized);

            c += 3;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        constexpr double scale = 32768.0 / 2147483648.0;
        //std::uniform_int_distribution<int16_t> dist(-128, 127); // ±0.5 LSB 基本右移+抖动
        std::uniform_real_distribution<double> dist_h(-0.5, 0.5); // 浮点处理+抖动（更精确）
        std::uniform_int_distribution<int16_t> dist_l(-256, 255); // 低电平信号：使用更强的抖动
        constexpr int32_t max32 = 2147483647;
        constexpr int32_t threshold = max32 / 256; // 低电平阈值
        const int32_t* pFromData = reinterpret_cast<const int32_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int32 = Read32(pFromData[i]);
#if 0
            // 基本右移+抖动
            int32_t dithered = (int32 >> 16) + dist(m_gen);
            pToData[i] = static_cast<int16_t>(std::clamp(dithered, -32768, 32767));
#endif
#if 0
            // 浮点处理+抖动（更精确）
            double floatSample = static_cast<double>(int32) * scale;
            // 添加抖动并量化
            double dithered = floatSample * 32768.0 + dist_h(m_gen);
            int32_t quantized = static_cast<int32_t>(std::round(dithered));
            pToData[i] = static_cast<int16_t>(std::clamp(quantized, -32768, 32767));
#endif
#if 1
            //针对低电平信号的优化抖动
            int32_t absSample = std::abs(int32);
            if (absSample < threshold) 
            {
                // 低电平信号：使用更强的抖动
                int32_t dithered = (int32 >> 16) + dist_l(m_gen);
                pToData[i] = static_cast<int16_t>(std::clamp(dithered, -32768, 32767));
            }
            else 
            {
                // 高电平信号：浮点处理+抖动（更精确）
                double floatSample = static_cast<double>(int32) * scale;
                // 添加抖动并量化
                double dithered = floatSample * 32768.0 + dist_h(m_gen);
                int32_t quantized = static_cast<int32_t>(std::round(dithered));
                pToData[i] = static_cast<int16_t>(std::clamp(quantized, -32768, 32767));
            }
#endif
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        const float* pFromData = reinterpret_cast<const float*>(pFromBuf);
        float headroom = 0.0f;
        const float scale = 32768.0f * (1.0f - headroom);
        std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromData[i]);

            // 应用限制
            float32 = std::max(-1.0f, std::min(1.0f, float32));
            // 缩放、抖动和量化
            float scaled = float32 * scale + dist(m_gen);
            int32_t sample16 = static_cast<int32_t>(std::round(scaled));
            pToData[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        const double* pFromData = reinterpret_cast<const double*>(pFromBuf);
        //std::uniform_real_distribution<double> dist(-0.5, 0.5); // ±0.5 LSB 简单一阶整形用
        //std::uniform_int_distribution<int32_t> dist(-32768, 32767); //高性能抖动（使用整数运算）
        for (uint32_t i = 0; i < samples; ++i)
        {
            double float64 = Read64(pFromData[i]);
#if 1
            // 简单的一阶噪声整形
            double sample = std::clamp(float64, -1.0, 1.0);
            // 添加前一个样本的误差反馈
            double shaped = sample + 0.5 * m_accError;
            // 量化和误差计算
            double scaled = shaped * 32768.0;
            int32_t quantized = static_cast<int32_t>(std::round(scaled));
            quantized = std::clamp(quantized, -32768, 32767);
            // 计算量化误差（在-1到1范围内）
            m_accError = shaped - (static_cast<double>(quantized) / 32768.0);
            pToData[i] = static_cast<int16_t>(quantized);
#endif
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::ToInt24SNoDither(const void* pFromBuf, AudioDataFormat fromFmt, uint8_t* pToData, uint32_t samples)
{
    if (fromFmt == AudioDataFormat::PCM_S8)
    {
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 转换为浮点数 [-1.0, 1.0]
            float normalized = static_cast<float>(pFromData[i]) / 128.0f;
            // 限制在 [-1.0, 1.0] 范围内
            normalized = std::max(-1.0f, std::min(1.0f, normalized));
            // 转换为 24 位，pack 在 32 位整数中
            int32_t sample_24bit = static_cast<int32_t>(normalized * 8388607.0f);
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;

            //big_endia
            //pToData[toPos] = (sample_24bit >> 16) & 0xFF;
            //pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            //pToData[toPos + 2] = sample_24bit & 0xFF;
            //toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_U8)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 转换为浮点数 [-1.0, 1.0]
            float normalized = (static_cast<float>(pFromData[i]) - 128.0f) / 128.0f;
            // 限制在 [-1.0, 1.0] 范围内
            normalized = std::max(-1.0f, std::min(1.0f, normalized));
            // 转换为 24 位，pack 在 32 位整数中
            int32_t sample_24bit = static_cast<int32_t>(normalized * 8388607.0f);
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;

            //big_endia
            //pToData[toPos] = (sample_24bit >> 16) & 0xFF;
            //pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            //pToData[toPos + 2] = sample_24bit & 0xFF;
            //toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S16)
    {
        const int16_t* pFromData = reinterpret_cast<const int16_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 16 位转 24 位
            int32_t sample_24bit = ((int32_t)pFromData[i]) << 8;
            //little_endian
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S24)
    {
        std::memcpy(pToData, pFromBuf, samples * 3);
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        constexpr float scale = 8388607.0 / 2147483648.0;
        const int32_t* pFromData = reinterpret_cast<const int32_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 使用浮点运算避免精度问题
            float scaled = static_cast<float>(Read32(pFromData[i])) * scale;
            int32_t sample_24bit = static_cast<int32_t>(std::roundf(scaled));
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endian
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        const float* pFromData = reinterpret_cast<const float*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromData[i]);
            int32_t sample_24bit = static_cast<int32_t>(float32 * 8388607.0f);
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        const double* pFromData = reinterpret_cast<const double*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            double float64 = Read64(pFromData[i]);
            int32_t sample_24bit = static_cast<int32_t>(float64 * 8388607.0f);
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::ToInt24SWithDither(const void* pFromBuf, AudioDataFormat fromFmt, uint8_t* pToData, uint32_t samples)
{
    if (fromFmt == AudioDataFormat::PCM_S8)
    {
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 转换为浮点数 [-1.0, 1.0]
            float normalized = static_cast<float>(pFromData[i]) / 128.0f;
            // 限制在 [-1.0, 1.0] 范围内
            normalized = std::max(-1.0f, std::min(1.0f, normalized));
            // 转换为 24 位，pack 在 32 位整数中
            int32_t sample_24bit = static_cast<int32_t>(normalized * 8388607.0f);
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;

            //big_endia
            //pToData[toPos] = (sample_24bit >> 16) & 0xFF;
            //pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            //pToData[toPos + 2] = sample_24bit & 0xFF;
            //toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_U8)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 转换为浮点数 [-1.0, 1.0]
            float normalized = (static_cast<float>(pFromData[i]) - 128.0f) / 128.0f;
            // 限制在 [-1.0, 1.0] 范围内
            normalized = std::max(-1.0f, std::min(1.0f, normalized));
            // 转换为 24 位，pack 在 32 位整数中
            int32_t sample_24bit = static_cast<int32_t>(normalized * 8388607.0f);
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;

            //big_endia
            //pToData[toPos] = (sample_24bit >> 16) & 0xFF;
            //pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            //pToData[toPos + 2] = sample_24bit & 0xFF;
            //toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S16)
    {
        const int16_t* pFromData = reinterpret_cast<const int16_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 16 位转 24 位
            int32_t sample_24bit = ((int32_t)pFromData[i]) << 8;
            //little_endian
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S24)
    {
        std::memcpy(pToData, pFromBuf, samples * 3);
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        m_tpdf.SetScale(8388607.0); //24 位抖动
        const int32_t* pFromData = reinterpret_cast<const int32_t*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            // 转换为浮点数 [-1.0, 1.0]
            float normalized = static_cast<float>(Read32(pFromData[i])) / 2147483648.0f;
            normalized = m_tpdf(normalized);

            int32_t sample_24bit = static_cast<int32_t>(std::round(normalized * 8388607.0));
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endian
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        m_tpdf.SetScale(8388607.0); //24 位抖动
        const float* pFromData = reinterpret_cast<const float*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromData[i]);
            float32 = m_tpdf(float32);

            int32_t sample_24bit = static_cast<int32_t>(float32 * 8388607.0f);
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        const double* pFromData = reinterpret_cast<const double*>(pFromBuf);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            double float64 = Read64(pFromData[i]);
            float64 = m_tpdf(float64);

            int32_t sample_24bit = static_cast<int32_t>(float64 * 8388607.0f);
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endia
            pToData[toPos] = sample_24bit & 0xFF;
            pToData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            pToData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::ToFloat32NoDither(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples)
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
        {
            double float64 = Read64(pFromData[i]);
            // 64位浮点 → 32位浮点
            float float32 = static_cast<float>(float64);
            // 处理超出float范围的值
            if (std::abs(float64) > std::numeric_limits<float>::max()) {
                float32 = (float)std::copysign(1.0f, float64);
            }

            pToData[i] = std::clamp(float32, -1.0f, 1.0f);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::ToFloat32WithDither(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples)
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
        {
            double float64 = Read64(pFromData[i]);
            // 64位浮点 → 32位浮点
            float float32 = static_cast<float>(float64);
            // 处理超出float范围的值
            if (std::abs(float64) > std::numeric_limits<float>::max()) {
                float32 = (float)std::copysign(1.0f, float64);
            }
            
            pToData[i] = std::clamp(float32, -1.0f, 1.0f);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::ToInt32SNoDither(const void* pFromBuf, AudioDataFormat fromFmt, int32_t* pToData, uint32_t samples)
{
    if (fromFmt == AudioDataFormat::PCM_S8)
    {
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int32s = pFromData[i];
            pToData[i] = int32s << 24;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_U8)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int32s = (int32_t)pFromData[i] - 128;
            pToData[i] = int32s << 24;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S16)
    {
        const int16_t* pFromData = reinterpret_cast<const int16_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int16_t int16s = Read16(pFromData[i]);
            pToData[i] = (int32_t)int16s << 16;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S24)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        uint32_t c = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int24 = Pack(pFromData[c], pFromData[c + 1], pFromData[c + 2]);
            pToData[i] = int24 << 8; 

            c += 3;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        std::memcpy(pToData, pFromBuf, samples * sizeof(int32_t));
        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        const float* pFromData = reinterpret_cast<const float*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromData[i]);
            float32 = std::clamp(float32, -1.0f, 1.0f);
            pToData[i] = (int32_t)(float32 * 2147483647.0f);
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        const double* pFromData = reinterpret_cast<const double*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            double float64 = Read64(pFromData[i]);
            float64 = std::clamp(float64, -1.0, 1.0);
            pToData[i] = (int32_t)(float64 * 2147483647.0);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::ToInt32SWithDither(const void* pFromBuf, AudioDataFormat fromFmt, int32_t* pToData, uint32_t samples)
{
    if (fromFmt == AudioDataFormat::PCM_S8)
    {
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int32s = pFromData[i];
            pToData[i] = int32s << 24;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_U8)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int32s = (int32_t)pFromData[i] - 128;
            pToData[i] = int32s << 24;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S16)
    {
        const int16_t* pFromData = reinterpret_cast<const int16_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            int16_t int16s = Read16(pFromData[i]);
            pToData[i] = (int32_t)int16s << 16;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S24)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        uint32_t c = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int24 = Pack(pFromData[c], pFromData[c + 1], pFromData[c + 2]);
            pToData[i] = int24 << 8;

            c += 3;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        std::memcpy(pToData, pFromBuf, samples * sizeof(int32_t));
        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        const float* pFromData = reinterpret_cast<const float*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromData[i]);
            float32 = std::clamp(float32, -1.0f, 1.0f);
            pToData[i] = (int32_t)(float32 * 2147483647.0f);
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        const double* pFromData = reinterpret_cast<const double*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
        {
            double float64 = Read64(pFromData[i]);
            float64 = std::clamp(float64, -1.0, 1.0);
            pToData[i] = (int32_t)(float64 * 2147483647.0);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::ToFloat64NoDither(const void* pFromBuf, AudioDataFormat fromFmt, double* pToData, uint32_t samples)
{
    if (fromFmt == AudioDataFormat::PCM_S8)
    {
        const int8_t* pFromData = reinterpret_cast<const int8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = int8_to_float64(pFromData[i]);

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_U8)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = uint8_to_float64(pFromData[i]);

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S16)
    {
        const int16_t* pFromData = reinterpret_cast<const int16_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = int16_to_float64(Read16(pFromData[i]));

        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S24)
    {
        const uint8_t* pFromData = reinterpret_cast<const uint8_t*>(pFromBuf);
        uint32_t c = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            int32_t int24 = Pack(pFromData[c], pFromData[c + 1], pFromData[c + 2]);
            pToData[i] = int24_to_float64(int24); // Packed types don't need addtional endian helpers
            c += 3;
        }
        return true;
    }
    else if (fromFmt == AudioDataFormat::PCM_S32)
    {
        const int32_t* pFromData = reinterpret_cast<const int32_t*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = int32_to_float64(Read32(pFromData[i]));

        return true;
    }
    else if (fromFmt == AudioDataFormat::Float32)
    {
        const float* pFromData = reinterpret_cast<const float*>(pFromBuf);
        for (uint32_t i = 0; i < samples; ++i)
            pToData[i] = (double)Read32(pFromData[i]);
        
        return true;
    }
    else if (fromFmt == AudioDataFormat::Float64)
    {
        std::memcpy(pToData, pFromBuf, samples * sizeof(double));

        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::Float32ToWithDither(const float* pFromBuf, void* pToData, AudioDataFormat toFmt, uint32_t samples)
{
    if (toFmt == AudioDataFormat::PCM_S8)
    {
        m_tpdf.SetScale(128.0f);
        int8_t* int8Data = reinterpret_cast<int8_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float scaled = m_tpdf(pFromBuf[i]) * 127.0f;
            int32_t sample8 = static_cast<int32_t>(scaled);
            int8Data[i] = static_cast<int8_t>(std::clamp(sample8, -128, 127));
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_U8)
    {
        m_tpdf.SetScale(256.0f);
        uint8_t* uint8Data = reinterpret_cast<uint8_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float scaled = m_tpdf(pFromBuf[i]) * 255;
            int32_t sample8 = static_cast<int32_t>(scaled);
            uint8Data[i] = static_cast<uint8_t>(std::clamp(sample8, 0, 255));
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S16)
    {
        m_tpdf.SetScale(32768.0f);
        int16_t* int16Data = reinterpret_cast<int16_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = m_tpdf(Read32(pFromBuf[i]));
            // 限制范围并转换
            int32_t sample16 = static_cast<int32_t>(float32 * 32767.0f);
            int16Data[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S24)
    {
        m_tpdf.SetScale(8388608.0f);
        uint8_t* outData = reinterpret_cast<uint8_t*>(pToData);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = m_tpdf(Read32(pFromBuf[i]));
            int32_t sample_24bit = static_cast<int32_t>(float32 * 8388607.0f);
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endia
            outData[toPos] = sample_24bit & 0xFF;
            outData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            outData[toPos + 2] = (sample_24bit >> 16) & 0xFF;

            toPos += 3;
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S32)
    {
        m_tpdf.SetScale(2147483648.0f);
        int32_t* outData = reinterpret_cast<int32_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = m_tpdf(Read32(pFromBuf[i]));
            int32_t sample_32bit = static_cast<int32_t>(float32 * 2147483647.0f);
            outData[i] = std::clamp<int32_t>(sample_32bit, -2147483648, 2147483647);
        }

        return true;

    }
    else if (toFmt == AudioDataFormat::Float32)
    {
        std::memcpy(pToData, pFromBuf, samples * sizeof(float));
        return true;
    }
    else if (toFmt == AudioDataFormat::Float64)
    {
        double* outData = reinterpret_cast<double*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            outData[i] = Read32(pFromBuf[i]);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CAudioConverter::Float32ToNoDither(const float* pFromBuf, void* pToData, AudioDataFormat toFmt, uint32_t samples)
{
    if (toFmt == AudioDataFormat::PCM_S8)
    {
        int8_t* int8Data = reinterpret_cast<int8_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float scaled = pFromBuf[i] * 127.0f;
            int32_t sample8 = static_cast<int32_t>(scaled);
            int8Data[i] = static_cast<int8_t>(std::clamp(sample8, -128, 127));
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_U8)
    {
        uint8_t* uint8Data = reinterpret_cast<uint8_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float scaled = pFromBuf[i] * 255;
            int32_t sample8 = static_cast<int32_t>(scaled);
            uint8Data[i] = static_cast<uint8_t>(std::clamp(sample8, 0, 255));
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S16)
    {
        int16_t* int16Data = reinterpret_cast<int16_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromBuf[i]);
            // 限制范围并转换
            int32_t sample16 = static_cast<int32_t>(float32 * 32767.0f);
            int16Data[i] = static_cast<int16_t>(std::clamp(sample16, -32768, 32767));
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S24)
    {
        uint8_t* outData = reinterpret_cast<uint8_t*>(pToData);
        uint32_t toPos = 0;
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromBuf[i]);
            int32_t sample_24bit = static_cast<int32_t>(float32 * 8388607.0f);
            sample_24bit = static_cast<int16_t>(std::clamp(sample_24bit, -8388608, 8388607));
            //little_endia
            outData[toPos] = sample_24bit & 0xFF;
            outData[toPos + 1] = (sample_24bit >> 8) & 0xFF;
            outData[toPos + 2] = (sample_24bit >> 16) & 0xFF;
            toPos += 3;
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::PCM_S32)
    {
        int32_t* outData = reinterpret_cast<int32_t*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            float float32 = Read32(pFromBuf[i]);
            int32_t sample_32bit = static_cast<int32_t>(float32 * 2147483647.0f);
            outData[i] = std::clamp<int32_t>(sample_32bit, -2147483648, 2147483647);
        }

        return true;
    }
    else if (toFmt == AudioDataFormat::Float32)
    {
        std::memcpy(pToData, pFromBuf, samples * sizeof(float));
        return true;
    }
    else if (toFmt == AudioDataFormat::Float64)
    {
        double* outData = reinterpret_cast<double*>(pToData);
        for (uint32_t i = 0; i < samples; ++i)
        {
            outData[i] = Read32(pFromBuf[i]);
        }

        return true;
    }
    else
    {
        return false;
    }
}

std::unique_ptr<CAudioConverter> MakeAudioConvert(DitherTypeT dither)
{
    auto pConv = std::make_unique<CAudioConverter>(dither);
    if (pConv)
    {
        if (pConv->InitConverter())
            return pConv;
    }

    return nullptr;
}

#if 0
4. SIMD 优化版本（使用 SSE / AVX）
c
#include <stdint.h>
#include <stdlib.h>
#include <immintrin.h>

#ifdef __SSE2__
void convert_16bit_to_24bit_sse2(const int16_t * input, int32_t * output, size_t samples) {
    size_t i = 0;

    // 每次处理 8 个样本
    for (; i + 7 < samples; i += 8) {
        // 加载 8 个 16 位样本
        __m128i v_input = _mm_loadu_si128((const __m128i*)(input + i));

        // 将 16 位扩展为 32 位（有符号）
        __m128i v_low = _mm_srai_epi32(_mm_unpacklo_epi16(v_input, v_input), 16);
        __m128i v_high = _mm_srai_epi32(_mm_unpackhi_epi16(v_input, v_input), 16);

        // 左移 8 位（相当于乘以 256）
        v_low = _mm_slli_epi32(v_low, 8);
        v_high = _mm_slli_epi32(v_high, 8);

        // 存储结果
        _mm_storeu_si128((__m128i*)(output + i), v_low);
        _mm_storeu_si128((__m128i*)(output + i + 4), v_high);
    }

    // 处理剩余样本
    for (; i < samples; i++) {
        output[i] = ((int32_t)input[i]) << 8;
    }
}
#endif

#ifdef __AVX2__
void convert_16bit_to_24bit_avx2(const int16_t* input, int32_t* output, size_t samples) {
    size_t i = 0;

    // 每次处理 16 个样本
    for (; i + 15 < samples; i += 16) {
        // 加载 16 个 16 位样本
        __m256i v_input = _mm256_loadu_si256((const __m256i*)(input + i));

        // 将 16 位扩展为 32 位
        __m256i v_low = _mm256_srai_epi32(_mm256_unpacklo_epi16(v_input, v_input), 16);
        __m256i v_high = _mm256_srai_epi32(_mm256_unpackhi_epi16(v_input, v_input), 16);

        // 左移 8 位
        v_low = _mm256_slli_epi32(v_low, 8);
        v_high = _mm256_slli_epi32(v_high, 8);

        // 存储结果
        _mm256_storeu_si256((__m256i*)(output + i), v_low);
        _mm256_storeu_si256((__m256i*)(output + i + 8), v_high);
    }

    // 处理剩余样本
    for (; i < samples; i++) {
        output[i] = ((int32_t)input[i]) << 8;
    }
}
#endif
#endif