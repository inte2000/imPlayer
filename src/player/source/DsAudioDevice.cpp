/*
20250522 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#include <cassert>
#include <algorithm>
#include <sstream>
#include "UnicodeConvert.h"
#include "DsAudioDevice.h"
#include "DeviceInfoWin.h"
#include "ScopeGuard.h"

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

extern uint32_t BufferFramesByRate(uint32_t samplerate);

static std::string GuidToString(const GUID& guid)
{
	char buf[64] = {};
	snprintf(buf, sizeof(buf),
		"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	return buf;
}

static bool StringToGuid(const std::string& guidStr, GUID& outGuid)
{
	if (guidStr.empty())
		return false;

	std::wstring wstrGuid = UTtf8ToUtf16Le(guidStr);

	HRESULT hr = CLSIDFromString(wstrGuid.c_str(), &outGuid);
	return SUCCEEDED(hr);
}

static uint32_t MapDsLayoutToStandLayout(DWORD layout)
{
	uint32_t chLayout = 0;

	switch (layout)
	{
	case DSSPEAKER_MONO: chLayout = 0x01; break;
	case DSSPEAKER_QUAD: chLayout = 0x33; break;
	case DSSPEAKER_STEREO: chLayout = 0x03; break;
	case DSSPEAKER_5POINT1: chLayout = 0x3F; break;
	case DSSPEAKER_7POINT1: chLayout = 0xFF; break;
	case DSSPEAKER_5POINT1_SURROUND: chLayout = 0x60F; break;
	case DSSPEAKER_7POINT1_SURROUND: chLayout = 0x63F; break;
	default: chLayout = 0x03; break;
	}

	return chLayout;
}

static void ThrowRtAudioException(const std::string& info, const std::string& deviceId)
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

CDsAudioDevice::CDsAudioDevice(const std::string& deviceId)
{
	InitEmptyAudioFormat(&m_curDeviceFmt);
	m_deviceId = deviceId;
	//m_hEvent = NULL;
	InitDsoundStream(&m_stream);
}

CDsAudioDevice::~CDsAudioDevice()
{
	Close();
}

void CDsAudioDevice::OnDeviceVolumeChanged(int vol, bool bMute)
{
}

AudioFormat CDsAudioDevice::NegotiateFormat(const AudioFormat& audioFmt)
{
	assert(m_ds != nullptr);

	AudioFormat negoFmt;
	//ʹ����Ƶ���Ե��������Ͳ����ʣ��������ݸ�ʽʹ�� 32 λ������
	//InitAudioFormat(&negoFmt, AudioDataFormat::Float32, audioFmt.numChannels, audioFmt.sampleRate);
	InitAudioFormat(&negoFmt, AudioDataFormat::Float32, 2, 48000);

	DWORD cfg = 0;
	//����鵽���ȵ����ã��ô�����
	HRESULT hr = m_ds->GetSpeakerConfig(&cfg);
	if (SUCCEEDED(hr))
	{
		DWORD layout = DSSPEAKER_CONFIG(cfg);
		DWORD flags = DSSPEAKER_GEOMETRY(cfg);
		negoFmt.chLayout = MapDsLayoutToStandLayout(layout);
		negoFmt.numChannels = std::popcount(negoFmt.chLayout);
		negoFmt.blockAlign = negoFmt.blockAlign;
	}

	return negoFmt;
}

const AudioDeviceInfo& CDsAudioDevice::GetDeviceInfo() const
{
	//�Ѿ� open ���豸���� open ��ʱ��������ϢΪ׼��û�� open ���豸����ָ���� deviceId ���? m_info
	if (!IsOpened())
	{
		static AudioDeviceInfo dummyInfo;

		return dummyInfo;
	}

	return m_info;
}

bool CDsAudioDevice::GetVolume(int* vol, bool* pBMute)
{
	int volume = 0;
	if (IsOpened())
	{
		LONG volume = 0;
		m_secondary->GetVolume(&volume);
		// ��DSBVOLUME_MIN(-10000)��DSBVOLUME_MAX(0)ת��Ϊ0.0��1.0
		*vol = (int)(100.0f * (volume - DSBVOLUME_MIN) / static_cast<float>(DSBVOLUME_MAX - DSBVOLUME_MIN));
		if (pBMute)
			*pBMute = (volume == DSBVOLUME_MIN);

		return true;
	}

	return false;
}

bool CDsAudioDevice::SetVolume(int vol, bool bMute)
{
	if (IsOpened())
	{
		LONG volume = 0;
		if (bMute)
			volume = DSBVOLUME_MIN;
		else
		{
			float fVol = vol / 100.0f;
			fVol = std::clamp(fVol, 0.0f, 1.0f);
			volume = static_cast<LONG>(DSBVOLUME_MIN + fVol * (DSBVOLUME_MAX - DSBVOLUME_MIN));
		}

		return SUCCEEDED(m_secondary->SetVolume(volume));
	}

	return false;
}

bool CDsAudioDevice::IsOpened() const
{
	if (!m_secondary || !m_ds)
		return false;

	return (m_stream.state != DirectSoundState::CLOSED);
}

bool CDsAudioDevice::IsRunning() const
{
	if (!m_secondary || !m_ds)
		return false;
	
	return (m_stream.state == DirectSoundState::RUNNING) || (m_stream.state == DirectSoundState::STOPPING);
}

void CDsAudioDevice::Open(const AudioFormat* audioFmt, uint32_t option, const std::string& deviceId)
{
	Close();

	m_deviceId = deviceId; //
	m_curDeviceFmt = *audioFmt;

	// Primary buffer
	DSBUFFERDESC pdesc{};
	pdesc.dwSize = sizeof(pdesc);
	pdesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	pdesc.dwBufferBytes = 0;
	pdesc.lpwfxFormat = nullptr;

	HRESULT hr = S_OK;
	if (m_deviceId.empty()) //Ĭ���豸
	{
		hr = DirectSoundCreate8(nullptr, &m_ds, nullptr);
	}
	else
	{
		GUID deviceGuid = { 0 };
		auto posFind = m_deviceId.rfind(L'.', -1);
		if (posFind != std::wstring::npos)
			m_deviceId = m_deviceId.substr(posFind + 1);

		if(!StringToGuid(m_deviceId, deviceGuid))
			ThrowRtAudioException("Fail to translate device id to GUID object", m_deviceId);

		hr = DirectSoundCreate8(&deviceGuid, &m_ds, nullptr);
	}

	if (FAILED(hr))
		ThrowRtAudioException("Fail to create direct sound object", hr);

	m_ds->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

	hr = m_ds->CreateSoundBuffer(&pdesc, &m_primary, nullptr);
	if (FAILED(hr))
		ThrowRtAudioException("Fail to create sound buffer object", hr);

	WAVEFORMATEXTENSIBLE wavFmtEx;
	FillWaveFormat(&wavFmtEx, &m_curDeviceFmt);

	hr = m_primary->SetFormat(reinterpret_cast<WAVEFORMATEX*>(&wavFmtEx));
	//hr = m_primary->SetFormat(&wfx);
	if (FAILED(hr))
		ThrowRtAudioException("Fail to set sound buffer format", hr);

	// Secondary buffer, ÿ�� buf 20ms ���ң�׼������ buffer
	m_stream.dsBufSize = (m_curDeviceFmt.blockAlign * m_curDeviceFmt.sampleRate) / 50; // 20ms buffer;
	m_stream.dsBufCount = 3;

	DSBUFFERDESC desc{};
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	desc.dwBufferBytes = m_stream.dsBufSize * m_stream.dsBufCount;
	desc.lpwfxFormat = reinterpret_cast<WAVEFORMATEX*>(&wavFmtEx);

	hr = m_ds->CreateSoundBuffer(&desc, &m_secondary, nullptr);
	if (FAILED(hr))
		ThrowRtAudioException("Fail to create direct sound buffer object", hr);

#if 0
	ComPtr<IDirectSoundBuffer> temp;
	hr = m_ds->CreateSoundBuffer(&desc, &temp, nullptr);
	if (FAILED(hr))
		ThrowRtAudioException("Fail to create direct sound buffer object", hr);

	hr = temp->QueryInterface(IID_IDirectSoundBuffer8, (void**)&m_buffer);
	if (FAILED(hr))
		ThrowRtAudioException("Fail to create direct sound buffer object", hr);
	temp.Release();
#endif

/*
	hr = temp.As(&m_buffer);
	if (FAILED(hr))
		ThrowRtAudioException("Fail to create direct sound buffer object", hr);
*/
	QueryDeviceInfo(m_info);
	m_info.userBufFrames = BufferFramesByRate(m_curDeviceFmt.sampleRate);
	m_stream.userBufSize = m_info.userBufFrames * m_curDeviceFmt.blockAlign;
	m_stream.userBuf = std::make_unique<uint8_t[]>(m_stream.userBufSize);
	m_stream.userBufpos = m_stream.userBufSize; //��ʾ��ǰû�п�������
	m_stream.state = DirectSoundState::STOPPED;
}

void CDsAudioDevice::Close()
{
	if (IsOpened())
	{
		if (IsRunning())
		{
			Stop();
		}

		m_secondary.Release();
		m_primary.Release();
		m_ds.Release();

		ResetDsoundStream(&m_stream);
		m_deviceId.clear();
		m_stream.state = DirectSoundState::CLOSED;
	}
}

void CDsAudioDevice::Start()
{
	if (m_secondary != nullptr)
	{
		std::lock_guard guard(m_stream.mutex);
		if (m_stream.state != DirectSoundState::STOPPED)
		{
			return;
			/*
			if (m_stream.state == WasapiState::RUNNING)
				throw std::runtime_error("Audio device stream is running!");
			else if (m_stream.state == WasapiState::STOPPING || m_stream.state == WasapiState::CLOSED)
				throw std::runtime_error("Audio device stream is stopping or closed!");
			*/
		}

		m_stream.state = DirectSoundState::RUNNING;

		// create WASAPI stream thread
		m_stream.hThread = CreateThread(NULL, 0, CDsAudioDevice::DirectSoundThread, this, CREATE_SUSPENDED, NULL);
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

void CDsAudioDevice::Stop()
{
	if (m_secondary != nullptr)
	{
		std::lock_guard guard(m_stream.mutex);
		if ((m_stream.state != DirectSoundState::RUNNING) && (m_stream.state != DirectSoundState::STOPPING))
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
		m_stream.state = DirectSoundState::STOPPING;
		WaitForSingleObject(m_stream.hThread, INFINITE);
		m_stream.userBufpos = m_stream.userBufSize;
	}
}

void CDsAudioDevice::Pause()
{
	if (m_secondary != nullptr)
	{
		std::lock_guard guard(m_stream.mutex);
		if ((m_stream.state != DirectSoundState::RUNNING) && (m_stream.state != DirectSoundState::STOPPING))
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
		m_stream.state = DirectSoundState::STOPPING;
		WaitForSingleObject(m_stream.hThread, INFINITE);
	}
}

void CDsAudioDevice::InitDsoundStream(DSoundStream* pStream)
{
	pStream->hThread = NULL;
	pStream->state = DirectSoundState::CLOSED;
	pStream->userInterleaved = true;
	pStream->priority = 0;
	pStream->userBufSize = 0;
	pStream->userBufpos = 0;
	pStream->dsBufSize = 0;
	pStream->dsBufCount = 0;
}

void CDsAudioDevice::ResetDsoundStream(DSoundStream* pStream)
{
	pStream->hThread = NULL;
	pStream->state = DirectSoundState::CLOSED;
	pStream->userInterleaved = true;
	pStream->priority = 0;
	pStream->userBuf.reset();
	pStream->userBufSize = 0;
	pStream->userBufpos = 0;
	pStream->dsBufSize = 0;
	pStream->dsBufCount = 0;
}

BOOL CALLBACK DSoundEnumCallbackA(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR /*lpcstrModule*/,	LPVOID lpContext)
{
	auto* devices =	reinterpret_cast<std::vector<AudioDeviceInfo>*>(lpContext);

	AudioDeviceInfo info;
	info.isOutput = true;

	if (lpGuid)
	{
		info.id = GuidToString(*lpGuid);
		info.isDefault = false;
	}
	else
	{
		// lpGuid == nullptr Լ��Ϊ��Ĭ���豸��
		info.id = "";
		info.isDefault = true;
	}

	info.name = LocalMBCSToUtf8(lpcstrDescription ? lpcstrDescription : "Unknown");

	devices->push_back(std::move(info));
	return TRUE; // ����ö��
}

void CDsAudioDevice::QueryDeviceInfo(AudioDeviceInfo& info)
{
	std::vector<AudioDeviceInfo> devices;
	HRESULT hr = DirectSoundEnumerateA(DSoundEnumCallbackA,	&devices);
	if (SUCCEEDED(hr))
	{
		for (const AudioDeviceInfo& i : devices)
		{
			if ((i.isDefault && m_deviceId.empty()) || (i.id == m_deviceId))
			{
				m_info = i;
				break;
			}
		}
	}
}

HRESULT CDsAudioDevice::FillDSoundBuffer(DWORD bufIndex)
{
	DeviceCallback& callback = GetCallback();

	// ���㻺����ƫ��
	DWORD offset = bufIndex * m_stream.dsBufSize;
    DWORD bytes = m_stream.dsBufSize;

	void* p1; DWORD b1;
	void* p2; DWORD b2;

	HRESULT hr = m_secondary->Lock(offset, bytes, &p1, &b1, &p2, &b2, 0);
	if (SUCCEEDED(hr))
	{
		//std::memcpy(p1, temp.data(), b1);
		uint32_t remain = m_stream.userBufSize - m_stream.userBufpos;
		if (remain >= b1)
		{
			std::memcpy(p1, m_stream.userBuf.get() + m_stream.userBufpos, b1);
			m_stream.userBufpos += b1;
		}
		else
		{
			uint32_t subRemain = b1 - remain;
			std::memcpy(p1, m_stream.userBuf.get() + m_stream.userBufpos, remain);
			callback(m_stream.userBuf.get(), m_stream.userBufSize / m_curDeviceFmt.blockAlign, 0.0, 0);
			m_stream.userBufpos = 0;
			std::memcpy((uint8_t *)p1 + remain, m_stream.userBuf.get(), subRemain);
			m_stream.userBufpos = subRemain;
		}
		if (p2)
		{
			//std::memcpy(p2, temp.data() + b1, b2);
			remain = m_stream.userBufSize - m_stream.userBufpos;
			if (remain >= b2)
			{
				std::memcpy(p2, m_stream.userBuf.get() + m_stream.userBufpos, b2);
				m_stream.userBufpos += b2;
			}
			else
			{
				uint32_t subRemain = b2 - remain;
				std::memcpy(p2, m_stream.userBuf.get() + m_stream.userBufpos, remain);
				callback(m_stream.userBuf.get(), m_stream.userBufSize / m_curDeviceFmt.blockAlign, 0.0, 0);
				m_stream.userBufpos = 0;
				std::memcpy((uint8_t*)p2 + remain, m_stream.userBuf.get(), subRemain);
				m_stream.userBufpos = subRemain;
			}
		}

		hr = m_secondary->Unlock(p1, b1, p2, b2);
	}
		
	return hr;
}

void CDsAudioDevice::DoRenderThread()
{
	HRESULT hr = S_OK;
	DWORD curBufIndex = 0;
	
	for (DWORD i = 0; i < m_stream.dsBufCount; ++i)
	{
		hr = FillDSoundBuffer(i);
		if (FAILED(hr))
			goto Exit;
	}

	hr = m_secondary->Play(0, 0, DSBPLAY_LOOPING);
	if (FAILED(hr))
		goto Exit;

	while (m_stream.state != DirectSoundState::STOPPING)
	{
		DWORD playCursor, writeCursor;
		hr = m_secondary->GetCurrentPosition(&playCursor, &writeCursor);
		if (FAILED(hr))
			goto Exit;

		// ���㵱ǰ���ŵĻ���������
		DWORD playedBufIdx = playCursor / m_stream.dsBufSize;
		// ����������Ѿ������˵�ǰ����������������Ҫ����µĻ�����
		if (playedBufIdx != curBufIndex)
		{
			FillDSoundBuffer(curBufIndex);
			curBufIndex = (curBufIndex + 1) % m_stream.dsBufCount;

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	// Wait for the last buffer to play before stopping.
	//std::this_thread::sleep_for(std::chrono::milliseconds(30)); //� buffer �� 60ms

Exit:
	m_secondary->Stop();
	// close thread handle
	CloseHandle(m_stream.hThread);
	m_stream.hThread = NULL;
	m_stream.state = DirectSoundState::STOPPED;
}

// Function pointers
typedef BOOL(WINAPI* AvRevertMmThreadCharacteristicsFn)(HANDLE);
typedef HANDLE(WINAPI* AvSetMmThreadCharacteristicsFn)(LPCWSTR, LPDWORD);

DWORD WINAPI CDsAudioDevice::DirectSoundThread(void* dsPtr)
{
	// as this is a new thread, we must CoInitialize it
	::CoInitialize(NULL);

	if (dsPtr)
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

		CDsAudioDevice* thisDevice = (CDsAudioDevice*)dsPtr;
		thisDevice->DoRenderThread();
	}

	::CoUninitialize();

	return 0;
}



