/*
20250522 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#ifndef AUDIO_DEVICE_MGMT_H
#define AUDIO_DEVICE_MGMT_H

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


using DeviceChangeCallback = std::function<void(const std::wstring& strDefDevId)>;

class CAudioDeviceNotifier : public IMMNotificationClient
{
public:
    CAudioDeviceNotifier(DeviceChangeCallback callback) : m_refCount(1), m_callback(std::move(callback)) {}
    virtual ~CAudioDeviceNotifier() { }

    CAudioDeviceNotifier(const CAudioDeviceNotifier&) = delete;
    CAudioDeviceNotifier& operator=(const CAudioDeviceNotifier&) = delete;
    CAudioDeviceNotifier(CAudioDeviceNotifier&&) = default;
    CAudioDeviceNotifier& operator=(CAudioDeviceNotifier&&) = default;

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    // IMMNotificationClient methods
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR deviceId) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

private:
    LONG m_refCount;
    std::wstring m_strDefaultDevId;
    DeviceChangeCallback m_callback;
};

class CAudioDeviceMgmt final
{
public:
    CAudioDeviceMgmt();
    ~CAudioDeviceMgmt();
    CAudioDeviceMgmt(const CAudioDeviceMgmt&) = delete;
    CAudioDeviceMgmt& operator=(const CAudioDeviceMgmt&) = delete;
    CAudioDeviceMgmt(CAudioDeviceMgmt&&) = default;
    CAudioDeviceMgmt& operator=(CAudioDeviceMgmt&&) = default;

    //for device change
    BOOL OpenDevcie(const std::wstring& deviceID, DeviceChangeCallback callback);
    void CloseDevice();
    BOOL IsDeviceOpen() const { return (m_pDevice != NULL) && (m_pDevEnumerator != nullptr); }
    BOOL IsSameDevice(const std::wstring& strDefDevId) const { return m_curDeviceID == strDefDevId; };
private:
    CAudioDeviceNotifier* m_pDevNotifier;
    std::wstring m_curDeviceID;
    ComPtr<IMMDeviceEnumerator> m_pDevEnumerator;
    ComPtr<IMMDevice> m_pDevice;
    ComPtr<IAudioEndpointVolume> m_pEndpointVol;
};


#endif //AUDIO_DEVICE_MGMT_H
