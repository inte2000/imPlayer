#include <stdexcept>
#include <cassert>
#include <bit>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <propvarutil.h> // 用于 PropVariantToString 等转换函数，需要链接 propsys.lib
//#include <devicetopology.h> //ConnectorType
#include "DeviceInfoWin.h"
#include "AudioInfo.h"
#include "UnicodeConvert.h"

#pragma comment(lib, "propsys.lib")


HRESULT QueryPropertyStoreString(IPropertyStore* pProps, REFPROPERTYKEY key, std::wstring& strInfo)
{
    PROPVARIANT varTemp;
    PropVariantInit(&varTemp);
    HRESULT hr = pProps->GetValue(key, &varTemp);
    if (FAILED(hr))
        return hr;
    if (varTemp.vt == VT_LPWSTR)
        strInfo = varTemp.pwszVal;
    else
    {
        LPWSTR pszValue = nullptr;
        if (SUCCEEDED(PropVariantToStringAlloc(varTemp, &pszValue)))
        {
            strInfo = pszValue;
            CoTaskMemFree(pszValue);
        }
    }
    PropVariantClear(&varTemp);

    return hr;
}

HRESULT QueryPropertyStoreInt(IPropertyStore* pProps, REFPROPERTYKEY key, UINT& uVal)
{
    PROPVARIANT varTemp;
    PropVariantInit(&varTemp);
    HRESULT hr = pProps->GetValue(key, &varTemp);
    if (FAILED(hr))
        return hr;

    uVal = varTemp.ulVal;
    PropVariantClear(&varTemp);

    return hr;
}

HRESULT QueryPropertyStoreDeviceFormat(IPropertyStore* pProps, WAVE_DEVICE_FORMAT& devFmt)
{
    PROPVARIANT varTemp;
    PropVariantInit(&varTemp);
    HRESULT hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varTemp);
    if (SUCCEEDED(hr) && (varTemp.vt == VT_BLOB))
    {
        hr = S_FALSE;
        if (varTemp.blob.cbSize >= sizeof(WAVEFORMATEX))
        {
            WAVEFORMATEX* pFormat = (WAVEFORMATEX*)varTemp.blob.pBlobData;
            devFmt.wFormatTag = pFormat->wFormatTag;
            devFmt.nChannels = pFormat->nChannels;
            devFmt.nSamplesPerSec = pFormat->nSamplesPerSec;
            devFmt.wBitsPerSample = pFormat->wBitsPerSample;
            std::memset(&devFmt.SubFormat, 0, sizeof(GUID));
            devFmt.dwChannelMask = 0;
            if ((pFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) && (pFormat->cbSize >= 22))
            {
                WAVEFORMATEXTENSIBLE* pWavExt = (WAVEFORMATEXTENSIBLE*)varTemp.blob.pBlobData;
                devFmt.SubFormat = pWavExt->SubFormat;
                devFmt.dwChannelMask = pWavExt->dwChannelMask;
            }

            hr = S_OK;
        }

        PropVariantClear(&varTemp);
    }

    return hr;
}

HRESULT QueryPropertyStorePhysicalSpeakers(IPropertyStore* pProps, UINT& mask)
{
    PROPVARIANT var;
    PropVariantInit(&var);

    mask = 0;
    HRESULT hr = pProps->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &var);
    if (SUCCEEDED(hr) && (var.vt == VT_UI4))
    {
        mask = var.ulVal;
    }

    PropVariantClear(&var);
    return hr;
}

/*
0: RemoteNetworkDevice 远程网络设备，可能支持多声道
1：Speakers 扬声器，可能支持多声道
2：LineLevel 线路电平设备，通常支持立体声
3：Headphones 耳机，通常立体声
4：Microphone 麦克风，单声道
5：Headset，头戴式耳机（带麦），耳机立体声，麦克风单声道
6：Handset，手持式耳机，单声道
7：DigitalAudioDisplayDevic 数字音频显示设备，可能支持多声道（如HDMI）
8：SPDIF，SPDIF接口，可能支持多声道
9：HDMI HDMI接口，可能支持多声道
*/
HRESULT QueryPropertyStoreFormFactor(IPropertyStore* pProps, UINT& formFactor)
{
    PROPVARIANT var;
    PropVariantInit(&var);

    formFactor = -1;
    HRESULT hr = pProps->GetValue(PKEY_AudioEndpoint_FormFactor, &var);
    if (SUCCEEDED(hr) && (var.vt == VT_UI4))
    {
        formFactor = var.uintVal;
    }

    PropVariantClear(&var);
    return hr;
}


/*
According to the MSDN docs "Any PCM format that has more than 2 channels,
more than 16 bits per sample, or more than 44,100 samples per second must
be described by WAVEFORMATEXTENSIBLE"
*/
void InitWaveFormat(WAVEFORMATEXTENSIBLE* wavFmt, uint32_t sampleRate, const ChannelStyle* chStyle, uint32_t bitsPerSample, bool bFloat)
{
    if (bFloat || bitsPerSample > 16 || chStyle->channels > 2)
    {
        wavFmt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wavFmt->Format.nChannels = chStyle->channels;
        wavFmt->Format.nSamplesPerSec = sampleRate;
        wavFmt->Format.wBitsPerSample = bitsPerSample;
        wavFmt->Format.nBlockAlign = chStyle->channels * (bitsPerSample / 8);
        wavFmt->Format.nAvgBytesPerSec = sampleRate * wavFmt->Format.nBlockAlign;
        wavFmt->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

        wavFmt->Samples.wValidBitsPerSample = bitsPerSample;
        wavFmt->dwChannelMask = chStyle->chLayout;
        wavFmt->SubFormat = bFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
        //std::memcpy(&wavFmt->SubFormat, bFloat ? &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : &KSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID));
    }
    else
    {
        wavFmt->Format.wFormatTag = WAVE_FORMAT_PCM;
        wavFmt->Format.nChannels = chStyle->channels;
        wavFmt->Format.nSamplesPerSec = sampleRate;
        wavFmt->Format.wBitsPerSample = bitsPerSample;
        wavFmt->Format.nBlockAlign = chStyle->channels * (bitsPerSample / 8);
        wavFmt->Format.nAvgBytesPerSec = sampleRate * wavFmt->Format.nBlockAlign;
        wavFmt->Format.cbSize = 0;
    }
}

void FillWaveFormat(WAVEFORMATEXTENSIBLE* pWavFmt, const AudioFormat* audioFmt)
{
    bool bExtensible = (audioFmt->numChannels > 2) || (audioFmt->chLayout != 0);

    pWavFmt->Format.nChannels = audioFmt->numChannels;
    pWavFmt->Format.nSamplesPerSec = audioFmt->sampleRate;
    pWavFmt->Format.wBitsPerSample = GetBitsPerSampleByFormat(audioFmt->format);
    //pWavFmt->Format.nBlockAlign = pWavFmt->Format.nChannels * pWavFmt->Format.wBitsPerSample / 8;
    pWavFmt->Format.nBlockAlign = audioFmt->blockAlign;
    pWavFmt->Format.nAvgBytesPerSec = pWavFmt->Format.nSamplesPerSec * pWavFmt->Format.nBlockAlign;
    pWavFmt->Format.cbSize = bExtensible ? (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) : 0;
    if (bExtensible)
    {
        pWavFmt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;

        pWavFmt->Samples.wValidBitsPerSample = pWavFmt->Format.wBitsPerSample;
        //解码器输出的数据格式只可能是 float 或 int
        if ((audioFmt->format == AudioDataFormat::Float32) || (audioFmt->format == AudioDataFormat::Float64))
            pWavFmt->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        else
            pWavFmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        pWavFmt->dwChannelMask = audioFmt->chLayout;
    }
    else
    {
        if ((audioFmt->format == AudioDataFormat::Float32) || (audioFmt->format == AudioDataFormat::Float64))
            pWavFmt->Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        else
            pWavFmt->Format.wFormatTag = WAVE_FORMAT_PCM;
    }
}

void FillAudioFormat(AudioFormat& audioFmt, const WAVEFORMATEX* pWavFmt)
{
    audioFmt.numChannels = pWavFmt->nChannels;
    audioFmt.sampleRate = pWavFmt->nSamplesPerSec;
    audioFmt.bitsPerSample = pWavFmt->wBitsPerSample;
    audioFmt.blockAlign = pWavFmt->nBlockAlign;

    if (pWavFmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        WAVEFORMATEXTENSIBLE* pWavExFmt = (WAVEFORMATEXTENSIBLE*)pWavFmt;
        audioFmt.chLayout = pWavExFmt->dwChannelMask;
        if (audioFmt.chLayout == 0)
            audioFmt.chLayout = StandLayoutByChannelsCount(audioFmt.numChannels);
        //if (IsEqualGUID(&pWavExFmt->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
        if (pWavExFmt->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        {
            audioFmt.format = (pWavExFmt->Format.wBitsPerSample == 32) ? AudioDataFormat::Float32 : AudioDataFormat::Float64;
        }
        else if (pWavExFmt->SubFormat == KSDATAFORMAT_SUBTYPE_ADPCM)
        {
            audioFmt.format = AudioDataFormat::MsAdpcm;
        }
        else
        {
            if (pWavExFmt->Format.wBitsPerSample == 8)
                audioFmt.format = AudioDataFormat::PCM_S8;
            else if (pWavExFmt->Format.wBitsPerSample == 16)
                audioFmt.format = AudioDataFormat::PCM_S16;
            else if (pWavExFmt->Format.wBitsPerSample == 24)
                audioFmt.format = AudioDataFormat::PCM_S24;
            else if (pWavExFmt->Format.wBitsPerSample == 32)
                audioFmt.format = AudioDataFormat::PCM_S32;
            else
                audioFmt.format = AudioDataFormat::PCM_64;
        }
    }
    else
    {
        audioFmt.chLayout = 0x03;
        //1: PCM integer, 3: IEEE 754 float
        if (pWavFmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        {
            audioFmt.format = (pWavFmt->wBitsPerSample == 32) ? AudioDataFormat::Float32 : AudioDataFormat::Float64;
        }
        else if (pWavFmt->wFormatTag == WAVE_FORMAT_ADPCM)
        {
            audioFmt.format = AudioDataFormat::MsAdpcm;
        }
        else
        {
            if (pWavFmt->wBitsPerSample == 8)
                audioFmt.format = AudioDataFormat::PCM_S8;
            else if (pWavFmt->wBitsPerSample == 16)
                audioFmt.format = AudioDataFormat::PCM_S16;
            else if (pWavFmt->wBitsPerSample == 24)
                audioFmt.format = AudioDataFormat::PCM_S24;
            else if (pWavFmt->wBitsPerSample == 32)
                audioFmt.format = AudioDataFormat::PCM_S32;
            else
                audioFmt.format = AudioDataFormat::PCM_64;
        }
    }
}

void CopyWaveFormat(WAVEFORMATEXTENSIBLE* pWavExtFmt, const WAVEFORMATEX* pWavFmt)
{
    pWavExtFmt->Format = *pWavFmt;
    if ((pWavFmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE) && (pWavFmt->cbSize >= 22))
    {
        WAVEFORMATEXTENSIBLE* pWavExFmt = (WAVEFORMATEXTENSIBLE*)pWavFmt;
        pWavExtFmt->Samples = pWavExFmt->Samples;
        pWavExtFmt->dwChannelMask = pWavExFmt->dwChannelMask;
        pWavExtFmt->SubFormat = pWavExFmt->SubFormat;
    }
}

HRESULT QueryAudioDeviceInfo(IMMDevice* pDevice, bool bShare, AudioDeviceMgmtInfo& info)
{
    HRESULT hr = S_OK;

    hr = QueryAudioDeviceID(pDevice, info.strDeviceId);
    if (FAILED(hr))
        return hr;

    ComPtr <IPropertyStore> pProps;
    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (FAILED(hr))
        return hr;

    hr = QueryPropertyStoreString(pProps.Get(), PKEY_Device_FriendlyName, info.strFriendlyName);
    if (FAILED(hr))
        return hr;
    hr = QueryPropertyStoreString(pProps.Get(), PKEY_Device_DeviceDesc, info.strDeviceDesc);
    if (FAILED(hr))
        return hr;
    hr = QueryPropertyStoreString(pProps.Get(), PKEY_Device_Manufacturer, info.strManufacturer);
    if (FAILED(hr))
        return hr;
    hr = QueryPropertyStoreString(pProps.Get(), PKEY_DeviceInterface_FriendlyName, info.strInfName);
    if (FAILED(hr))
        return hr;

    QueryPropertyStoreFormFactor(pProps.Get(), info.uFormFactor);

    info.nChannels = 2;
    info.chLayout = 0x03;
    if (bShare)
    {
        WAVE_DEVICE_FORMAT devFmt;
        hr = QueryAudioDeviceFormat(pDevice, devFmt);
        if (FAILED(hr))
        {
            hr = QueryPropertyStoreDeviceFormat(pProps.Get(), devFmt);
        }
        if (SUCCEEDED(hr))
        {
            info.nChannels = devFmt.nChannels;
            if (devFmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            {
                info.chLayout = devFmt.dwChannelMask;
            }
            else
            {
                info.chLayout = StandLayoutByChannelsCount(devFmt.nChannels);
            }
        }
    }
    else
    {
        info.physicalSpeakers = 0x03;
        if(info.uFormFactor == 1) //扬声器
            hr = QueryPropertyStorePhysicalSpeakers(pProps.Get(), info.physicalSpeakers);
        else
            hr = QueryAudioDevicePhysicalSpeakers(pDevice, info.physicalSpeakers);

        if (SUCCEEDED(hr))
        {
            info.nChannels = std::popcount(info.physicalSpeakers);
            info.chLayout = info.physicalSpeakers;
        }
        else
        {
            hr = S_OK;
            info.nChannels = 2;
            info.chLayout = 0x03;
        }
    }

    /*
    hr = QueryPropertyStoreString(pProps.Get(), PKEY_Device_Class, info.strClass);
    if (FAILED(hr))
        return hr;
    hr = QueryPropertyStoreString(pProps.Get(), PKEY_Device_ClassGuid, info.strClassGuid);
    if (FAILED(hr))
        return hr;
    hr = QueryPropertyStoreString(pProps.Get(), PKEY_AudioEndpoint_Association, info.strAudioEndpointAssociation);
    if (FAILED(hr))
        return hr;
    hr = QueryPropertyStoreInt(pProps.Get(), PKEY_AudioEndpoint_FormFactor, info.uFormFactor);
    */
    return hr;
}

bool IsActiveAudioDevice(IMMDevice* pDevice)
{
    bool bActive = false;
    DWORD dwState = 0;
    HRESULT hr = pDevice->GetState(&dwState);
    if (SUCCEEDED(hr))
    {
        bActive = ((dwState & DEVICE_STATE_ACTIVE) == DEVICE_STATE_ACTIVE);
    }

    return bActive;
}

HRESULT QueryDefaultDeviceID(IMMDeviceEnumerator* pEnumerator, std::string& defDeviceID)
{
    ComPtr<IMMDevice> pDefaultDevice;
    HRESULT hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice);
    if (FAILED(hr))
        return hr;

    return QueryAudioDeviceID(pDefaultDevice.Get(), defDeviceID);
}

HRESULT QueryAudioDeviceID(IMMDevice* pDevice, std::string& strDevId)
{
    std::wstring wstrDevId;
    HRESULT hr = QueryAudioDeviceIDW(pDevice, wstrDevId);
    if (SUCCEEDED(hr))
    {
        strDevId = Utf16ToUtf8(wstrDevId);
    }

    return hr;
}

HRESULT QueryAudioDeviceIDW(IMMDevice* pDevice, std::wstring& strDevId)
{
    LPWSTR pszDeviceId = nullptr;
    HRESULT hr = pDevice->GetId(&pszDeviceId);
    if (SUCCEEDED(hr))
    {
        strDevId = pszDeviceId;
        CoTaskMemFree(pszDeviceId);
    }

    return hr;
}

HRESULT QueryAudioDeviceFormat(IMMDevice* pDevice, WAVE_DEVICE_FORMAT& devFmt)
{
    ComPtr<IAudioClient> pAudioClient;
    HRESULT hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (SUCCEEDED(hr))
    {
        WAVEFORMATEX* pMixFormat = NULL;
        hr = pAudioClient->GetMixFormat(&pMixFormat);
        if (SUCCEEDED(hr))
        {
            devFmt.wFormatTag = pMixFormat->wFormatTag;
            devFmt.nChannels = pMixFormat->nChannels;
            devFmt.nSamplesPerSec = pMixFormat->nSamplesPerSec;
            devFmt.wBitsPerSample = pMixFormat->wBitsPerSample;
            std::memset(&devFmt.SubFormat, 0, sizeof(GUID));
            devFmt.dwChannelMask = 0;
            if ((pMixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) && (pMixFormat->cbSize >= 22))
            {
                WAVEFORMATEXTENSIBLE* pWavExt = (WAVEFORMATEXTENSIBLE*)pMixFormat;
                devFmt.SubFormat = pWavExt->SubFormat;
                devFmt.dwChannelMask = pWavExt->dwChannelMask;
            }

            CoTaskMemFree(pMixFormat);
        }
    }

    return hr;
}

// 使用 IKsJackDescription
HRESULT QueryAudioDevicePhysicalSpeakers(IMMDevice* pDevice, UINT& mask)
{
    mask = 0;
    ComPtr<IDeviceTopology> pTopology;
    HRESULT hr = pDevice->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&pTopology);
    if (FAILED(hr))
        return hr;

    // 获取连接器数量
    UINT connectorCount = 0;
    hr = pTopology->GetConnectorCount(&connectorCount);
    for (UINT i = 0; i < connectorCount; i++)
    {
        ComPtr<IConnector> pConnector;
        hr = pTopology->GetConnector(i, &pConnector);
        if (FAILED(hr))
            continue;

        // 获取连接器类型
        ConnectorType connType;
        hr = pConnector->GetType(&connType);
        if (SUCCEEDED(hr)) 
        {
            // 只处理物理连接器
            if (connType == Physical_External || connType == Physical_Internal)
            {
                BOOL bConnect = FALSE;
                hr = pConnector->IsConnected(&bConnect);
                LPWSTR pwstrDeviceId = NULL;
                hr = pConnector->GetDeviceIdConnectedTo(&pwstrDeviceId);
                if (SUCCEEDED(hr) && (pwstrDeviceId != NULL))
                    CoTaskMemFree(pwstrDeviceId);

                // 尝试获取插孔描述
                ComPtr<IKsJackDescription> pJackDesc;
                hr = pConnector->QueryInterface(__uuidof(IKsJackDescription), (void**)&pJackDesc);
                if (SUCCEEDED(hr))
                {
                    UINT jackCount = 0;
                    pJackDesc->GetJackCount(&jackCount); //"音频插孔数量: "
                    int totalConnectedChannels = 0;

                    for (UINT j = 0; j < jackCount; j++)
                    {
                        KSJACK_DESCRIPTION desc;
                        ZeroMemory(&desc, sizeof(desc));
                        if (SUCCEEDED(pJackDesc->GetJackDescription(j, &desc)))
                        {
                            //desc.IsConnected ? "已连接" : "未连接"
                            if (desc.IsConnected && desc.ChannelMapping != 0)
                            {
                                mask |= desc.ChannelMapping;
                                // 统计已连接的声道
                                totalConnectedChannels += std::popcount(desc.ChannelMapping);
                            }
                        }
                    }

                    pJackDesc.Release();
                }
            }
        }    
    }

    return hr;
}

void QueryAudioDeviceChannelInfo(IMMDevice* pDevice, uint32_t& channels, uint32_t& chLayout)
{
    channels = 2;
    chLayout = 0x03;

    ComPtr <IPropertyStore> pProps;
    HRESULT hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (SUCCEEDED(hr))
    {
        uint32_t physicalSpeakers = 0;
        hr = QueryPropertyStorePhysicalSpeakers(pProps.Get(), physicalSpeakers);
        if (FAILED(hr) || (physicalSpeakers == 0))
            hr = QueryAudioDevicePhysicalSpeakers(pDevice, physicalSpeakers);

        if (SUCCEEDED(hr) && (physicalSpeakers > 0))
        {
            channels = std::popcount(physicalSpeakers);
            chLayout = physicalSpeakers;
        }
        else
        {
            WAVE_DEVICE_FORMAT devFmt{};
            hr = QueryPropertyStoreDeviceFormat(pProps.Get(), devFmt);
            if (FAILED(hr))
            {
                hr = QueryAudioDeviceFormat(pDevice, devFmt);
            }

            if (SUCCEEDED(hr))
            {
                channels = devFmt.nChannels;
                chLayout = (devFmt.dwChannelMask != 0) ? devFmt.dwChannelMask : StandLayoutByChannelsCount(devFmt.nChannels);
            }
        }
    }
}


// 检查当前默认音频设备是否为“耳机”
bool IsCurrentOutputHeadphones()
{
    ComPtr<IMMDeviceEnumerator> pEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    if (SUCCEEDED(hr))
        return false;

    ComPtr<IMMDevice> pDevice;
    if (FAILED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice)))
        return false;

    ComPtr <IPropertyStore> pProps;
    if (FAILED(pDevice->OpenPropertyStore(STGM_READ, &pProps)))
        return false;

    PROPVARIANT varName;
    PropVariantInit(&varName);
    bool result = false;

    if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
        std::wstring name = varName.pwszVal;
        if (name.find(L"耳机") != std::wstring::npos || name.find(L"Headphones") != std::wstring::npos) {
            result = true;
        }
    }

    PropVariantClear(&varName);
    return result;
}


uint32_t s_sampleRate[] = { /*384000, 352800,*/ 192000, 176400, 96000, 88200, 48000, 44100 };
BitsWidthDef s_bitsDefs[] = { {32, true}, {32, false}, {24, false}, {16, false} };
ChannelStyle s_chStyles[] = {
    { "7.1 (Side)", 8, 0x63F },
    { "7.1 (Wide)", 8, 0x6CF },
    { "6.1", 7, 0x70F },
    { "5.1", 6, 0x3F },
    { "5.1 (Side)", 6, 0x60F },
    { "4.0 (Quad)", 4, 0x33 },
    { "2.1", 3, 0x0B },
    { "2.0", 2, 0x03 }
};

AudioFormat GetFirstAvailableFormats(IAudioClient* pAudioClient, bool bExclusive, uint32_t maxChannels, uint32_t maxSampleRate)
{
    AudioFormat devFmt;

    AUDCLNT_SHAREMODE mode = bExclusive ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED;
    std::size_t cs_count = sizeof(s_chStyles) / sizeof(s_chStyles[0]);
    for (std::size_t sc = 0; sc < cs_count; ++sc)
    {
        if (s_chStyles[sc].channels > maxChannels) //超过声道能力，就不再尝试探测
            continue;

        std::size_t sr_count = sizeof(s_sampleRate) / sizeof(s_sampleRate[0]);
        for (std::size_t sr = 0; sr < sr_count; ++sr)
        {
            if (s_sampleRate[sr] > maxSampleRate)
                continue;

            std::size_t bd_count = sizeof(s_bitsDefs) / sizeof(s_bitsDefs[0]);
            for (std::size_t bd = 0; bd < bd_count; ++bd)
            {
                WAVEFORMATEXTENSIBLE wavFmt = { 0 };
                InitWaveFormat(&wavFmt, s_sampleRate[sr], &s_chStyles[sc], s_bitsDefs[bd].bitsPerSample, s_bitsDefs[bd].bFloat);
                HRESULT hr = pAudioClient->IsFormatSupported(mode, (const WAVEFORMATEX*)&wavFmt, NULL);
                if (hr == S_OK)
                {
                    FillAudioFormat(devFmt, (const WAVEFORMATEX*)&wavFmt);
                    return devFmt;
                }
            }
        }
    }

    InitAudioFormat(&devFmt, AudioDataFormat::PCM_S16, 2, 44100); //默认使用 CD 音质格式，被支持的概率更高
    return devFmt;
}

std::vector<AudioFormat> EnumAvailableFormats(IMMDevice* pDevice, bool bExclusive, uint32_t maxChannels, uint32_t maxSampleRate)
{
    std::vector<AudioFormat> devFmts;

    AUDCLNT_SHAREMODE mode = bExclusive ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED;
    ComPtr<IAudioClient> pAudioClient;
    HRESULT hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (SUCCEEDED(hr))
    {
        std::size_t cs_count = sizeof(s_chStyles) / sizeof(s_chStyles[0]);
        for (std::size_t sc = 0; sc < cs_count; ++sc)
        {
            if (s_chStyles[sc].channels > maxChannels) //超过声道能力，就不再尝试探测
                continue;

            std::size_t sr_count = sizeof(s_sampleRate) / sizeof(s_sampleRate[0]);
            for (std::size_t sr = 0; sr < sr_count; ++sr)
            {
                if (s_sampleRate[sr] > maxSampleRate)
                    continue;

                std::size_t bd_count = sizeof(s_bitsDefs) / sizeof(s_bitsDefs[0]);
                for (std::size_t bd = 0; bd < bd_count; ++bd)
                {
                    WAVEFORMATEXTENSIBLE wavFmt = { 0 };
                    InitWaveFormat(&wavFmt, s_sampleRate[sr], &s_chStyles[sc], s_bitsDefs[bd].bitsPerSample, s_bitsDefs[bd].bFloat);
                    if (mode == AUDCLNT_SHAREMODE_SHARED)
                    {
                        WAVEFORMATEX* pClosestMatch = NULL;
                        hr = pAudioClient->IsFormatSupported(mode, (const WAVEFORMATEX*)&wavFmt, &pClosestMatch);
                        if (pClosestMatch) 
                            CoTaskMemFree(pClosestMatch);
                    }
                    else
                        hr = pAudioClient->IsFormatSupported(mode, (const WAVEFORMATEX*)&wavFmt, NULL);

                    if (hr == S_OK)
                    {
                        AudioFormat audioFmt;
                        FillAudioFormat(audioFmt, (const WAVEFORMATEX*)&wavFmt);
                        devFmts.push_back(audioFmt);
                    }
                }
            }
        }
    }

    return devFmts;
}

std::vector<AudioDeviceBaseInfo> EnumDeviceBaseInfo()
{
    std::vector<AudioDeviceBaseInfo> baseInfos;

    ComPtr<IMMDeviceEnumerator> pDevEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDevEnumerator);
    if (SUCCEEDED(hr))
    {
        std::string defaultDeviceId;
        QueryDefaultDeviceID(pDevEnumerator.Get(), defaultDeviceId);
        ComPtr<IMMDeviceCollection> pDeviceColl;
        hr = pDevEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDeviceColl);
        if (SUCCEEDED(hr))
        {
            UINT count = 0;
            pDeviceColl->GetCount(&count);
            for (UINT i = 0; i < count; ++i)
            {
                ComPtr<IMMDevice> pDevice;
                pDeviceColl->Item(i, &pDevice);

                AudioDeviceBaseInfo info;
                hr = QueryAudioDeviceID(pDevice.Get(), info.strDeviceId);
                if (SUCCEEDED(hr))
                {
                    info.isDefault = (info.strDeviceId == defaultDeviceId);
                    ComPtr <IPropertyStore> pProps;
                    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
                    if (SUCCEEDED(hr))
                    {
                        QueryPropertyStoreString(pProps.Get(), PKEY_Device_FriendlyName, info.strFriendlyName);
                        QueryPropertyStoreString(pProps.Get(), PKEY_Device_DeviceDesc, info.strDeviceDesc);
                    }

                    baseInfos.push_back(std::move(info));
                }
            }
        }
    }

    return baseInfos;
}

AudioDeviceMgmtInfo GetDeviceInfoById(const std::string& deviceId, bool bExclusive)
{
    AudioDeviceMgmtInfo deviceInfo;

    ComPtr<IMMDeviceEnumerator> pDevEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDevEnumerator);
    if (SUCCEEDED(hr))
    {
        ComPtr<IMMDevice> pDevice;
        std::wstring wstrDeviceId = UTtf8ToUtf16Le(deviceId);
        hr = pDevEnumerator->GetDevice(wstrDeviceId.c_str(), &pDevice);
        if (SUCCEEDED(hr))
        {
            ComPtr <IPropertyStore> pProps;
            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if (SUCCEEDED(hr))
            {
                QueryPropertyStoreString(pProps.Get(), PKEY_Device_FriendlyName, deviceInfo.strFriendlyName);
                QueryPropertyStoreString(pProps.Get(), PKEY_Device_DeviceDesc, deviceInfo.strDeviceDesc);
                QueryPropertyStoreString(pProps.Get(), PKEY_DeviceInterface_FriendlyName, deviceInfo.strInfName);
                QueryPropertyStoreFormFactor(pProps.Get(), deviceInfo.uFormFactor);

                deviceInfo.nChannels = 2;
                deviceInfo.physicalSpeakers = 0x03;
                QueryAudioDeviceChannelInfo(pDevice.Get(), deviceInfo.nChannels, deviceInfo.physicalSpeakers);
                deviceInfo.avlFmts = EnumAvailableFormats(pDevice.Get(), bExclusive, deviceInfo.nChannels, 384000);
            }

            std::string defaultDeviceId;
            QueryDefaultDeviceID(pDevEnumerator.Get(), defaultDeviceId);
            deviceInfo.isDefault = (defaultDeviceId == deviceId);
        }
    }

    return deviceInfo;
}

bool GetDefaultDeviceId(std::string& deviceId)
{
    ComPtr<IMMDeviceEnumerator> pDevEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDevEnumerator);
    if (SUCCEEDED(hr))
    {
        hr = QueryDefaultDeviceID(pDevEnumerator.Get(), deviceId);
        return SUCCEEDED(hr);
    }

    return false;
}


#if 0
方法 1：查看新默认设备名称是否包含：

“Headphones”

“耳机”

“3.5mm”

“Realtek HD Audio 2nd output”

可通过 IMMDevice::OpenPropertyStore 获取 PKEY_Device_FriendlyName
✅ 方法 2：枚举设备 + 检查状态

使用 IMMDeviceEnumerator::EnumAudioEndpoints 枚举所有输出设备，并通过状态或属性判断是否可用。
#endif

#if 0
// 获取音频端点接口
IAudioEndpointVolume * pEndpointVol = nullptr;
hr = pDevice->Activate(__uuidof(IAudioEndpointVolume),
    CLSCTX_ALL,
    nullptr,
    (void**)&pEndpointVol);

if (SUCCEEDED(hr))
{
    // 可以查询或设置音量、静音状态等
    BOOL bMuted;
    pEndpointVol->GetMute(&bMuted);
    wprintf(L"Device is %s\n", bMuted ? L"muted" : L"not muted");
    pEndpointVol->Release();
}
#endif