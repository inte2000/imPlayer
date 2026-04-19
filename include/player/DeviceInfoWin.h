#ifndef DEVICE_INFO_WIN_H
#define DEVICE_INFO_WIN_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <utility>
#include <memory>
#include <string>
#include <functional>
#include "ComPtr.h"

#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include <ksmedia.h>

#include <mmeapi.h>
#include <audioclient.h>
#include "AudioInfo.h"


typedef struct tagChannelStyle
{
    const char* desc;
    uint32_t channels;
    uint32_t chLayout;
}ChannelStyle;

typedef struct tagBitsWidthDef
{
    uint32_t bitsPerSample;
    bool bFloat;
}BitsWidthDef;

typedef struct tagAudioDeviceMgmtInfo
{
    std::string strDeviceId;
    std::wstring strFriendlyName;
    std::wstring strDeviceDesc;
    std::wstring strInfName;  // PKEY_DeviceInterface_FriendlyName
    //std::wstring strClass;
    //std::wstring strClassGuid;
    //std::wstring strAudioEndpointAssociation;
    std::wstring strManufacturer;
    UINT         uFormFactor;
    uint32_t       nChannels;
    uint32_t       chLayout;
    bool isDefault;
    UINT physicalSpeakers;
    std::vector<AudioFormat> avlFmts;
}AudioDeviceMgmtInfo;

typedef struct tagAudioDeviceBaseInfo
{
    std::string strDeviceId;
    std::wstring strFriendlyName;
    std::wstring strDeviceDesc;
    bool isDefault;
}AudioDeviceBaseInfo;

typedef struct {
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    DWORD       dwChannelMask;      /* which channels are */
    GUID        SubFormat;
} WAVE_DEVICE_FORMAT, * PWAVE_DEVICE_FORMAT;

HRESULT QueryPropertyStoreString(IPropertyStore* pProps, REFPROPERTYKEY key, std::wstring& strInfo);
HRESULT QueryPropertyStoreInt(IPropertyStore* pProps, REFPROPERTYKEY key, UINT& uVal);
HRESULT QueryPropertyStoreDeviceFormat(IPropertyStore* pProps, WAVE_DEVICE_FORMAT& devFmt);
HRESULT QueryPropertyStorePhysicalSpeakers(IPropertyStore* pProps, UINT& mask);
HRESULT QueryPropertyStoreFormFactor(IPropertyStore* pProps, UINT& formFactor);

bool IsActiveAudioDevice(IMMDevice* pDevice);
HRESULT QueryDefaultDeviceID(IMMDeviceEnumerator* pEnumerator, std::string& defDeviceID);
HRESULT QueryAudioDeviceID(IMMDevice* pDevice, std::string& strDevId);
HRESULT QueryAudioDeviceIDW(IMMDevice* pDevice, std::wstring& strDevId);
HRESULT QueryAudioDeviceFormat(IMMDevice* pDevice, WAVE_DEVICE_FORMAT& devFmt);
HRESULT QueryAudioDevicePhysicalSpeakers(IMMDevice* pDevice, UINT& mask);

void InitWaveFormat(WAVEFORMATEXTENSIBLE* wavFmt, uint32_t sampleRate, const ChannelStyle* chStyle, uint32_t bitsPerSample, bool bFloat);
void FillWaveFormat(WAVEFORMATEXTENSIBLE* pWavFmt, const AudioFormat* audioFmt);
void FillAudioFormat(AudioFormat& audioFmt, const WAVEFORMATEX* pWavFmt);
void CopyWaveFormat(WAVEFORMATEXTENSIBLE* pWavExtFmt, const WAVEFORMATEX* pWavFmt);

HRESULT QueryAudioDeviceInfo(IMMDevice* pDevice, bool bShare, AudioDeviceMgmtInfo& info);
AudioFormat GetFirstAvailableFormats(IAudioClient* pAudioClient, bool bExclusive, uint32_t maxChannels, uint32_t maxSampleRate);
void QueryAudioDeviceChannelInfo(IMMDevice* pDevice, uint32_t& channels, uint32_t& chLayout);
std::vector<AudioFormat> EnumAvailableFormats(IMMDevice* pDevice, bool bExclusive, uint32_t maxChannels, uint32_t maxSampleRate);
std::vector<AudioDeviceBaseInfo> EnumDeviceBaseInfo();
AudioDeviceMgmtInfo GetDeviceInfoById(const std::string& deviceId, bool bExclusive);
bool GetDefaultDeviceId(std::string& deviceId);


#endif //DEVICE_INFO_WIN_H
