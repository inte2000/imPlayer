#ifndef RIFF_UTILS_H
#define RIFF_UTILS_H

#include <string>

#pragma pack(1)

struct RiffHeader
{
	char riff[4];        // "RIFF"
	uint32_t fileSize;
	char wave[4];
};

struct RiffChunkHeader
{
	char chunkId[4]; 
	uint32_t size;
};

struct WavHeader {
    char riff[4];        // "RIFF"
    uint32_t fileSize;   // 文件总大小
    char wave[4];        // "WAVE"
    char fmt[4];         // "fmt "
    uint32_t fmtSize;    // fmt 块大小
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign; //frame_size
    uint16_t bitsPerSample;
};

// Chunk ID: 'bext'
struct BextChunk
{
	uint8_t description[256];   // Description of the sound (ascii)
	uint8_t origin[32];         // Name of the originator (ascii)
	uint8_t origin_ref[32];     // Reference of the originator (ascii)
	uint8_t orgin_date[10];     // yyyy-mm-dd (ascii)
	uint8_t origin_time[8];     // hh-mm-ss (ascii)
	uint64_t time_ref;          // First sample count since midnight
	uint32_t version;           // Version of the BWF
	uint8_t uimd[64];           // Byte 0 of SMPTE UMID
	uint8_t reserved[188];      // 190 bytes, reserved for future use & set to NULL
};

// Chunk ID: 'fact'
struct FactChunk
{
	uint32_t sample_length;     // number of samples per channel
};

struct ExtensibleData
{
	uint16_t size;
	uint16_t valid_bits_per_sample;
	uint32_t channel_mask;
	struct GUID
	{
		uint32_t data0;
		uint16_t data1;
		uint16_t data2;
		uint16_t data3;
		uint8_t data4[6];
	} subFormat;
};

#pragma pack()

#endif //RIFF_UTILS_H
