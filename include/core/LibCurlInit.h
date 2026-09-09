#ifndef LIB_CURL_INIT_H
#define LIB_CURL_INIT_H

class CLibCurlInit
{
public:
    CLibCurlInit();
    ~CLibCurlInit();

    CLibCurlInit(const CLibCurlInit&) = delete;
    CLibCurlInit& operator=(const CLibCurlInit&) = delete;
    CLibCurlInit(CLibCurlInit&&) = delete;
    CLibCurlInit& operator=(CLibCurlInit&&) = delete;

    bool IsInitialized() const;

private:
    bool m_initialized;
};

#endif //LIB_CURL_INIT_H
