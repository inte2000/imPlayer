#include <stdexcept>
#include <cassert>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <propvarutil.h> // 用于 PropVariantToString 等转换函数，需要链接 propsys.lib
#include "AudioDeviceMgmt.h"
#include "DeviceInfoWin.h"
#include "ScopeGuard.h"


#pragma comment(lib, "propsys.lib")


//CAudioDeviceNotifier
ULONG STDMETHODCALLTYPE CAudioDeviceNotifier::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE CAudioDeviceNotifier::Release()
{
    ULONG res = InterlockedDecrement(&m_refCount);
    if (res == 0) delete this;
    return res;
}

HRESULT STDMETHODCALLTYPE CAudioDeviceNotifier::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
        *ppv = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CAudioDeviceNotifier::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR deviceId)
{
    if (flow == eRender && role == eConsole)
    {
        HRESULT hr = CoInitialize(NULL);
        if (FAILED(hr))
            return hr;
        
        CScopeGuard guard_comenv([]() { CoUninitialize(); });

        ComPtr<IMMDeviceEnumerator> pDevEnumerator;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
            CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDevEnumerator);
        if (FAILED(hr))
            return hr;
        /*
        ComPtr<IMMDevice> pDevice;
        hr = pDevEnumerator->GetDevice(deviceId, &pDevice);
        if (FAILED(hr))
            return hr;

        */
        // 3. 获取当前默认输出设备
        ComPtr <IMMDevice> pDefaultDevice;
        hr = pDevEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice);
        if (FAILED(hr))
            return hr;

        std::wstring strDefDevId;
        hr = QueryAudioDeviceIDW(pDefaultDevice.Get(), strDefDevId);
        if (FAILED(hr))
            return hr;

        if (m_strDefaultDevId.compare(strDefDevId) != 0)
        {
            m_strDefaultDevId = strDefDevId;
            if (m_callback)
                m_callback(m_strDefaultDevId);
        }
    }

    return S_OK;
}


CAudioDeviceMgmt::CAudioDeviceMgmt()
{
    m_pDevNotifier = nullptr;
}

CAudioDeviceMgmt::~CAudioDeviceMgmt() 
{
    CloseDevice();
}

BOOL CAudioDeviceMgmt::OpenDevcie(const std::wstring& deviceID, DeviceChangeCallback callback)
{
    assert(!m_pDevEnumerator);
    assert(m_pDevNotifier == nullptr);

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_pDevEnumerator);

    if (FAILED(hr))
        return FALSE;

	if (deviceID.empty())
	{
		if (FAILED(m_pDevEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice)))
			return FALSE;

        hr = QueryAudioDeviceIDW(m_pDevice.Get(), m_curDeviceID);
        if (FAILED(hr))
            return FALSE;
	}
    else
    {
        if(FAILED(m_pDevEnumerator->GetDevice(deviceID.c_str(), &m_pDevice)))
            return FALSE;
	
        m_curDeviceID = deviceID;
    }

    hr = m_pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&m_pEndpointVol);
    if (SUCCEEDED(hr))
    {
        m_pDevNotifier = new CAudioDeviceNotifier(std::move(callback));
        hr = m_pDevEnumerator->RegisterEndpointNotificationCallback(m_pDevNotifier);
        if (FAILED(hr))
            return FALSE;
    }

    return TRUE;
}

void CAudioDeviceMgmt::CloseDevice()
{
    if ((m_pEndpointVol != nullptr) && (m_pDevNotifier != nullptr))
    {
        HRESULT hr = m_pDevEnumerator->UnregisterEndpointNotificationCallback(m_pDevNotifier);
        m_pDevNotifier->Release();
        m_pDevNotifier = nullptr;

        m_pEndpointVol.Release();
    }

    if(m_pDevice != nullptr)
        m_pDevice.Release();

    if (m_pDevEnumerator != nullptr)
        m_pDevEnumerator.Release();

    m_curDeviceID.clear();
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