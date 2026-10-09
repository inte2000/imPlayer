#ifndef CONVERT_FORMAT_H
#define CONVERT_FORMAT_H

#include <array>
#include <memory>
#include "CpuArchEndian.h"
#include "AudioInfo.h"

#define F_ROUND(x) ((x) > 0 ? (x) + 0.5f : (x) - 0.5f)
#define D_ROUND(x) ((x) > 0 ? (x) + 0.5  : (x) - 0.5)

inline uint64_t Pack(uint32_t a, uint32_t b)
{
	uint64_t tmp = (uint64_t)b << 32 | (uint64_t)a;
#ifdef ARCH_CPU_LITTLE_ENDIAN
	return tmp;
#else
	return Swap64(tmp);
#endif
}

inline uint32_t Pack(uint16_t a, uint16_t b)
{
	uint32_t tmp = (uint32_t)b << 16 | (uint32_t)a;
#ifdef ARCH_CPU_LITTLE_ENDIAN
	return tmp;
#else
	return Swap32(tmp);
#endif
}

inline uint16_t Pack(uint8_t a, uint8_t b)
{
	uint16_t tmp = (uint16_t)b << 8 | (uint16_t)a;
#ifdef ARCH_CPU_LITTLE_ENDIAN
	return tmp;
#else
	return Swap16(tmp);
#endif
}

// http://www.dsprelated.com/showthread/comp.dsp/136689-1.php
inline int32_t Pack(uint8_t a, uint8_t b, uint8_t c)
{
	// uint32_t tmp = ((c & 0x80) ? (0xFF << 24) : 0x00 << 24) | (c << 16) | (b << 8) | (a << 0); // alternate method
	int32_t x = (c << 16) | (b << 8) | (a << 0);
	auto sign_extended = (x) | (!!((x) & 0x800000) * 0xff000000);
#ifdef ARCH_CPU_LITTLE_ENDIAN
	return sign_extended;
#else
	Swap32(sign_extended);
#endif
}

inline std::array<uint8_t, 3> Unpack(uint32_t a)
{
	static std::array<uint8_t, 3> output;

#ifdef ARCH_CPU_LITTLE_ENDIAN
	output[0] = static_cast<uint8_t>(a >> 0);
	output[1] = static_cast<uint8_t>(a >> 8);
	output[2] = static_cast<uint8_t>(a >> 16);
#else
	output[0] = static_cast<uint8_t>(a >> 16);
	output[1] = static_cast<uint8_t>(a >> 8);
	output[2] = static_cast<uint8_t>(a >> 0);
#endif
	return output;
}

// Signed maxes, defined for readabilty/convenience
#define NQR_INT16_MAX 32767.f
#define NQR_INT24_MAX 8388608.f
#define NQR_INT32_MAX 2147483648.f

static const float NQR_BYTE_2_FLT = 1.0f / 127.0f;

#define int8_to_float32(s)  ((float) (s) * NQR_BYTE_2_FLT)
#define uint8_to_float32(s)(((float) (s) - 128) * NQR_BYTE_2_FLT)
#define int16_to_float32(s) ((float) (s) / NQR_INT16_MAX)
#define int24_to_float32(s) ((float) (s) / NQR_INT24_MAX)
#define int32_to_float32(s) ((float) (s) / NQR_INT32_MAX)

#define int8_to_float64(s)  ((double) (s) * NQR_BYTE_2_FLT)
#define uint8_to_float64(s)(((double) (s) - 128) * NQR_BYTE_2_FLT)
#define int16_to_float64(s) ((double) (s) / NQR_INT16_MAX)
#define int24_to_float64(s) ((double) (s) / NQR_INT24_MAX)
#define int32_to_float64(s) ((double) (s) / NQR_INT32_MAX)

#define float32_to_int8(s)  (float) (s * 127.f)
#define float32_to_uint8(s) (float) ((s * 127.f) + 128)
#define float32_to_int16(s) (float) (s * NQR_INT16_MAX)
#define float32_to_int24(s) (float) (s * NQR_INT24_MAX)
#define float32_to_int32(s) (float) (s * NQR_INT32_MAX)

inline float Float32SampleFromInt16(int16_t sample)
{
	return int16_to_float32(Read16(sample));
}

inline float Float32SampleFromInt24(const uint8_t* samples)
{
	int32_t int24 = Pack(samples[0], samples[1], samples[2]);
	return int24_to_float32(int24); // Packed types don't need addtional endian helpers
}

inline float Float32SampleFromInt32(int32_t samples)
{
	return int32_to_float32(Read32(samples));
}

inline float Float32SampleFromDouble(double sample)
{
	return (float)Read64(sample);
}

bool ConvertFromPcmFloat32(const float* pFromData, void* pToBuf, AudioDataFormat toFmt, uint32_t samples);
bool ConverToPcmFloat32(const void* pFromBuf, AudioDataFormat fromFmt, float* pToData, uint32_t samples);
bool ConvertAudioFormat(const void* pFromBuf, AudioDataFormat fromFmt, void* pToBuf, AudioDataFormat toFmt, uint32_t samples);


#endif //CONVERT_FORMAT_H
