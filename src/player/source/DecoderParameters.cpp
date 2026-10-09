
#include "DecoderParameters.h"
#include "UnicodeConvert.h"


CMidiDecoderInitCtx::CMidiDecoderInitCtx(const std::string& backend, uint32_t sampleRate, float gain)
{
    m_backendName = backend;
    m_sampleRate = sampleRate;
    m_gain = gain;
}

CCDDecoderInitCtx::CCDDecoderInitCtx(uint32_t sacdSampleRate, bool bPreferMultiChArea)
{
    m_sacdSampleRate = sacdSampleRate;
    m_bPreferMultiChArea = bPreferMultiChArea;
}

