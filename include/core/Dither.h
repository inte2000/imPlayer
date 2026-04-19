#ifndef DITHER_OPERATIONS_H
#define DITHER_OPERATIONS_H

#include <random>
#include <algorithm>

enum class DitherTypeT
{
    None,           // 无抖动
    Triangle,       // 三角抖动 （Triangular Dithering）
    Gaussian,       //高斯抖动（Gaussian Dithering）
    NoiseShaping1,   // 一阶噪声整形（Noise Shaping）
    NoiseShaping2   // 二阶噪声整形（Noise Shaping）
};

class Dither
{
	std::uniform_real_distribution<float> distribution;
	std::mt19937 rndGen;
	float prev = 0.0f;
    DitherTypeT d;

public:
		
	Dither(DitherTypeT d) : distribution(-0.5f, +0.5f), d(d) {}

   	float operator()(float s)
    {
		if (d == DitherTypeT::Triangle)
		{
			const float value = distribution(rndGen);
			s = s + value - prev;
			prev = value;
			return s;
		}
		else
		{
			return s;
		}
	}
};


class DitherTriangle
{
    std::uniform_real_distribution<float> rpdfDist;
    std::mt19937& rndGen;
    float lsb_value;
public:

    DitherTriangle(std::mt19937& gen) : rndGen(gen), rpdfDist(-0.5f, +0.5f), lsb_value(0.0f) {}
    
    //8 位抖动 scale=128，16 位抖动 scale=32768，24 位抖动 scale=8388608，32 位抖动 scale=2147483648
    void SetScale(float scale) {
        lsb_value = 1.0f / scale;
    }
    // TPDF 抖动 (Triangular Probability Density Function) 三角概率密度函数
    float operator()(float input)
    {
        float dither = (rpdfDist(rndGen) - rpdfDist(rndGen)) * lsb_value;
        return input + dither;
    }
    double operator()(double input)
    {
        double dither = (rpdfDist(rndGen) - rpdfDist(rndGen)) * lsb_value;
        return input + dither;
    }
};

#if 0
class AdvancedDither 
{
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> rpdfDist;
    std::normal_distribution<float> normalDist;
    DitherTypeT ditherType;
    float prevError[2] = {0.0f, 0.0f};
    
public:
    AdvancedDither(DitherTypeT type, float sigma = 0.5f) : ditherType(type), gen(rd()), rpdfDist(-0.5f, 0.5f), normalDist(0.0f, sigma) {}

    //8 位抖动 scale=128，16 位抖动 scale=32768
    // TPDF 抖动 (Triangular Probability Density Function) 三角概率密度函数
    float tpdf(float input, float scale) {
        float dither = (rpdfDist(gen) + rpdfDist(gen)) * (1.0f / scale);
        return input + dither;
    }

    // 高斯抖动
    float gaussian(float input, float scale) {
        float dither = normalDist(gen) * (1.0f / scale);
        return input + dither;
    }
    
    // 噪声整形抖动（一阶）
    float noiseShaped1st(float input, float scale) {
        double shaped = input + 0.5 * prevError[0];
        // 量化和误差计算
        double scaled = shaped * scale;
        int32_t quantized = static_cast<int32_t>(std::round(scaled));
        quantized = std::clamp(quantized, (int32_t)-scale, (int32_t)(scale+1));
        // 计算量化误差（在-1到1范围内）
        //prevError[0] = shaped - (static_cast<double>(quantized) / scale);
        prevError[0] = float(shaped - (static_cast<double>(quantized) / scale));
        return static_cast<int16_t>(quantized);
    }
    
    // 噪声整形抖动（二阶）
    //二阶噪声整形传递函数：H(z) = 1 - 1.5z^ { -1 } + 0.5z^ { -2 }
    float noiseShaped2nd(float input, float scale) {
        float lsb = 1.0f / scale;
        //float ditherVal = dither.tpdf() * lsb;
        float ditherVal = (rpdfDist(gen) + rpdfDist(gen)) / scale;
        // 二阶噪声整形传递函数：H(z) = 1 - 1.5z^{-1} + 0.5z^{-2}
        float output = input + ditherVal - 1.5f * prevError[0] + 0.5f * prevError[1];
        // 更新历史
        prevError[1] = prevError[0];
        prevError[0] = ditherVal;

        return output;
#if 0
        // 应用噪声整形
        input += 0.5f * prevError[0] + 0.25f * prevError[1];
        // 添加抖动
        //input += dither.getTPDF() / scale;
        input += (rpdfDist(gen) + rpdfDist(gen)) / scale;
        input = std::clamp(input, -1.0f, 1.0f);
        // 更新误差状态
        float error = input - (static_cast<float>(sampleInt) / scale);
        prevError[1] = prevError[0];
        prevError[0] = error;
        
        floatData[i] = input;
#endif
    }
    
    // 重置状态
    void reset() {
        prevError[0] = 0.0f;
        prevError[1] = 0.0f;
    }
    
#if 0
    // 适用于特定位深度的抖动
    float getDitherForBitDepth(int bitDepth) {
        float lsb = 1.0f / (1 << (bitDepth - 1));
        switch (bitDepth) {
            case 8:  return tpdf() * lsb;
            case 16: return tpdf() * lsb;
            case 24: return gaussian(0.7f) * lsb;
            case 32: return gaussian(0.3f) * lsb;
            default: return tpdf() * lsb;
        }
    }
#endif
};
#endif

#if 0
// 噪声整形器
class NoiseShaper {
private:
    float history[3] = { 0.0f, 0.0f, 0.0f };
    DitherAlgorithms dither;

public:
    // 一阶噪声整形
    float process1stOrder(float input, int bitDepth) {
        float lsb = 1.0f / (1 << (bitDepth - 1));
        float ditherVal = dither.tpdf() * lsb;

        float output = input + ditherVal - history[0];
        history[0] = ditherVal;

        return output;
    }

    // 二阶噪声整形（更好的高频噪声抑制）
    float process2ndOrder(float input, int bitDepth) {
        float lsb = 1.0f / (1 << (bitDepth - 1));
        float ditherVal = dither.tpdf() * lsb;

        // 二阶噪声整形传递函数：H(z) = 1 - 1.5z^{-1} + 0.5z^{-2}
        float output = input + ditherVal - 1.5f * history[0] + 0.5f * history[1];

        // 更新历史
        history[1] = history[0];
        history[0] = ditherVal;

        return output;
    }

    // 重置状态
    void reset() {
        history[0] = 0.0f;
        history[1] = 0.0f;
        history[2] = 0.0f;
    }
};
#endif


#endif
