#ifndef CPU_ARCH_ENDIAN_H
#define CPU_ARCH_ENDIAN_H

#include <memory>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define CPU_X86 1
#endif

#if defined(__arm__) || defined(_M_ARM)
#define CPU_ARM 1
#endif

#if defined(CPU_X86) && defined(CPU_ARM)
#error CPU_X86 and CPU_ARM both defined.
#endif

#if !defined(ARCH_CPU_BIG_ENDIAN) && !defined(ARCH_CPU_LITTLE_ENDIAN)
#if CPU_X86 || CPU_ARM || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ARCH_CPU_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ARCH_CPU_BIG_ENDIAN
#else
#error ARCH_CPU_BIG_ENDIAN or ARCH_CPU_LITTLE_ENDIAN should be defined.
#endif
#endif

#if defined(ARCH_CPU_BIG_ENDIAN) && defined(ARCH_CPU_LITTLE_ENDIAN)
#error ARCH_CPU_BIG_ENDIAN and ARCH_CPU_LITTLE_ENDIAN both defined.
#endif

static inline uint16_t Swap16(uint16_t value)
{
	return (uint16_t)((value >> 8) | (value << 8));
}

static inline uint32_t Swap24(uint32_t value)
{
	return (((value & 0x00ff0000) >> 16) |
		((value & 0x0000ff00)) |
		((value & 0x000000ff) << 16)) & 0x00FFFFFF;
}

static inline uint32_t Swap32(uint32_t value)
{
	return (((value & 0x000000ff) << 24) |
		((value & 0x0000ff00) << 8) |
		((value & 0x00ff0000) >> 8) |
		((value & 0xff000000) >> 24));
}

static inline uint64_t Swap64(uint64_t value)
{
	return (((value & 0x00000000000000ffLL) << 56) |
		((value & 0x000000000000ff00LL) << 40) |
		((value & 0x0000000000ff0000LL) << 24) |
		((value & 0x00000000ff000000LL) << 8) |
		((value & 0x000000ff00000000LL) >> 8) |
		((value & 0x0000ff0000000000LL) >> 24) |
		((value & 0x00ff000000000000LL) >> 40) |
		((value & 0xff00000000000000LL) >> 56));
}

template <typename T>
inline bool isOdd(const T x)
{
	return (x & 0x1);
}

#ifdef ARCH_CPU_LITTLE_ENDIAN
#define Read16(n) (n)
#define Read24(n) (n)
#define Read32(n) (n)
#define Read64(n) (n)
#define Write16(n) (n)
#define Write24(n) (n)
#define Write32(n) (n)
#define Write64(n) (n)

#define LittleEndian2Bytes(n) (n)
#define LittleEndian4Bytes(n) (n)
#else
#define Read16(n) Swap16(n)
#define Read24(n) Swap24(n)
#define Read32(n) Swap32(n)
#define Read64(n) Swap64(n)
#define Write16(n) Swap16(n)
#define Write24(n) Swap24(n)
#define Write32(n) Swap32(n)
#define Write64(n) Swap64(n)

#define LittleEndian2Bytes(n) Swap16(n)
#define LittleEndian4Bytes(n) Swap32(n)
#endif



#endif //CPU_ARCH_ENDIAN_H
