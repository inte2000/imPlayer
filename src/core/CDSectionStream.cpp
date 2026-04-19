#include <format>
#include "UnicodeConvert.h"
#include "CDSectionStream.h"
#include <nlohmann/json.hpp> 

using json = nlohmann::json;


std::unique_ptr<CDataStream> MakeCDSectionStream(const std::wstring& deviceName, uint32_t track)
{
    auto ptr = std::make_unique<CCDSectionStream>();
    if (ptr)
    {
        ptr->Open(deviceName, track);
    }
    return ptr;
    //return std::unique_ptr<CFileStream>(new CFileStream{});
}

bool CCDSectionStream::Open(const std::wstring& deviceName, uint32_t track)
{
    if (m_Buf || IsOpen())
        return false;

	if (m_cd.Open(deviceName)) // Audio CD or Audio CD Image
	{
		m_track = (track > 0) ? (track - 1) : 0;
		m_totalLength = m_cd.GetTrackSize(m_track);
		m_curPos = 0;
		m_name = std::format(L"CD Audio Track {}", m_track + 1);

		m_metaInfo.itemSequence = m_track;
		m_metaInfo.itemName = m_name;
		m_metaInfo.itemTitle = m_cd.GetTrackTitle(m_track);
		m_metaInfo.itemArtist = m_cd.GetTrackArtist(m_track);
		m_metaInfo.itemPerformer = m_cd.GetTrackPerformer(m_track);
		m_metaInfo.itemAlbum = m_cd.GetTrackAlbum(m_track);

		m_Buf = std::make_unique<uint8_t[]>(CDSECTION_BUF_SIZE);

		if (!ReloadBuffer(m_curPos)) //CD buffer 预读
			return false;

		m_metaInfo.itemMediaType = "PCM-CD";
		//the audio-CD format always is 44100Hz, 16 bit, stereo
		m_metaInfo.itemFormat.format = AudioDataFormat::PCM_S16;
		m_metaInfo.itemFormat.bytesPerSample = 2;
		m_metaInfo.itemFormat.numChannels = 2;
		m_metaInfo.itemFormat.blockAlign = m_metaInfo.itemFormat.bytesPerSample * m_metaInfo.itemFormat.numChannels;
		m_metaInfo.itemFormat.chLayout = StandLayoutByChannelsCount(m_metaInfo.itemFormat.numChannels);
		m_metaInfo.itemFormat.sampleRate = 44100;

		return true;
	}


    return false;
}

void CCDSectionStream::Close() 
{ 
    if(m_cd.IsOpened())
        m_cd.Close(); 
}

bool CCDSectionStream::IsOpen() const
{
    return m_cd.IsOpened();
}

uint32_t CCDSectionStream::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    if (!m_Buf || !IsOpen())
        return 0;

    uint8_t* pOutBuf = static_cast<uint8_t*>(pBuf);
    //std::size_t uplimit = m_curBufPos + CDSECTION_BUF_SIZE;
    std::size_t uplimit = m_curBufPos + m_curBufValidSize;
    if (uplimit > m_totalLength)
        uplimit = m_totalLength;
    std::size_t remain = size;
    std::size_t readcount = 0;
    while ((m_curPos + remain) > uplimit)
    {
        std::size_t inBufOffset = m_curPos - m_curBufPos;
        std::size_t avail = uplimit - m_curPos;
        std::memcpy(pOutBuf, m_Buf.get() + inBufOffset, avail);
        remain -= avail;
        readcount += avail;
        m_curPos += avail;
        if (!ReloadBuffer(m_curPos))
            return static_cast<uint32_t>(readcount);
        //修正上限
        //uplimit = m_curBufPos + CDSECTION_BUF_SIZE;
        uplimit = m_curBufPos + m_curBufValidSize;

        if (uplimit > m_totalLength)
            uplimit = m_totalLength;
    }

    if ((m_curPos + remain) > m_totalLength)
    {
        std::size_t inBufOffset = m_curPos - m_curBufPos;
        std::size_t avail = m_totalLength - m_curPos;
        std::memcpy(pOutBuf + readcount, m_Buf.get() + inBufOffset, avail);
        m_curPos += avail;
        readcount += avail;
    }
    else
    {
        std::size_t inBufOffset = m_curPos - m_curBufPos;
        std::memcpy(pOutBuf + readcount, m_Buf.get() + inBufOffset, remain);
        m_curPos += remain;
        readcount += remain;
    }

    return static_cast<uint32_t>(readcount);
}

uint32_t CCDSectionStream::Write(const void* pBuf, uint32_t size, uint32_t timeout)
{
    return 0; //不支持
}

void CCDSectionStream::Seek(SeekBase base, long long off)
{
    bool bNegLeft = (off < 0);
    std::size_t poff = std::abs(off);
    if (m_Buf && IsOpen())
    {
        std::size_t newpos = 0;
        if (base == SeekBase::Begin)
            newpos = bNegLeft ? 0 : poff;
        else if (base == SeekBase::End)
            newpos = bNegLeft ? (m_totalLength - poff) : m_totalLength;
        else
            newpos = bNegLeft ? (m_curPos - poff) : (m_curPos + poff);

        if (m_curPos != newpos)
        {
            std::size_t uplimit = m_curBufPos + m_curBufValidSize;
            if ((newpos > uplimit) || (newpos < m_curBufPos))
                ReloadBuffer(newpos);
            
            m_curPos = newpos;
        }
    }
}

void CCDSectionStream::StreamControl(const CDecodeInitCtx* decodeInit)
{
    if (decodeInit)
    {
        //uint32_t sacdSamplerate = decodeInit->GetInitSampleRate();
        //uint32_t preferMultiChArea = decodeInit->GetMultiChAreaMode();
        //m_sacd.SetPcmSampleRate(sacdSamplerate);
        //m_sacd.SetPreferMultiChArea(preferMultiChArea);
    }
}

bool CCDSectionStream::ReloadBuffer(std::size_t curPos)
{
    if (curPos >= m_totalLength)
        return false;

    uint32_t sectorsCount = CDSECTION_SECTORS; //预备读取 2048 扇区
    std::size_t bytesCount = CDSECTION_SECTORS * RAW_SECTOR_SIZE;
    if ((curPos + bytesCount) > m_totalLength)
    {
        std::size_t bytesRemain = m_totalLength - curPos;
        sectorsCount = static_cast<uint32_t>(bytesRemain / RAW_SECTOR_SIZE);
    }

	ULONG roundStart = static_cast<ULONG>(curPos / RAW_SECTOR_SIZE); //根据 curpos 圆整计算开始 sector
	ULONG readed = m_cd.ReadTrack(m_track, roundStart, sectorsCount, m_Buf.get(), CDSECTION_BUF_SIZE);

	m_curBufPos = roundStart * RAW_SECTOR_SIZE;
	m_curBufValidSize = readed;

	return (readed > 0);
}