#ifndef WASAPI_AUDIO_DEVICE_H
#define WASAPI_AUDIO_DEVICE_H

#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include "AudioDevice.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <utility>
#include "ComPtr.h"

#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>

enum class WasapiMode {
	Share = 0,
	Exclusive = 1
};

enum class WasapiState {
	STOPPED,
	STOPPING,
	RUNNING,
	CLOSED = -50
};

typedef struct tagWasapiStream
{
	std::mutex mutex;
	WasapiState state;
	bool userInterleaved;
	int priority;
	HANDLE hThread;
	std::unique_ptr<uint8_t[]> userBuf;
	uint32_t userBufSize;
	uint32_t userBufpos;
}WasapiStream;

class CWasapiAudioDevice;

class CAudioVolumeNotifier : public IAudioEndpointVolumeCallback
{
public:
	CAudioVolumeNotifier(CWasapiAudioDevice* pAudioDevice) : m_refCount(1), m_pAudioDevice(pAudioDevice) {}
	virtual ~CAudioVolumeNotifier() { }

	CAudioVolumeNotifier(const CAudioVolumeNotifier&) = delete;
	CAudioVolumeNotifier& operator=(const CAudioVolumeNotifier&) = delete;
	CAudioVolumeNotifier(CAudioVolumeNotifier&&) = default;
	CAudioVolumeNotifier& operator=(CAudioVolumeNotifier&&) = default;

	// IUnknown
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

	//IAudioEndpointVolumeCallback
	HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override;
private:
	LONG m_refCount;
	CWasapiAudioDevice* m_pAudioDevice;
};

class CWasapiAudioDevice : public CAudioDevice
{
public:
	CWasapiAudioDevice(const std::string& deviceId = "", WasapiMode mode = WasapiMode::Share);
	virtual ~CWasapiAudioDevice();
	void OnDeviceVolumeChanged(int vol, bool bMute);

	AudioFormat NegotiateFormat(const AudioFormat& audioFmt) override;
	const AudioDeviceInfo& GetDeviceInfo() const override;
	const AudioFormat& GetDeviceFormat() const override { return m_curDeviceFmt; }
	bool GetVolume(int* vol, bool* pBMute = nullptr) override;
	bool SetVolume(int vol, bool bMute = false) override;

	bool IsOpened() const override;
	bool IsRunning() const override;
	void Open(const AudioFormat* audioFmt, uint32_t option, const std::string& deviceId = "") override;
	void Close() override;
	void Start() override;
	void Stop() override;
	void Pause() override;

protected:
	HRESULT StartDeviceVolumeNotifirt(IMMDevice* pDevice);
	HRESULT StopDeviceVolumeNotifirt(IMMDevice* pDevice);
	HRESULT GetDeviceVolume(IMMDevice *pDevice, int *pVol, bool* pBMute = nullptr);
	HRESULT SetDeviceVolume(IMMDevice* pDevice, int vol, bool bMute = false);
	void MakeupDeviceInfo(IMMDevice *pDevice, AudioDeviceInfo& info);
	HRESULT GetAudioDeviceObject(const std::string& deviceId, bool mustActive, ComPtr<IMMDevice>& pDevice);
	void InitShareAudioClient(const AudioFormat* audioFmt, uint32_t option);
	void InitExclusiveAudioClient(const AudioFormat* audioFmt, uint32_t option);
	void InitWasapiStream(WasapiStream* pStream);
	void ResetWasapiStream(WasapiStream* pStream);
	void LoadStreamBuffer(BYTE* streamBuffer, uint32_t bufferSize);
	void DoWasapiThread();
	static DWORD WINAPI WasapiThread(void* wasapiPtr);
private:
	AudioFormat m_curDeviceFmt;
	AudioDeviceInfo m_info;
	std::string m_deviceId;
	WasapiMode m_wasapiMode;
	REFERENCE_TIME m_bufferDuration;
	WasapiStream m_stream;
	HANDLE m_hEvent;
	ComPtr<IMMDeviceEnumerator> m_pDevEnumerator;
	ComPtr<IMMDevice> m_pDevice;
	ComPtr<IAudioClient> m_pAudioClient;
	ComPtr<IAudioRenderClient> m_pAudioRender;
	ComPtr<IAudioEndpointVolume> m_pEndpointVol;
	CAudioVolumeNotifier* m_pVolNotifier;
};


#endif //WASAPI_AUDIO_DEVICE_H

