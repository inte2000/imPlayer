#include "framework.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <algorithm>
#include <chrono>
#include <future>
#include "DeviceInfoWin.h"
#include "UnicodeConvert.h"
#include "Playback.h"

//#define TIMER_ELLASP_TEST
#ifdef TIMER_ELLASP_TEST
#include "Timer.h"
#endif

using namespace std::chrono_literals;


uint32_t BufferFramesByRate(uint32_t samplerate)
{
	uint32_t frames = samplerate / 8;
	frames = (frames + 63) & ~63; 

	if (frames < AUDIO_BUFFER_FRAMES)
		frames = AUDIO_BUFFER_FRAMES;

	return frames;
}

std::shared_ptr<CPlayback> CPlayback::Create(PlaybackCallback *pCallback, std::unique_ptr<CAudioDevice> audioDevice)
{
    //return std::make_shared<CPlayback>();
	std::shared_ptr<CPlayback> playbackPtr = std::shared_ptr<CPlayback>(new CPlayback());
	if (playbackPtr)
	{
		playbackPtr->InitPlayback(pCallback, std::move(audioDevice));
	}

    return playbackPtr;
}

CPlayback::~CPlayback()
{
	CloseAudioDevice();
}

void CPlayback::UpdataPlayback(void* audioBuf, uint32_t frames, std::size_t framesInBuffer)
{
	if (!m_dataSource)
		return;

	std::size_t totalFrame = m_dataSource->GetTotalFrames();
	std::size_t curFrame = m_dataSource->GetCurrentFrame();
	bool lastStream = m_dataSource->IsLastAudioStream();
	
	std::size_t realCurFrame = curFrame;
	if(curFrame >= framesInBuffer)
	    realCurFrame = curFrame - framesInBuffer;

	float curSeconds = m_dataSource->FramesToSeconds(realCurFrame);
	if ((realCurFrame >= totalFrame) && (framesInBuffer == 0))
	{
		if (m_status == PlaybackStatus::Playing)
		{
			bool bNeedStop = true;
			m_audioEndSemaphore.release();
			if (m_pCallback)
			{
				bNeedStop = !m_pCallback->OnAudioEnd(lastStream);
			}
			//m_status = PlaybackStatus::PlayingEnd;
			m_status = lastStream ? PlaybackStatus::PlayingEnd : m_status;

		}
	}
	else
	{
		float powerBand[32];
		if ((m_pCallback) && (m_status == PlaybackStatus::Playing))
			m_pCallback->OnAudioUpdate(curSeconds, powerBand, 0);
	}
}

void CPlayback::MockAudioEndCallback()
{
	if (m_pCallback)
	{
		m_pCallback->OnAudioEnd(true);
	}
}

void CPlayback::NotifyAudioStreamBegin(const CAudioSource* pAudioSource, uint32_t streamIdx)
{
	if (m_pCallback)
	{
		float totalSeconds = m_dataSource->GetTotalSeconds();
		const std::wstring& name = m_dataSource->GetName();
		CMediaTag metaInfo = m_dataSource->GetMetaInformation();
		 
		m_audioEndSemaphore.try_acquire_for(100ns); //清除信号量
		m_pCallback->OnAudioBegin(streamIdx, metaInfo, name, totalSeconds);
	}
}

bool IsSameDeviceId(std::string id1, std::string id2)
{
	auto posFind = id1.rfind(L'.', -1);
	if (posFind != std::wstring::npos)
		id1 = id1.substr(posFind + 1);

	posFind = id2.rfind(L'.', -1);
	if (posFind != std::wstring::npos)
		id2 = id2.substr(posFind + 1);

	return (_stricmp(id1.c_str(), id2.c_str()) == 0);
}

void CPlayback::OnDefaultDeviceChanged(const std::wstring& deviceId)
{
	std::lock_guard guard(m_defDeviceChange);

	std::string utf8deviceId = Utf16ToUtf8(deviceId);
	if (m_deviceId.empty()) 
	{
		const AudioDeviceInfo& devInfo = m_audioDevice->GetDeviceInfo();
		if (!IsSameDeviceId(utf8deviceId, devInfo.id)) 
		{
			bool isOpened = m_audioDevice->IsOpened();
			bool isRunning = m_audioDevice->IsRunning();
			if (isRunning)
				StopCurrentAudioSource(true);
			if (isOpened)
			{
				CloseAudioDevice();

				AudioFormat negoFmt;
				if (!GetAudioDeviceNegoFormat(utf8deviceId, negoFmt))
				{
					return;
				}

				OpenAudioDevice(negoFmt);
			}
			if (isRunning)
				StartCurrentAudioSource();
		}
	}
	else
	{
	}
}

bool CPlayback::SetAudioSource(std::unique_ptr<CAudioSource> ds, bool autostart)
{
	if (ds == nullptr)
	{
		StopCurrentAudioSource(false);
		m_dataSource.reset();
	}
	else
	{
	    StopCurrentAudioSource(true);
		m_dataSource = std::move(ds);

		AudioFormat negoFmt;
		if (!GetAudioDeviceNegoFormat(m_deviceId, negoFmt))
		{
			std::cerr << "Playback: SetAudioSource fail to nego device format!" << std::endl;
			return false;
		}

		if (m_audioDevice->IsOpened())
		{
			const AudioFormat& deviceFmt = m_audioDevice->GetDeviceFormat();
			if (!IsSameAudioFormat(&deviceFmt, &negoFmt))
			{
				CloseAudioDevice();
				OpenAudioDevice(negoFmt);
			}
		}
		else
		{
			OpenAudioDevice(negoFmt);
		}

		if (autostart)
		{
			StartCurrentAudioSource();
		}
	}

	return true;
}

bool CPlayback::SwitchAudioDevice(std::unique_ptr<CAudioDevice> audioDevice)
{
	bool isDeviceOpen = false;
	bool isAudioRunning = false;

	if (m_audioDevice)
	{
		isAudioRunning = (m_status != PlaybackStatus::Stoped);
		StopCurrentAudioSource(true);
		isDeviceOpen = m_audioDevice->IsOpened();
		if (isDeviceOpen)
		{
			CloseAudioDevice();
		}
	}

	SetAudioDevice(std::move(audioDevice));

	if (isDeviceOpen)
	{
		AudioFormat negoFmt;
		if (!GetAudioDeviceNegoFormat(m_deviceId, negoFmt))
		{
			std::cerr << "Playback: Switch Audio Device, fail to reopen new device on negoFmt" << std::endl;
			return false;
		}

		OpenAudioDevice(negoFmt);
	}
	if (isAudioRunning)
	{
		StartCurrentAudioSource();
	}

	return true;
}

bool CPlayback::RestartAudioDevice()
{
	assert(m_audioDevice);

	if (m_audioDevice->IsOpened())
	{
	    bool isAudioRunning = m_audioDevice->IsRunning();
		if(isAudioRunning)
		    StopCurrentAudioSource(true);
		
		CloseAudioDevice();
		
		AudioFormat negoFmt;
		if (!GetAudioDeviceNegoFormat(m_deviceId, negoFmt))
		{
			std::cerr << "Playback: restart audio source, fail to reopen new device on negoFmt" << std::endl;
			return false;
		}

		OpenAudioDevice(negoFmt);
		if (isAudioRunning)
			StartCurrentAudioSource();
	}

	return true;
}

void CPlayback::Shutdown()
{
	StopCurrentAudioSource();
}

std::optional<int> CPlayback::GetCurrentDeviceVolume() 
{ 
	assert(m_audioDevice != nullptr);

	int vol = 0;
	bool bBMute = false;
	if (m_audioDevice->GetVolume(&vol, &bBMute))
	{
		if (bBMute)
			vol = 0;

		return vol;
	}

	return std::nullopt;
}

bool CPlayback::SetCurrentDeviceVolume(int vol) 
{ 
	assert(m_audioDevice != nullptr);

	bool bMute = (vol == 0);
	return m_audioDevice->SetVolume(vol, bMute);
}

void CPlayback::Play()
{
	if (!m_dataSource)
		return;
	
	StartCurrentAudioSource();
}

void CPlayback::Stop()
{
	if (!m_dataSource)
		return;

	StopCurrentAudioSource();
}

void CPlayback::Pause()
{
	if (!m_dataSource)
		return;
	
	if (m_status == PlaybackStatus::Playing)
	{
		m_audioDevice->Pause();
		//m_audioDevice->Stop();
		m_status = PlaybackStatus::Paused;
		if (m_pCallback)
			m_pCallback->OnControlEvent(PlayControl::Pause);
	}
	else if (m_status == PlaybackStatus::Paused)
	{
		StartCurrentAudioSource();
	}
	else
	{
	}
}

void CPlayback::SeekPosition(float seconds_pos)
{
	if (!m_dataSource)
		return;

	if (seconds_pos < 0.0f)
		seconds_pos = 0.0f;
	std::size_t totalframes = m_dataSource->GetTotalFrames();
	/*
	const AudioFormat& audioFmt = m_dataSource->GetAudioFormat();
	std::size_t curframes = static_cast<std::size_t>(audioFmt.sampleRate * seconds_pos);
	*/
	std::size_t curframes = m_dataSource->SecondsToFrames(seconds_pos);
	if (curframes > totalframes)
		curframes = totalframes;
	
	//m_audioBuf.Clear();
	m_dataSource->SeekToFrame(curframes);
}

float CPlayback::GetCurrentPosition() const
{
	if (!m_dataSource)
		return 0.0;

	std::size_t curframes = m_dataSource->GetCurrentFrame();
	std::size_t framesInBuffer = m_audioBuf.GetAvailableRead();
	if (curframes >= framesInBuffer)
		curframes -= framesInBuffer;
	else
		curframes = 0;

	return m_dataSource->FramesToSeconds(curframes);
}

void CPlayback::OnDeviceVolumeChanged(BOOL bMute, int vol)
{
	if (m_pCallback)
		m_pCallback->OnVolumeChanged(bMute, vol);
}

bool CPlayback::GetAudioDeviceNegoFormat(const std::string& deviceId, AudioFormat& negoFmt)
{
	std::optional<DeviceFormat> outputDeviceFmt;
	
	if (deviceId.empty()) //default id
	{
		std::string defDeviceId;
		if (!GetDefaultDeviceId(defDeviceId))
			return false;
		outputDeviceFmt = m_speaker->GetDeviceFormat(defDeviceId);
	}
	else
		outputDeviceFmt = m_speaker->GetDeviceFormat(deviceId);

	if (outputDeviceFmt)
	{
		negoFmt = outputDeviceFmt.value().deviceFmt;
	}
	else
	{
		AudioFormat audioFmt;
		InitAudioFormat(&audioFmt, AudioDataFormat::PCM_S16, 2, 44100);
		negoFmt = m_audioDevice->NegotiateFormat(audioFmt);
	}

	return true;
}

bool CPlayback::InitPlayback(PlaybackCallback* pCallback, std::unique_ptr<CAudioDevice> audioDevice)
{
	assert(audioDevice);
	assert(!m_audioDevice);

	m_pCallback = pCallback;
	SetAudioDevice(std::move(audioDevice));

	return true;
}

void CPlayback::SetAudioDevice(std::unique_ptr<CAudioDevice> audioDevice)
{
	if (m_audioDevice)
	{
		assert(!m_audioDevice->IsOpened());
		m_audioDevice.reset();
	}

	m_audioDevice = std::move(audioDevice);
	m_audioDevice->SetCallback([_Self = weak_from_this()](void* out_buf, uint32_t frames, double stream_time, int status)
	{
		if (auto ThisPtr = _Self.lock(); ThisPtr)
		{
			AudioBuffer& audioBuffer = ThisPtr->GetAudioBuffer();
			//const CAudioSource* pAudioSource = ThisPtr->GetAudioSource();
			const CAudioDevice* audioDevice = ThisPtr->GetAudioDevice();
			const AudioFormat& deviceFmt = audioDevice->GetDeviceFormat();

			uint32_t channels = deviceFmt.numChannels;
			uint32_t frameSize = deviceFmt.blockAlign;

			// Playback
			std::size_t availR = audioBuffer.GetAvailableRead();

			std::size_t framesInBuffer = availR;
#ifdef TIMER_ELLASP_TEST
			long long beg_read = g_glbTimer.GetNS();
#endif
			if (availR > frames)
				audioBuffer.Read((uint8_t*)out_buf, frames, deviceFmt.format);
			else if (availR > 0) {
				audioBuffer.Read((uint8_t*)out_buf, availR, deviceFmt.format);
				memset((uint8_t*)out_buf + availR * frameSize, 0, (frames - availR) * frameSize);
			}
			else
				memset(out_buf, 0, frames * frameSize);
#ifdef TIMER_ELLASP_TEST
			long long beg_read2 = g_glbTimer.GetNS();
			char cad[512];
			sprintf_s(cad, "AudioDeviceCallback, read to out_buf, availR=%I64u, elasp=%I64d \n", availR, (beg_read2 - beg_read));
			OutputDebugStringA(cad);
#endif
			ThisPtr->UpdataPlayback(out_buf, frames, framesInBuffer);

			ThisPtr->m_pbSemaphore.release();
		}

		return 0;
	});

	m_audioDevice->SetVolumeNotifier([_Self = weak_from_this()](int vol, bool bMute) {
		if (auto ThisPtr = _Self.lock(); ThisPtr)
			ThisPtr->OnDeviceVolumeChanged(bMute ? TRUE : FALSE, vol);
	});
}

void CPlayback::ShutdownPlayback()
{
	StopPlaybackThread();
	CloseAudioDevice();
}

void CPlayback::StartPlaybackThread()
{
	m_quitSign = false;

	m_pbThread = std::thread([_Self = weak_from_this()]() {
		if (auto ThisPtr = _Self.lock(); ThisPtr)
		{
			ThisPtr->m_pbThreadRunning.store(true, std::memory_order_release);
			ThisPtr->m_quitSemaphore.try_acquire_for(100ns); //清除信号量

			AudioBuffer& audioBuffer = ThisPtr->GetAudioBuffer();
			const CAudioDevice* audioDevice = ThisPtr->GetAudioDevice();
			AudioFormat deviceFmt = audioDevice->GetDeviceFormat();
			CAudioSource* pAudioSource = ThisPtr->GetAudioSource();
			for(uint32_t streamIdx = 0; streamIdx < pAudioSource->GetTotalAudioStreams(); ++streamIdx)
			{
				try
				{
					bool bQuirSign = ThisPtr->m_quitSign.load(std::memory_order_acquire);
					if (bQuirSign)
					{
						break;
					}
					pAudioSource->StartAudioStream(streamIdx);
					AudioFormat audioFmt = pAudioSource->GetAudioFormat();
					ThisPtr->NotifyAudioStreamBegin(pAudioSource, streamIdx);

					uint32_t multi = (uint32_t)(float(deviceFmt.sampleRate) / float(audioFmt.sampleRate)) + 1;
					multi = std::max(multi, (uint32_t)2);
					uint32_t deviceFrames = DECODE_BUF_FRAMES * multi;
							uint32_t deviceBufSize = deviceFrames * deviceFmt.blockAlign;
					std::unique_ptr<uint8_t[]> tmpBuf = std::make_unique<uint8_t[]>(deviceBufSize);
					if (pAudioSource->SetOutputFormat(deviceFmt, true))
					{
						bool init_open_first = true; //曲目最开始的 fadein 标志
						bool decoding_finished = false;
						pAudioSource->SetDecodingFinished(false);
						while (!decoding_finished)
						{
							bool singaled = ThisPtr->m_pbSemaphore.try_acquire_for(std::chrono::milliseconds(2000)); //, 2s
							bQuirSign = ThisPtr->m_quitSign.load(std::memory_order_acquire);
							if (bQuirSign)
							{
								audioBuffer.Clear();
								break;
							}
							if(pAudioSource->IsSeekOnGoing())
							{
								audioBuffer.Clear();
                                pAudioSource->ClearSeekOnGoing();
							}

							if (singaled)
							{
								std::size_t availWrite = audioBuffer.GetAvailableWrite();
								while (availWrite > deviceFrames)
								{
		#ifdef TIMER_ELLASP_TEST
									long long beg_read = g_glbTimer.GetNS();
		#endif
									uint32_t readFrames = pAudioSource->ReadBuffer(tmpBuf.get(), deviceBufSize, DECODE_BUF_FRAMES);
		#ifdef TIMER_ELLASP_TEST
									long long beg_read2 = g_glbTimer.GetNS();
									char cad[512];
									sprintf_s(cad, "PlaybackThread, ReadBuffer(%u), availW = %I64u, elasp = %I64d\n", readFrames, availWrite, (beg_read2 - beg_read));
		#endif

									if (readFrames == 0)
									{
										if (pAudioSource->GetCurrentFrame() < pAudioSource->GetTotalFrames())
										{
										}
												decoding_finished = true;
										break;
									}

									if (!audioBuffer.Write(tmpBuf.get(), readFrames))
									{
										break;
									}

									availWrite = audioBuffer.GetAvailableWrite();
								}
							}
						}			
					}
					else
					{
						std::cerr << "Playback: playback fail to set device format!" << std::endl;
					}
					tmpBuf.reset();
					pAudioSource->SetDecodingFinished(true);
					ThisPtr->m_audioEndSemaphore.acquire(); 
					pAudioSource->StopAudioStream(streamIdx);
				}
				catch (const std::exception& e)
				{
				}
			}

			ThisPtr->m_quitSemaphore.release();
			ThisPtr->m_pbThreadRunning.store(false, std::memory_order_release);	
		}
	});
	m_pbThread.detach();
}

void CPlayback::StopPlaybackThread()
{
	bool bIsRnnung = m_pbThreadRunning.load(std::memory_order_acquire);
	if (bIsRnnung)
	{
		m_quitSign.store(true, std::memory_order_release);
		m_pbSemaphore.release(); 
		m_audioEndSemaphore.release();
		m_quitSemaphore.acquire(); 
	}
}

void CPlayback::OpenAudioDevice(const AudioFormat& audioFmt)
{
	m_audioDevice->Open(&audioFmt, 0, m_deviceId);
	AudioFormat deviceFmt = m_audioDevice->GetDeviceFormat();
	
	m_audioBuf.ResetBuffer((std::size_t)deviceFmt.sampleRate * m_PreferBufferLength, deviceFmt.blockAlign);
}

void CPlayback::CloseAudioDevice()
{
	m_audioDevice->Close();
}

void CPlayback::StartCurrentAudioSource()
{
	try
	{
		if (m_status != PlaybackStatus::Paused)
		{
			//m_audioBuf.Clear();
			//m_dataSource->SeekToFrame(0);

			if ((m_status != PlaybackStatus::Playing) && (m_status != PlaybackStatus::PlayingEnd))
				StartPlaybackThread();
		}

		m_pbSemaphore.release();
		m_audioDevice->Start();
		m_status = PlaybackStatus::Playing;	
	}
	catch (...)
	{
		std::this_thread::sleep_for(100ms);
		StopPlaybackThread();
		m_status = PlaybackStatus::Stoped;
		throw;
	}

	if (m_pCallback)
		m_pCallback->OnControlEvent(PlayControl::Play);
}

void CPlayback::StopCurrentAudioSource(bool bNotify)
{
	if (m_status != PlaybackStatus::Stoped)
	{
		m_audioDevice->Stop();
		StopPlaybackThread();
		m_dataSource->Reset();
		m_status = PlaybackStatus::Stoped;

		if (m_pCallback && bNotify)
			m_pCallback->OnControlEvent(PlayControl::Stop);
	}
}
