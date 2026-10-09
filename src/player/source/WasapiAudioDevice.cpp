/*
20250522 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#include <cassert>
#include <algorithm>
#include <sstream>
#include "UnicodeConvert.h"
#include "WasapiAudioDevice.h"
#include "DeviceInfoWin.h"
#include "ScopeGuard.h"

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

extern uint32_t BufferFramesByRate(uint32_t samplerate);

//CAudioVolumeNotifier
ULONG STDMETHODCALLTYPE CAudioVolumeNotifier::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE CAudioVolumeNotifier::Release()
{
	ULONG res = InterlockedDecrement(&m_refCount);
	if (res == 0)
		delete this;

	return res;
}

HRESULT STDMETHODCALLTYPE CAudioVolumeNotifier::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == __uuidof(IUnknown) || riid == __uuidof(IAudioEndpointVolumeCallback))
	{
		*ppv = static_cast<IAudioEndpointVolumeCallback*>(this);
		AddRef();
		return S_OK;
	}

	*ppv = nullptr;
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CAudioVolumeNotifier::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
{
	if (pNotify)
	{
		float vol = pNotify->fMasterVolume;
		BOOL muted = pNotify->bMuted;
		if (m_pAudioDevice)
			m_pAudioDevice->OnDeviceVolumeChanged(int(vol * 100.0f), muted ? true : false);
	}

	return S_OK;
}

CWasapiAudioDevice::CWasapiAudioDevice(const std::string& deviceId, WasapiMode mode)
{
	InitEmptyAudioFormat(&m_curDeviceFmt);
	m_wasapiMode = mode;
	m_deviceId = deviceId;
	m_hEvent = NULL;
	m_pVolNotifier = nullptr;
	InitWasapiStream(&m_stream);

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
		throw std::runtime_error("Wasapi fail to initialize com env!");

	CScopeGuard guard_comenv([]() { CoUninitialize(); });
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_pDevEnumerator);
	if (FAILED(hr))
	{
		throw std::runtime_error("Wasapi fail to create DeviceEnumerator object!");
	}

	guard_comenv.Dismiss(); 
}

CWasapiAudioDevice::~CWasapiAudioDevice()
{
	Close();
	if (m_pDevEnumerator)
	{
		m_pDevEnumerator.Release();
	}

	CoUninitialize();
}

void CWasapiAudioDevice::OnDeviceVolumeChanged(int vol, bool bMute)
{
	DeviceVolumeNotifier& notifier = GetVolumeNotifier();
	if (notifier)
	{
		notifier(vol, bMute);
	}
}

static void ThrowRtAudioException(const std::string& info, int deviceId)
{
	std::stringstream ss;
	ss << info << ", device: " << deviceId << "!";

	throw std::runtime_error(ss.str());
}

static void ThrowRtAudioException(const std::string& info, HRESULT hr)
{
	std::stringstream ss;
	ss << info << ", HRESULT: 0x" << std::hex << hr << "!";

	throw std::runtime_error(ss.str());
}

static void ThrowRtAudioException(const std::string& info, AudioDataFormat fmt)
{
	std::stringstream ss;
	ss << info << ", audio format is: " << static_cast<int>(fmt) << "!";

	throw std::runtime_error(ss.str());
}

AudioFormat CWasapiAudioDevice::NegotiateFormat(const AudioFormat& audioFmt)
{
	assert(m_pDevEnumerator != nullptr);

	AudioFormat negoFmt;
	InitAudioFormat(&negoFmt, AudioDataFormat::Float32, 2, 48000);

	ComPtr<IMMDevice> pDevice;
	HRESULT hr = GetAudioDeviceObject(m_deviceId, false, pDevice);
	if (SUCCEEDED(hr))
	{
		ComPtr<IAudioClient> pAudioClient;
		hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
		if (SUCCEEDED(hr))
		{
			if (m_wasapiMode == WasapiMode::Share)
			{
				//InitAudioFormat(&negoFmt, AudioDataFormat::PCM_S24, 2, 48000); 
#if 1 
				WAVEFORMATEX* mixFormat = nullptr;
				hr = pAudioClient->GetMixFormat(&mixFormat);
				if (SUCCEEDED(hr))
				{
					CScopeGuard guard_mix([&mixFormat]() { if (mixFormat) CoTaskMemFree(mixFormat); });
					FillAudioFormat(negoFmt, mixFormat);
				}
#endif
			}
			else
			{
				uint32_t channels, chLayout;
				QueryAudioDeviceChannelInfo(pDevice.Get(), channels, chLayout);
				negoFmt = GetFirstAvailableFormats(pAudioClient.Get(), true, channels, 192000);
			}
		}
	}

	return negoFmt;
}

const AudioDeviceInfo& CWasapiAudioDevice::GetDeviceInfo() const
{
	assert(m_pDevEnumerator != nullptr);

	if (!IsOpened())
	{
		static AudioDeviceInfo dummyInfo;

		return dummyInfo;
	}

	return m_info;
}

bool CWasapiAudioDevice::GetVolume(int* vol, bool* pBMute)
{
	*vol = 0;
	if (!IsOpened())
	{
		ComPtr<IMMDevice> pDevice;
		HRESULT hr = GetAudioDeviceObject(m_deviceId, false, pDevice);
		if (SUCCEEDED(hr))
		{
			GetDeviceVolume(pDevice.Get(), vol, pBMute);
		}
	}
	else
	{
		GetDeviceVolume(m_pDevice.Get(), vol, pBMute);
	}

	return true;
}

bool CWasapiAudioDevice::SetVolume(int vol, bool bMute)
{
	bool bSuccess = false;

	if (!IsOpened())
	{
		ComPtr<IMMDevice> pDevice;
		HRESULT hr = GetAudioDeviceObject(m_deviceId, false, pDevice);
		if (SUCCEEDED(hr))
		{
			hr = SetDeviceVolume(pDevice.Get(), vol, bMute);
			bSuccess = SUCCEEDED(hr);
		}
	}
	else
	{
		HRESULT hr = SetDeviceVolume(m_pDevice.Get(), vol, bMute);
		bSuccess = SUCCEEDED(hr);
	}

	return bSuccess;
}

bool CWasapiAudioDevice::IsOpened() const
{
	if (!m_pDevice || !m_pAudioClient)
		return false;

	return (m_stream.state != WasapiState::CLOSED);
}

bool CWasapiAudioDevice::IsRunning() const
{
	if (!m_pDevice || !m_pAudioClient)
		return false;
	
	return (m_stream.state == WasapiState::RUNNING) || (m_stream.state == WasapiState::STOPPING);
}

void CWasapiAudioDevice::Open(const AudioFormat* audioFmt, uint32_t option, const std::string& deviceId)
{
	assert(m_pDevEnumerator != nullptr);

	Close();

	m_deviceId = deviceId; //
	HRESULT hr = GetAudioDeviceObject(deviceId, true, m_pDevice);
	if (FAILED(hr))
		ThrowRtAudioException("Wasapi fail to get audio device", hr);

	hr = m_pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_pAudioClient);
	if (FAILED(hr))
		ThrowRtAudioException("Wasapi fail to active audio client object", hr);

	MakeupDeviceInfo(m_pDevice.Get(), m_info);
	hr = StartDeviceVolumeNotifirt(m_pDevice.Get());
	if (FAILED(hr))
		ThrowRtAudioException("Wasapi fail to start volume notifier", hr);

	CScopeGuard guard_ntf([pDevice = m_pDevice.Get(), this]() { if (pDevice) StopDeviceVolumeNotifirt(pDevice); });

	if (m_wasapiMode == WasapiMode::Share)
	{
		InitShareAudioClient(audioFmt, option);
	}
	else
	{
		InitExclusiveAudioClient(audioFmt, option);
	}

	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pAudioClient->SetEventHandle(m_hEvent);

	hr = m_pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_pAudioRender);
	if (FAILED(hr))
		ThrowRtAudioException("Wasapi fail to get audio render object", hr);

	if ((option & DEVICE_OPTION_REALTIME_PRIORITY) == DEVICE_OPTION_REALTIME_PRIORITY)
		m_stream.priority = 15;
	else
		m_stream.priority = 0;
	if ((option & DEVICE_OPTION_NONINTERLEAVED) == DEVICE_OPTION_NONINTERLEAVED)
		m_stream.userInterleaved = false;
	else
		m_stream.userInterleaved = true;

	m_info.userBufFrames = BufferFramesByRate(m_curDeviceFmt.sampleRate);
	m_stream.userBufSize = m_info.userBufFrames * m_curDeviceFmt.blockAlign;
	m_stream.userBuf = std::make_unique<uint8_t[]>(m_stream.userBufSize);
	m_stream.userBufpos = m_stream.userBufSize; 
	
	hr = m_pAudioClient->Reset();
	if (FAILED(hr))
		ThrowRtAudioException("Wasapi unable to reset render stream", hr);

#if 0
	hr = m_pAudioClient->Start();
	if (FAILED(hr))
		ThrowRtAudioException("Wasapi fail to start audio device stream", hr);
#endif
	guard_ntf.Dismiss(); 
	m_stream.state = WasapiState::STOPPED;
	//IAudioClock
}

void CWasapiAudioDevice::Close()
{
	if (IsOpened())
	{
		if (IsRunning())
		{
			Stop();
		}

		if (m_pAudioRender)
			m_pAudioRender.Release();
		if (m_pAudioClient)
			m_pAudioClient.Release();
		if (m_pDevice)
		{
			StopDeviceVolumeNotifirt(m_pDevice.Get());
			m_pDevice.Release();
		}

		if (m_hEvent != NULL)
		{
			CloseHandle(m_hEvent);
			m_hEvent = NULL;
		}

		ResetWasapiStream(&m_stream);
		m_deviceId.clear();
		m_stream.state = WasapiState::CLOSED;
	}
}

void CWasapiAudioDevice::Start()
{
	if (m_pAudioClient != nullptr)
	{
		std::lock_guard guard(m_stream.mutex);
		if (m_stream.state != WasapiState::STOPPED)
		{
			return;
			/*
			if (m_stream.state == WasapiState::RUNNING)
				throw std::runtime_error("Audio device stream is running!");
			else if (m_stream.state == WasapiState::STOPPING || m_stream.state == WasapiState::CLOSED)
				throw std::runtime_error("Audio device stream is stopping or closed!");
			*/
		}

		m_stream.state = WasapiState::RUNNING;

		// create WASAPI stream thread
		m_stream.hThread = CreateThread(NULL, 0, CWasapiAudioDevice::WasapiThread, this, CREATE_SUSPENDED, NULL);
		if (!m_stream.hThread) 
		{
			throw std::runtime_error("Unable to instantiate callback thread!");
		}
		else 
		{
			SetThreadPriority(m_stream.hThread, m_stream.priority);
			ResumeThread(m_stream.hThread);
		}
	}
}

void CWasapiAudioDevice::Stop()
{
	if (m_pAudioClient != nullptr)
	{
		std::lock_guard guard(m_stream.mutex);
		if ((m_stream.state != WasapiState::RUNNING) && (m_stream.state != WasapiState::STOPPING))
		{
			return;
			/*
			if (m_stream.state == WasapiState::STOPPED)
				throw std::runtime_error("Audio device stream is already stopped!");
			else if (m_stream.state == WasapiState::CLOSED)
				throw std::runtime_error("Audio device stream is closed!");
		    */
		}

		// inform stream thread by setting stream state to STREAM_STOPPING
		m_stream.state = WasapiState::STOPPING;
		//SetEvent(m_hEvent); 
		WaitForSingleObject(m_stream.hThread, INFINITE);
		m_stream.userBufpos = m_stream.userBufSize;
	}
}

void CWasapiAudioDevice::Pause()
{
	if (m_pAudioClient != nullptr)
	{
		std::lock_guard guard(m_stream.mutex);
		if ((m_stream.state != WasapiState::RUNNING) && (m_stream.state != WasapiState::STOPPING))
		{
			return;
			/*
			if (m_stream.state == WasapiState::STOPPED)
				throw std::runtime_error("Audio device stream is already stopped!");
			else if (m_stream.state == WasapiState::CLOSED)
				throw std::runtime_error("Audio device stream is closed!");
			*/
		}

		// inform stream thread by setting stream state to STREAM_STOPPING
		m_stream.state = WasapiState::STOPPING;
		//SetEvent(m_hEvent);
		WaitForSingleObject(m_stream.hThread, INFINITE);
	}
}

HRESULT CWasapiAudioDevice::StartDeviceVolumeNotifirt(IMMDevice* pDevice)
{
	assert(m_pVolNotifier == nullptr);
	assert(m_pEndpointVol == nullptr);

	HRESULT hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&m_pEndpointVol);
	if (SUCCEEDED(hr))
	{
		m_pVolNotifier = new CAudioVolumeNotifier(this);
		hr = m_pEndpointVol->RegisterControlChangeNotify(m_pVolNotifier);
		if (SUCCEEDED(hr))
		{
			int Vol = 0;
			bool bBMute = false;
			GetDeviceVolume(pDevice, &Vol, &bBMute);
			OnDeviceVolumeChanged(Vol, bBMute);
		}
	}

	return hr;
}

HRESULT CWasapiAudioDevice::StopDeviceVolumeNotifirt(IMMDevice* pDevice)
{
	assert(m_pVolNotifier != nullptr);
	assert(m_pEndpointVol != nullptr);

	HRESULT hr = S_OK;
	if ((m_pVolNotifier != nullptr) && (m_pEndpointVol != nullptr))
	{
		hr = m_pEndpointVol->UnregisterControlChangeNotify(m_pVolNotifier);
		m_pVolNotifier->Release();
		m_pVolNotifier = nullptr;

		m_pEndpointVol.Release();
	}

	return hr;
}

HRESULT CWasapiAudioDevice::GetDeviceVolume(IMMDevice* pDevice, int* pVol, bool* pBMute)
{
	ComPtr<IAudioEndpointVolume> pEndpointVol;
	HRESULT hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pEndpointVol);
	if (SUCCEEDED(hr))
	{
		float leveldb = 0;
		pEndpointVol->GetMasterVolumeLevelScalar(&leveldb);
		*pVol = int(leveldb * 100);

		if (pBMute)
		{
			BOOL bMuted = FALSE;
			pEndpointVol->GetMute(&bMuted);
			*pBMute = bMuted ? true : false;
		}
	}

	return hr;
}

HRESULT CWasapiAudioDevice::SetDeviceVolume(IMMDevice* pDevice, int vol, bool bMute)
{
	ComPtr<IAudioEndpointVolume> pEndpointVol;
	HRESULT hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pEndpointVol);
	if (SUCCEEDED(hr))
	{
		float fLevelScalar = float(vol) / 100.0f;
		hr = pEndpointVol->SetMasterVolumeLevelScalar(fLevelScalar, nullptr);
		if (SUCCEEDED(hr))
		{
			hr = pEndpointVol->SetMute(bMute, nullptr);
		}
	}

	return hr;
}

void CWasapiAudioDevice::MakeupDeviceInfo(IMMDevice* pDevice, AudioDeviceInfo& info)
{
	AudioDeviceMgmtInfo mgmtinfo{};
	bool bShare = (m_wasapiMode == WasapiMode::Share);
	HRESULT hr = QueryAudioDeviceInfo(pDevice, bShare, mgmtinfo);
	if (SUCCEEDED(hr))
	{
		info.id = mgmtinfo.strDeviceId;
		info.name = Utf16ToUtf8(mgmtinfo.strFriendlyName);
		info.channels = mgmtinfo.nChannels;
		info.chLayout = mgmtinfo.chLayout;
		info.isOutput = true;
		info.isDefault = false;

		std::string defDeviceID;
		hr = QueryDefaultDeviceID(m_pDevEnumerator.Get(), defDeviceID);
		if (SUCCEEDED(hr))
		{
			info.isDefault = (defDeviceID == info.id);
		}
	}
}

HRESULT CWasapiAudioDevice::GetAudioDeviceObject(const std::string& deviceId, bool mustActive, ComPtr<IMMDevice>& pDevice)
{
	if (!deviceId.empty())
	{
		std::wstring realId = UTtf8ToUtf16Le(deviceId);
		HRESULT hr = m_pDevEnumerator->GetDevice(realId.c_str(), &pDevice);
		if (FAILED(hr))
			return hr;

		if (!mustActive || (mustActive && IsActiveAudioDevice(pDevice.Get())))
			return S_OK;
		
		pDevice.Release(); 
	}

	return m_pDevEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
}


#if 0
int framesPerBuffer = 1024;
int sampleRate = 44100;
REFERENCE_TIME bufferDuration =
(REFERENCE_TIME)((double)framesPerBuffer / sampleRate * 10000000);
#endif

void CWasapiAudioDevice::InitShareAudioClient(const AudioFormat* audioFmt, uint32_t option)
{
	assert(m_pAudioClient != nullptr);

	//m_bufferDuration = 10000000 / 8; // 1/8s
	m_bufferDuration = 100 * 10000; 

	WAVEFORMATEXTENSIBLE wavFmt;
	FillWaveFormat(&wavFmt, audioFmt);

	WAVEFORMATEX* pClosestMatch = NULL;
	HRESULT hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (const WAVEFORMATEX*)&wavFmt, &pClosestMatch);
	CScopeGuard guard_match([&pClosestMatch]() { if (pClosestMatch) CoTaskMemFree(pClosestMatch); });

	if (hr == S_OK)
	{
		FillAudioFormat(m_curDeviceFmt, (const WAVEFORMATEX*)&wavFmt);
		hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			m_bufferDuration, 0, (const WAVEFORMATEX*)&wavFmt, nullptr);
		if (FAILED(hr))
			throw std::runtime_error("Wasapi fail to active audio client object!");
	}
	else if (hr == S_FALSE)
	{
		FillAudioFormat(m_curDeviceFmt, pClosestMatch);
		hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			m_bufferDuration, 0, pClosestMatch, nullptr);
		if (FAILED(hr))
			throw std::runtime_error("Wasapi fail to active audio client object!");
	}
	else
		throw std::runtime_error("Wasapi share mode not support this audio format!");
}

// Makes Hns period from frames and sample rate
static REFERENCE_TIME MakeHnsPeriod(UINT32 nFrames, DWORD nSamplesPerSec)
{
	return (REFERENCE_TIME)((10000.0 * 1000 / nSamplesPerSec * nFrames) + 0.5);
}

void CWasapiAudioDevice::InitExclusiveAudioClient(const AudioFormat* audioFmt, uint32_t option)
{
	assert(m_pAudioClient != nullptr);

	//m_bufferDuration = hnsPeriod * 4;  // minPeriod * 4

	REFERENCE_TIME default_period, min_period;
	m_pAudioClient->GetDevicePeriod(&default_period, &min_period);
	m_bufferDuration = min_period * 4;

	WAVEFORMATEXTENSIBLE wavFmt{};
	FillWaveFormat(&wavFmt, audioFmt);

	HRESULT hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (const WAVEFORMATEX*)&wavFmt, NULL);
	//hr2 = AUDCLNT_E_UNSUPPORTED_FORMAT;
	if (hr == S_OK)
	{

		FillAudioFormat(m_curDeviceFmt, (const WAVEFORMATEX*)&wavFmt);
		hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			m_bufferDuration, m_bufferDuration, (const WAVEFORMATEX*)&wavFmt, nullptr);
		if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
		{
			// Align the buffer if needed, see IAudioClient::Initialize() documentation
			UINT32 alignedBufferFrames = 0;
			m_pAudioClient->GetBufferSize(&alignedBufferFrames);
			m_bufferDuration = (REFERENCE_TIME)((double)alignedBufferFrames * REFTIMES_PER_SEC / wavFmt.Format.nSamplesPerSec);
			//m_bufferDuration = (REFERENCE_TIME)((double)REFTIMES_PER_SEC / wavFmt.Format.nSamplesPerSec * nFrames + 0.5);
			hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
				m_bufferDuration, m_bufferDuration, (const WAVEFORMATEX*)&wavFmt, nullptr);
		}
		if (FAILED(hr))
			throw std::runtime_error("Wasapi fail to active audio client object!");
	}
	else
		throw std::runtime_error("Wasapi exclusive mode not support this audio format!");
}

void CWasapiAudioDevice::InitWasapiStream(WasapiStream* pStream)
{
	pStream->hThread = NULL;
	pStream->state = WasapiState::CLOSED;
	pStream->userInterleaved = true;
	pStream->priority = 0;
	pStream->userBufSize = 0;
	pStream->userBufpos = 0;
}

void CWasapiAudioDevice::ResetWasapiStream(WasapiStream* pStream)
{
	pStream->hThread = NULL;
	pStream->state = WasapiState::CLOSED;
	pStream->userInterleaved = true;
	pStream->priority = 0;
	pStream->userBuf.reset();
	pStream->userBufSize = 0;
	pStream->userBufpos = 0;
}

void CWasapiAudioDevice::LoadStreamBuffer(BYTE* streamBuffer, uint32_t bufferSize)
{
	DeviceCallback& callback = GetCallback();
	uint32_t frameSize = m_curDeviceFmt.blockAlign;

	uint32_t remain = m_stream.userBufSize - m_stream.userBufpos;
	if (remain >= bufferSize)
	{
		std::memcpy(streamBuffer, m_stream.userBuf.get() + m_stream.userBufpos, bufferSize);
		m_stream.userBufpos += bufferSize;
	}
	else
	{
		uint32_t subRemain = bufferSize - remain;
		std::memcpy(streamBuffer, m_stream.userBuf.get() + m_stream.userBufpos, remain);
		callback(m_stream.userBuf.get(), m_stream.userBufSize / frameSize, 0.0, 0);
		m_stream.userBufpos = 0;
		std::memcpy(streamBuffer + remain, m_stream.userBuf.get(), subRemain);
		m_stream.userBufpos = subRemain;
	}
}

/*
For an exclusive-mode rendering or capture stream that was initialized with the AUDCLNT_STREAMFLAGS_EVENTCALLBACK flag, 
the client typically has no use for the padding value reported by GetCurrentPadding. Instead, the client accesses an 
entire buffer during each processing pass. Each time a buffer becomes available for processing, the audio engine notifies 
the client by signaling the client's event handle.

*/
void CWasapiAudioDevice::DoWasapiThread()
{
	HRESULT hr = S_OK;
	DeviceCallback& callback = GetCallback();
	uint32_t frameSize = m_curDeviceFmt.blockAlign;
	BYTE* streamBuffer = nullptr;
	uint32_t bufferSize = 0;
	REFERENCE_TIME defaultPeriod = 0;
	REFERENCE_TIME minPeriod = 0;
	uint32_t periodFrames = 1024;

	// Get render buffer from stream
	uint32_t bufferFrameCount = 0;
	hr = m_pAudioClient->GetBufferSize(&bufferFrameCount);
	if (FAILED(hr))
		goto Exit;
	hr = m_pAudioClient->GetDevicePeriod(&defaultPeriod, &minPeriod);
	if (FAILED(hr))
		goto Exit;

	periodFrames = (uint32_t)((minPeriod * m_curDeviceFmt.sampleRate) / REFTIMES_PER_SEC);
	
	hr = m_pAudioRender->GetBuffer(bufferFrameCount, &streamBuffer);
	if (FAILED(hr))
		goto Exit;

	bufferSize = bufferFrameCount * frameSize;
	LoadStreamBuffer(streamBuffer, bufferSize);

	hr = m_pAudioRender->ReleaseBuffer(bufferFrameCount, 0);
	if (FAILED(hr))
		goto Exit;

	hr = m_pAudioClient->Start();
	if (FAILED(hr))
		goto Exit;

	while (m_stream.state != WasapiState::STOPPING)
	{
		DWORD retval = WaitForSingleObject(m_hEvent, 2000); //INFINITE
		if (retval != WAIT_OBJECT_0) // Event handle timed out after a 2-second wait.
			goto Exit;

		uint32_t bufferFrames = bufferFrameCount;
		if (m_wasapiMode == WasapiMode::Share)
		{
			uint32_t numFramesPadding = 0;
			hr = m_pAudioClient->GetCurrentPadding(&numFramesPadding);
			if (FAILED(hr))
				goto Exit;

			bufferFrames -= numFramesPadding;
		}

		//UINT32 framesToWrite = std::min(avail, periodFrames);
		if (bufferFrames != 0)
		{
			hr = m_pAudioRender->GetBuffer(bufferFrames, &streamBuffer);
			if (FAILED(hr))
				goto Exit;

			bufferSize = bufferFrames * frameSize;
			LoadStreamBuffer(streamBuffer, bufferSize);

			// Release render buffer
			hr = m_pAudioRender->ReleaseBuffer(bufferFrames, 0);
			if (FAILED(hr))
				goto Exit;
		}
		else
		{
			// Inform WASAPI that render was unsuccessful
			hr = m_pAudioRender->ReleaseBuffer(0, 0);
			if (FAILED(hr))
				goto Exit;
		}
	}

	// Wait for the last buffer to play before stopping.
	Sleep((DWORD)(m_bufferDuration / REFTIMES_PER_MILLISEC));
Exit:
	m_pAudioClient->Stop();
	// close thread handle
	CloseHandle(m_stream.hThread);
	m_stream.hThread = NULL;
	m_stream.state = WasapiState::STOPPED;
}

// Function pointers
typedef BOOL(WINAPI* AvRevertMmThreadCharacteristicsFn)(HANDLE);
typedef HANDLE(WINAPI* AvSetMmThreadCharacteristicsFn)(LPCWSTR, LPDWORD);


DWORD WINAPI CWasapiAudioDevice::WasapiThread(void* wasapiPtr)
{
	// as this is a new thread, we must CoInitialize it
	::CoInitialize(NULL);

	if (wasapiPtr)
	{
		// Attempt to assign "Pro Audio" characteristic to thread
		HANDLE hMmThread = NULL;
		HMODULE AvrtDll = LoadLibraryW(L"AVRT.dll");
		if (AvrtDll) 
		{
			DWORD taskIndex = 0;
			AvSetMmThreadCharacteristicsFn AvSetMmThreadCharacteristicsPtr =
				(AvSetMmThreadCharacteristicsFn)GetProcAddress(AvrtDll, "AvSetMmThreadCharacteristicsW");
			hMmThread = AvSetMmThreadCharacteristicsPtr(L"Pro Audio", &taskIndex);
			FreeLibrary(AvrtDll);
		}
		CScopeGuard guard_mmHandl([&hMmThread]() {
			if (hMmThread != NULL) {
				HMODULE AvrtDll = LoadLibraryW(L"AVRT.dll");
				if (AvrtDll)
				{
					AvRevertMmThreadCharacteristicsFn AvRevertMmThreadCharacteristicsPtr =
						(AvRevertMmThreadCharacteristicsFn)GetProcAddress(AvrtDll, "AvRevertMmThreadCharacteristics");
					AvRevertMmThreadCharacteristicsPtr(hMmThread);
					FreeLibrary(AvrtDll);
				}
			}
		});

		CWasapiAudioDevice* thisDevice = (CWasapiAudioDevice*)wasapiPtr;
		thisDevice->DoWasapiThread();
	}

	::CoUninitialize();

	return 0;
}




