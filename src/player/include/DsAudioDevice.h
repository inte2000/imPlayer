/*
20250522 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#ifndef DIRECT_SOUND_AUDIO_DEVICE_H
#define DIRECT_SOUND_AUDIO_DEVICE_H

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <chrono>
#include "AudioDevice.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <utility>
#include <dsound.h>
#include "ComPtr.h"

enum class DirectSoundState {
	STOPPED,
	STOPPING,
	RUNNING,
	CLOSED = -50
};

typedef struct tagDSoundStream
{
	std::mutex mutex;
	DirectSoundState state;
	bool userInterleaved;
	int priority;
	HANDLE hThread;
	std::unique_ptr<uint8_t[]> userBuf;
	uint32_t userBufSize;
	uint32_t userBufpos;
	DWORD dsBufSize;
	DWORD dsBufCount;
}DSoundStream;


class CDsAudioDevice : public CAudioDevice
{
public:
	CDsAudioDevice(const std::string& deviceId = "");
	virtual ~CDsAudioDevice();
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
	void InitDsoundStream(DSoundStream* pStream);
	void ResetDsoundStream(DSoundStream* pStream);
	void QueryDeviceInfo(AudioDeviceInfo& info);
	HRESULT FillDSoundBuffer(DWORD bufIndex);

	void DoRenderThread();
	static DWORD WINAPI DirectSoundThread(void* dsPtr);
private:
	AudioFormat m_curDeviceFmt;
	AudioDeviceInfo m_info;
	std::string m_deviceId;
	DSoundStream m_stream;
	
	ComPtr<IDirectSound8> m_ds;
	ComPtr<IDirectSoundBuffer> m_primary;
	ComPtr<IDirectSoundBuffer> m_secondary;
	//ComPtr<IDirectSoundBuffer8> m_buffer;
};


#endif //DIRECT_SOUND_AUDIO_DEVICE_H

