#ifndef _DECODER_PARAMETERS_MGMT_H_
#define _DECODER_PARAMETERS_MGMT_H_

#include <string>
#include <unordered_map>
#include <mutex>
#include "DecoderParameters.h"
#include "AudioInfo.h"

class CDecoderParametersMgmt
{
public:
    static CDecoderParametersMgmt& GetInstance() {
        static CDecoderParametersMgmt s_Instance;

        return s_Instance;
    }

    const CDecodeInitCtx* GetDecodeParamter(uint32_t type) {
        std::lock_guard<std::mutex> guard(m_mgmtMtx);

        auto it = m_Parameters.find(type);
        if(it != m_Parameters.end()) {
            return it->second;
        }

        return nullptr;
    }
    void SetDecoderParameter(uint32_t type, CDecodeInitCtx* parameters) {
        std::lock_guard<std::mutex> guard(m_mgmtMtx);

        auto it = m_Parameters.find(type);
        if (it == m_Parameters.end()) {
            m_Parameters[type] = parameters;
        }
        else {
            CDecodeInitCtx* oldparam = it->second;
            it->second = parameters;
            if (oldparam)
                delete oldparam;
        }
    }

    ~CDecoderParametersMgmt() {
        std::lock_guard<std::mutex> guard(m_mgmtMtx);

        for (auto& item : m_Parameters)
            delete item.second;
        m_Parameters.clear();
    }
protected:
    CDecoderParametersMgmt() {
    }
private:
    std::mutex m_mgmtMtx;
    std::unordered_map<uint32_t, CDecodeInitCtx*> m_Parameters;
};

#endif //_DECODER_PARAMETERS_MGMT_H_


