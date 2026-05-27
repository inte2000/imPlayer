/*
DecoderFactory.h 文件中约 30% 的代码是手写的，因为对插件解码器的理解跟大模型沟通了很多次，输出都不是很满意，
我感觉自己写可能更快，所以就直接上手了固定了成员方法的参数和返回值，然后让 AI 生成成员函数的实现。

添加 AddPluginObject 和 RemovePluginObject 成员
大模型：GPT 5.3 Codex
任务说明：todo_task_11.txt
*/
#ifndef DECODER_FACTORY_H
#define DECODER_FACTORY_H

#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <optional>
#include <functional>
#include <windows.h>
#include "AudioInfo.h"
#include "AudioDecoder.h"
#include "AudioDecoderMap.h"
#include "PluginObjects.h"


using DecoderCreator = std::function<CAudioDecoder* (uint32_t)>;
using ParserFunc = std::function<uint32_t(const std::wstring&)>;
using ConfigFunc = std::function<void(HWND hWnd)>;
using DecoderItem = std::tuple<std::string, uint32_t, std::string>;

typedef struct tagDecoderSettingItem
{
    uint32_t st;
    std::string desc;
    std::string extList;
    std::string decodername;
}DecoderSettingItem;

typedef struct tagFileExtItem
{
    uint32_t st;
    std::string desc;
    std::string extList;
}FileExtItem;

typedef struct tagDecoderMapItem
{
    std::string name;
    std::string publisher;
    std::string hostfile;
    uint32_t type;
    DecoderCreator Creator;
    ParserFunc parser;
    ConfigFunc config;
    std::vector<FileExtItem> exts;
}DecoderMapItem;

class CDecoderFactory final
{
public:
    static CDecoderFactory& GetInstance() {
        static CDecoderFactory s_Instance;

        return s_Instance;
    }
    void SetAppVersion(uint32_t major, uint32_t minor) {
        m_verMajor = major;
        m_verMinor = minor;
    }

    void LoadCustomDecoderConfig(const std::string& decoderfile);
    bool SaveCustomDecoderConfig(const std::string& decoderfile);

    uint32_t ParseFileFormat(const std::wstring& filename);
    std::unique_ptr<CAudioDecoder> MakeAudioDecoder(uint32_t fileFmt);
    
    std::optional<DecoderMapItem> GetDecoderPlugin(const std::string& name);
    std::tuple<bool, std::string> AddPluginObject(const PluginDllObject& dllObj);
    std::tuple<bool, std::string> RemovePluginObject(const std::string& decoderName);
    std::vector<FileExtItem> GetPluginDecoderExtFilters() const;
    std::vector<DecoderItem> GetDecoders() const;

    std::vector<DecoderSettingItem> GetDecoderSettings() const;
    bool SetCustomDecoderMap(uint32_t fileFmt, const std::string& desc, const std::string& decoderName);
    void RemoveDecoderMap(const std::string& decoderName);
protected:

private:
    CDecoderFactory();
    uint32_t m_availableStreamFormatID;
    uint32_t m_verMajor, m_verMinor;
    std::vector<DecoderMapItem> m_DecoderItems;

    CAudioDecoderMap m_DecoderMap;
};

#endif //DECODER_FACTORY_H
