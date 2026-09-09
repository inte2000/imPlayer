#include <curl/curl.h>

#include "LibCurlInit.h"

CLibCurlInit::CLibCurlInit()
    : m_initialized(false)
{
    m_initialized = (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
}

CLibCurlInit::~CLibCurlInit()
{
    if (m_initialized) {
        curl_global_cleanup();
    }
}

bool CLibCurlInit::IsInitialized() const
{
    return m_initialized;
}
