/*
Human Action，指定接口定义
*/
#ifndef AUDIO_DEVICE_H
#define AUDIO_DEVICE_H

#include <vector>
#include <memory>
#include <functional>
#include "AudioInfo.h"


struct AudioDeviceInfo
{
	std::string id;
	std::string name;
	bool isOutput;
	bool isDefault;
	uint32_t channels;
	uint32_t chLayout;

	uint32_t userBufFrames;
};

//typedef int (*DeviceCallbackFuncPtr)(void* out_buf, void* in_buf, uint32_t frames, double stream_time, int status, void* user_Data);
using DeviceCallback = std::function<int(void*, uint32_t, double, int)>;
using DeviceVolumeNotifier = std::function<void(int vol, bool bMute)>;

#define DEVICE_OPTION_REALTIME_PRIORITY          0x00000001
#define DEVICE_OPTION_FORCE_FORMAT               0x00000002
#define DEVICE_OPTION_NONINTERLEAVED             0x00000004


class CAudioDevice
{
public:
	CAudioDevice() = default;
	CAudioDevice(const CAudioDevice& r) = delete;
	CAudioDevice& operator = (const CAudioDevice& r) = delete;
	virtual ~CAudioDevice() = default;

	const DeviceCallback& GetCallback() const { return m_callback; }
	DeviceCallback& GetCallback() { return m_callback; }
	void SetCallback(DeviceCallback&& callback) { m_callback = std::move(callback); }

	const DeviceVolumeNotifier& GetVolumeNotifier() const { return m_volumeNotifier; }
	DeviceVolumeNotifier& GetVolumeNotifier() { return m_volumeNotifier; }
	void SetVolumeNotifier(DeviceVolumeNotifier&& callback) { m_volumeNotifier = std::move(callback); }

	virtual AudioFormat NegotiateFormat(const AudioFormat& audioFmt) = 0;
	virtual const AudioDeviceInfo& GetDeviceInfo() const = 0;
	virtual const AudioFormat& GetDeviceFormat() const = 0;
	virtual bool GetVolume(int *vol, bool *pBMute = nullptr) = 0;
	virtual bool SetVolume(int vol, bool bMute = false) = 0;
	virtual bool IsOpened() const = 0;
	virtual bool IsRunning() const = 0;
	virtual void Open(const AudioFormat* audioFmt, uint32_t option, const std::string& deviceId = "") = 0;
	virtual void Close() = 0;
	virtual void Start() = 0;
	virtual void Pause() = 0;
	virtual void Stop() = 0;

protected:
	DeviceCallback m_callback;
	DeviceVolumeNotifier m_volumeNotifier;
};


#endif

