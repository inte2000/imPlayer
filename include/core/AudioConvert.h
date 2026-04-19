#ifndef AUDIO_CONVERTER_H
#define AUDIO_CONVERTER_H

#include <string>
#include <random>
#include "AudioInfo.h"
#include "Dither.h"


class CAudioConverter final
{
    friend std::unique_ptr<CAudioConverter> MakeAudioConvert(DitherTypeT dither);
public:
    CAudioConverter(DitherTypeT dither = DitherTypeT::None) : m_dither(dither), m_tpdf(m_gen) {}
    bool ToInt16S(const void* pFromBuf, AudioDataFormat fromFmt, int16_t* pToData, uint32_t samples);
    bool ToInt24S(const void* pFromBuf, AudioDataFormat fromFmt, uint8_t* pToData, uint32_t samples);
    bool ToFloat32(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples);
    bool ToInt32S(const void* pFromBuf, AudioDataFormat fromFmt, int32_t* pToData, uint32_t samples);
    bool ToFloat64(const void* pFromBuf, AudioDataFormat fromFmt, double* pToData, uint32_t samples) {
        //不需要增加抖动
        return ToFloat64NoDither(pFromBuf, fromFmt, pToData, samples);
    }
    bool Float32To(const float* pFromBuf, void* pToData, AudioDataFormat toFmt, uint32_t samples);
protected:
    bool ToInt16SNoDither(const void* pFromBuf, AudioDataFormat fromFmt, int16_t* pToData, uint32_t samples);
    bool ToInt16SWithDither(const void* pFromBuf, AudioDataFormat fromFmt, int16_t* pToData, uint32_t samples);
    bool ToInt24SNoDither(const void* pFromBuf, AudioDataFormat fromFmt, uint8_t* pToData, uint32_t samples);
    bool ToInt24SWithDither(const void* pFromBuf, AudioDataFormat fromFmt, uint8_t* pToData, uint32_t samples);
    bool ToFloat32NoDither(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples);
    bool ToFloat32WithDither(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples);
    bool ToInt32SNoDither(const void* pFromBuf, AudioDataFormat fromFmt, int32_t* pToData, uint32_t samples);
    bool ToInt32SWithDither(const void* pFromBuf, AudioDataFormat fromFmt, int32_t* pToData, uint32_t samples);
    bool ToFloat64NoDither(const void* pFromBuf, AudioDataFormat fromFmt, double* pToData, uint32_t samples);

    bool Float32ToWithDither(const float* pFromBuf, void* pToData, AudioDataFormat toFmt, uint32_t samples);
    bool Float32ToNoDither(const float* pFromBuf, void* pToData, AudioDataFormat toFmt, uint32_t samples);

    bool InitConverter();
    
    DitherTypeT m_dither;
    double m_accError; // 误差累积
    DitherTriangle m_tpdf;
    std::mt19937 m_gen;
};

std::unique_ptr<CAudioConverter> MakeAudioConvert(DitherTypeT dither = DitherTypeT::None);


#endif //AUDIO_CONVERTER_H
