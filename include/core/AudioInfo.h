#pragma once

#include <string>

const uint32_t AUDIO_BUFFER_FRAMES = 4096;  //44100 / 4

const uint32_t StreamFormatUnknown = 0;
const uint32_t StreamFormatAa = 1;
const uint32_t StreamFormatAac = 2;
const uint32_t StreamFormatAc3_4 = 3;
const uint32_t StreamFormatAc4 = 4;
const uint32_t StreamFormatAmr = 5;
const uint32_t StreamFormatApe = 6;
const uint32_t StreamFormatAsf = 7;
const uint32_t StreamFormatAu = 8;
const uint32_t StreamFormatDts = 9;  //dts
const uint32_t StreamFormatFlac = 10; 
const uint32_t StreamFormatMp3 = 11;
const uint32_t StreamFormatOgg = 12;
const uint32_t StreamFormatVoc = 13;
const uint32_t StreamFormatWav = 14;
const uint32_t StreamFormatWv = 15;
const uint32_t StreamFormatWma = 16;
const uint32_t StreamFormatDtsHd = 17; 
const uint32_t StreamFormatM4a = 18; 
const uint32_t StreamFormatAiff = 19; 
const uint32_t StreamFormatCaf = 20; 
const uint32_t StreamFormatMov = 21; 
const uint32_t StreamFormat3gp = 22;

const uint32_t StreamFormatRaw = 23;
const uint32_t StreamFormatWve = 24;
const uint32_t StreamFormatWav64 = 25;
const uint32_t StreamFormatMidSds = 26; //midi sds
const uint32_t StreamFormatPaf = 27;
const uint32_t StreamFormatSvx = 28;
const uint32_t StreamFormatNist = 29;
const uint32_t StreamFormatIrcam = 30;
const uint32_t StreamFormatMat = 31;
//const uint32_t StreamFormatMat5 = 32;
const uint32_t StreamFormatPvf = 33;
const uint32_t StreamFormatXi = 34;
const uint32_t StreamFormatHtk = 35;
const uint32_t StreamFormatAvr = 36;
const uint32_t StreamFormatSd2 = 37;
const uint32_t StreamFormatMPC2k = 38;
const uint32_t StreamFormatW64 = 39;
const uint32_t StreamFormatWavEx = 40;

const uint32_t StreamFormatMp2 = 41;
const uint32_t StreamFormatMp1 = 42;
const uint32_t StreamFormatMidi = 43; //*.mid, *.smf
const uint32_t StreamFormatRmi = 44; //*.rmi
const uint32_t StreamFormatCmf = 45;
const uint32_t StreamFormatMidiRaw = 46; //*.raw midi
const uint32_t StreamFormatCDA = 47;  //CDA file
const uint32_t StreamFormatCUE = 48;  //Cue tracks
const uint32_t StreamFormatDsf = 49;  //DSD audio
const uint32_t StreamFormatDff = 50;  //DSDIFF audio
const uint32_t StreamFormatDst = 51;  //DST audio

//Adplug music format
const uint32_t StreamFormatHsc = 53; 
const uint32_t StreamFormatSng = 54; 
const uint32_t StreamFormatImfWlf = 55; 
const uint32_t StreamFormatA2m = 56; 
const uint32_t StreamFormatA2mA2t = 57;
const uint32_t StreamFormatATSng = 58; //Adlib Tracker
const uint32_t StreamFormatAmd = 59; 
const uint32_t StreamFormatXms = 60; 
const uint32_t StreamFormatBam = 61; 
const uint32_t StreamFormatCoktelAdl = 62;
const uint32_t StreamFormatD00 = 63; 
const uint32_t StreamFormatDfm = 64; 
const uint32_t StreamFormatHsp = 65; 
const uint32_t StreamFormatKsm = 66; 
const uint32_t StreamFormatMad = 67; 
const uint32_t StreamFormatMusMdyIms = 68;
const uint32_t StreamFormatMdi = 69;
//const uint32_t StreamFormatMidSciLaa = 70;
const uint32_t StreamFormatMidSciLaa = StreamFormatMidi;
const uint32_t StreamFormatMkj = 71; 
const uint32_t StreamFormatCff = 72; 
const uint32_t StreamFormatDmo = 73; 
const uint32_t StreamFormatS3m = 74; 
const uint32_t StreamFormatDtm = 75; 
const uint32_t StreamFormatFmSng = 76; //Faust Music Creator
const uint32_t StreamFormatMtk = 77; 
const uint32_t StreamFormatMtr = 78;
const uint32_t StreamFormatRad = 79; 
const uint32_t StreamFormatRac = 80;
const uint32_t StreamFormatSatSa2 = 81; 
const uint32_t StreamFormatXadBmf = 82; 
const uint32_t StreamFormatXadFlash = 83; //Flash
const uint32_t StreamFormatXadHybrid = 84; //Hybrid
const uint32_t StreamFormatXadHyp = 85; //Hypnosis
const uint32_t StreamFormatXadPsi = 86; //PSI
const uint32_t StreamFormatXadRat = 87; //rat
const uint32_t StreamFormatLds = 88; 
const uint32_t StreamFormatPlx = 89;
const uint32_t StreamFormatM = 90; 
const uint32_t StreamFormatRol = 91; 
const uint32_t StreamFormatXsm = 92; 
const uint32_t StreamFormatDro = 93; 
const uint32_t StreamFormatDro2 = 94;
const uint32_t StreamFormatPis = 95;
const uint32_t StreamFormatMsc = 96; 
const uint32_t StreamFormatRixMkf = 97; 
const uint32_t StreamFormatWestAdl = 98; //Westwood ADL
const uint32_t StreamFormatJbm = 99;
const uint32_t StreamFormatGot = 100;
const uint32_t StreamFormatOperaCmf = 101;
const uint32_t StreamFormatVgmVgz = 102; 
const uint32_t StreamFormatSop = 103;
const uint32_t StreamFormatHsqSqxSdbAgdHa2 = 104;
const uint32_t StreamFormatRa = 105;
const uint32_t StreamFormatRm = 106;
const uint32_t StreamFormatCDT = 107;
const uint32_t StreamFormatMpc = 108;
const uint32_t StreamFormatTHD = 109;
const uint32_t StreamFormatTsM2ts = 110;
const uint32_t StreamFormatTta = 111;
const uint32_t StreamFormatKar = 112; //Karaoke midi
const uint32_t StreamFormatAce = 113; //tri-Ace Audio Container
const uint32_t StreamFormatAcm = 114; //Interplay ACM
const uint32_t StreamFormatMp4 = 115; //
const uint32_t StreamFormatWmv = 116; //

const uint32_t StreamFormatPlusBegin = 1024;

enum class AudioDataFormat
{
	UNKNOWN = 0x00,
	//交错格式
	PCM_U8 = 0x01,
	PCM_S8 = 0x02,
	PCM_S16 = 0x03,
	PCM_S24 = 0x04,
	PCM_S24_32 = 0x05,
	PCM_S32 = 0x06,
	PCM_64 = 0x07,
	Float32 = 0x08,
	Float64 = 0x09,
	//平面格式
	PCM_U8P = 0x0A,
	PCM_S8P = 0x0B,
	PCM_S16P = 0x0C,
	PCM_S24P = 0x0D,
	PCM_S24_32P = 0x0E,
	PCM_S32P = 0x0F,
	PCM_64P = 0x10,
	Float32P = 0x11,
	Float64P = 0x12,

	Ulaw = 0x13,
	Alaw = 0x14,
	ImaAdpcm = 0x15,
	MsAdpcm = 0x16,
	Gsm610 = 0x17,
	G721Adpcm = 0x18,
	G723Adpcm_24 = 0x19,
	G723Adpcm_40 = 0x1A,
	MpegLayer3 = 0x1B,
	Alac16 = 0x1C,
	Alac20 = 0x1D,
	Alac24 = 0x1E,
	Alac32 = 0x1F,
	Dpcm_8 = 0x20,
	Dpcm_16 = 0x21,

	Vorbis = 0x22,
	Opus = 0x23,

	Native = 0x24
};

// Speaker Positions for dwChannelMask in WAVEFORMATEXTENSIBLE:
#define CHANNEL_FRONT_LEFT              0x1
#define CHANNEL_FRONT_RIGHT             0x2
#define CHANNEL_FRONT_CENTER            0x4
#define CHANNEL_LOW_FREQUENCY           0x8
#define CHANNEL_BACK_LEFT               0x10
#define CHANNEL_BACK_RIGHT              0x20
#define CHANNEL_FRONT_LEFT_OF_CENTER    0x40
#define CHANNEL_FRONT_RIGHT_OF_CENTER   0x80
#define CHANNEL_BACK_CENTER             0x100
#define CHANNEL_SIDE_LEFT               0x200
#define CHANNEL_SIDE_RIGHT              0x400
#define CHANNEL_TOP_CENTER              0x800
#define CHANNEL_TOP_FRONT_LEFT          0x1000
#define CHANNEL_TOP_FRONT_CENTER        0x2000
#define CHANNEL_TOP_FRONT_RIGHT         0x4000
#define CHANNEL_TOP_BACK_LEFT           0x8000
#define CHANNEL_TOP_BACK_CENTER         0x10000
#define CHANNEL_TOP_BACK_RIGHT          0x20000

struct AudioFormat
{
	AudioDataFormat format;// = AudioDataFormat::UNKNOWN;
	uint32_t numChannels;
	uint32_t chLayout;
	uint32_t sampleRate;
	uint32_t bitsPerSample;
	//uint32_t bytesPerSample;
	uint32_t blockAlign;
};

typedef struct tagTypeNameFormat
{
	const char *typeName;
	uint32_t typeFmt;
	const char* extName;
}TypeNameFormat;

uint32_t GetBitsPerSampleByFormat(AudioDataFormat format);
std::string StringFromAudioFormat(AudioDataFormat wavAudioFormat);
std::string PcmDescriptionFromFormat(AudioDataFormat wavAudioFormat);
std::string SampleRateBrifStr(uint32_t rate);
std::string AudioFormatBrifStr(const AudioFormat* fmt);
uint32_t StandLayoutByChannelsCount(uint32_t channels);
uint32_t SACDLayoutByChannelsCount(uint32_t channels);
std::string ChannelNamesFromLayout(uint32_t chLayout);
std::string ChannelBrifName(uint32_t channels, uint32_t chLayout);
AudioDataFormat AudioFormatFromString(const std::string& fmtStr, uint32_t bitsPerSample);
AudioDataFormat AudioFormatByBitsPerSample(uint32_t bitsPerSample);
bool IsSameAudioFormat(const AudioFormat* fmt1, const AudioFormat* fmt2);
void InitAudioFormat(AudioFormat* fmt, AudioDataFormat dataFmt, uint32_t channels, uint32_t sampleRate, uint32_t bitsPerSample = 0, int blockAlign = 0, uint32_t chLayout = 0);
void InitEmptyAudioFormat(AudioFormat* fmt);
const TypeNameFormat* GetTypenameFormat();
