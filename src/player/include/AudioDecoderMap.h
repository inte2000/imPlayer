#ifndef __AUDIO_DECODER_MAP_H__
#define __AUDIO_DECODER_MAP_H__

#include <string>
//#include <map>
#include <map>

typedef struct tagDecoderDesc
{
    uint32_t st;
    std::string desc;
    std::string decodername;
}DecoderDesc;

class CAudioDecoderMap
{
public:
    CAudioDecoderMap();
    void Clear() { m_DecoderMap.clear(); }
    bool IsEmpty() const { return m_DecoderMap.empty(); }
    void SetDecoderMap(uint32_t st, const std::string& desc, const std::string& decoderName);
    void RemoveDecoderMap(const std::string& decoderName);
    std::string GetDecoderName(uint32_t fileFmt);
    bool AddCustomDecoderMap(uint32_t st, const std::string& desc, const std::string& decoderName);
    bool RemoveCustomDecoderMap(const std::string& decoderName);

    template<typename Func>
    void EnumCustomDecoderMap(Func&& callback) const {
        for (const auto& item : m_CustomMap) {
            callback(item.st, item.desc, item.decodername);
        }
    }

protected:
    std::map<uint32_t, DecoderDesc> m_DecoderMap; //运行中的动态关系，用户快速得到文件类型与解码器的关系
    std::vector<DecoderDesc> m_CustomMap; //用户自定义的关系，需要保存到文件
};

bool LoadDecoderMapFile(const std::string& filename, std::vector<DecoderDesc>& decodermap);
bool SaveDecoderMapFile(const std::string& filename, const std::vector<DecoderDesc>& decodermap);

#endif //__AUDIO_DECODER_MAP_H__
