#ifndef DECODER_FACTORY_H
#define DECODER_FACTORY_H

#include <memory>
#include <string>
#include <functional>
#include "AudioInfo.h"
#include "AudioDecoder.h"

using DecoderMaker = std::function<CAudioDecoder*()>;

class CDecoderFactory final
{
    using DecoderMap = std::unordered_map<uint32_t, DecoderMaker>;
public:
    static CDecoderFactory& GetInstance() {
        static CDecoderFactory s_Instance;

        return s_Instance;
    }
    std::unique_ptr<CAudioDecoder> MakeAudioDecoder(uint32_t fileFmt);
    bool RegisterDecoder(uint32_t fileFmt, DecoderMaker maker);
private:
    CDecoderFactory();
    DecoderMap m_decoders;
};

uint32_t ParseAudioFileFormat(const std::wstring& filename);



#endif //DECODER_FACTORY_H
