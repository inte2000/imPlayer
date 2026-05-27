/*
s_nativeMp3 静态成员是手工填的，构造函数中这一行是手工加上的：
m_DecoderItems.reserve(64);

MakeAudioDecoder() 成员函数有一半是手写的，出了根据插件名确定解码器，还要看看有没有用户配置的
自定义解码器设计。有时候多个解码器都可以处理同一种媒体文件，用户可设置优先采用何种解码器，AI 对于
DecodeMap 的理解有问题，始终无法生成满意的结果，于是就上手了。

添加 AddPluginObject 和 RemovePluginObject 成员
大模型：GPT 5.3 Codex
任务说明：todo_task_11.txt
*/
#include <vector>
#include <utility>
#include <format>
#include <algorithm>
#include "UnicodeConvert.h"
#include "stdfilesystem.h"
#include "AudioInfo.h"
#include "WavDecoder.h"
#include "PluginDecoder.h"
#include "DecoderFactory.h"

static DecoderMapItem s_nativeWav = {
    CWavDecoder::Name(), "imPlayer Group", "", DECODE_TYPE_NATIVE,
    [](uint32_t st) { return new CWavDecoder(st); }, WavQueryFileType, nullptr,
    {
        {StreamFormatWav, "Waveform(WAV)", ".wav;.wave"},
        {StreamFormatWavEx, "Waveform(WAVEEX)", ".wav;.rf64"}
    }
};

CDecoderFactory::CDecoderFactory()
{
    m_verMajor = 1;
    m_verMinor = 0;
    m_availableStreamFormatID = 1024;

    m_DecoderItems.reserve(64);
    m_DecoderItems.push_back(std::move(s_nativeWav));

    for (const auto& item : m_DecoderItems)
    {
        for (const auto& k : item.exts)
        {
            m_DecoderMap.SetDecoderMap(k.st, k.desc, item.name);
        }
    }
}

void CDecoderFactory::LoadCustomDecoderConfig(const std::string& decoderfile)
{
    std::vector<DecoderDesc> decodermap;
    if (LoadDecoderMapFile(decoderfile, decodermap))
    {
        for (const auto& dd : decodermap)
        {
            m_DecoderMap.AddCustomDecoderMap(dd.st, dd.desc, dd.decodername);
        }
    }
}

bool CDecoderFactory::SaveCustomDecoderConfig(const std::string& decoderfile)
{
    std::vector<DecoderDesc> decodermap;
    m_DecoderMap.EnumCustomDecoderMap([&decodermap](uint32_t st, const std::string& desc, const std::string& decodername) {
        decodermap.emplace_back(st, desc, decodername);
        });

    return SaveDecoderMapFile(decoderfile, decodermap);
}

uint32_t CDecoderFactory::ParseFileFormat(const std::wstring& filename)
{
    uint32_t type = StreamFormatUnknown;

    for (const auto& item : m_DecoderItems)
    {
        type = item.parser(filename);
        if (type != StreamFormatUnknown)
            break;
    }

    return type;
}

std::unique_ptr<CAudioDecoder> CDecoderFactory::MakeAudioDecoder(uint32_t fileFmt)
{
    std::string decodername = m_DecoderMap.GetDecoderName(fileFmt);

    for (const auto& item : m_DecoderItems)
    {
        if (item.name == decodername)
        {
            return std::unique_ptr<CAudioDecoder>(item.Creator(fileFmt));
        }
    }

    throw std::runtime_error(std::format("not find decoder has name: {}", decodername));
    return nullptr;
}

std::vector<FileExtItem> CDecoderFactory::GetPluginDecoderExtFilters() const
{
    std::vector<FileExtItem> result;

    for (const auto& item : m_DecoderItems)
    {
        if (item.type == DECODE_TYPE_PLUGIN)
        {
            for (const auto& k : item.exts)
            {
                auto findIt = std::find_if(result.begin(), result.end(), [k](const FileExtItem& item) {
                    return (item.st == k.st);
                    });
                if (findIt == result.end())
                    result.emplace_back(k.st, k.desc, k.extList);
            }
        }
    }

    return result;
}

std::optional<DecoderMapItem> CDecoderFactory::GetDecoderPlugin(const std::string& name)
{
    auto idFind = std::find_if(m_DecoderItems.begin(), m_DecoderItems.end(), [&name](const DecoderMapItem& item) {
        return ((item.name == name) && (item.type == DECODE_TYPE_PLUGIN));
        });
    if (idFind != m_DecoderItems.end())
        return *idFind;

    return std::nullopt;
}

std::tuple<bool, std::string> CDecoderFactory::AddPluginObject(const PluginDllObject& dllObj)
{
    auto idFind = std::find_if(m_DecoderItems.begin(), m_DecoderItems.end(), [&dllObj](const DecoderMapItem& item) {
        return (item.name == dllObj.name);
        });
    if (idFind != m_DecoderItems.end())
    {
        return { false, std::format("decoder has same name existed: {}", dllObj.name) };
    }

    ApplicationConfig appCfg = {};
    appCfg.major_ver = m_verMajor;
    appCfg.minor_ver = m_verMinor;
    appCfg.FileTypeIdBegin = m_availableStreamFormatID;

    PluginRegister regInfo = {};
    regInfo.size = sizeof(regInfo);
    if (dllObj.dll->OnRegister(&appCfg, &regInfo) != 0)
    {
        return { false, std::format("register plugin decoder failed: {}", dllObj.name) };
    }

    m_availableStreamFormatID += PLUG_FORMAT_MAX_LIMIT;
    const uint32_t regCount = std::min<uint32_t>(regInfo.format_count, PLUG_FORMAT_MAX_LIMIT);
    std::vector<FileExtItem> exts;
    exts.reserve(regCount);
    for (uint32_t i = 0; i < regCount; ++i)
    {
        const FileFormatReg& fmt = regInfo.fmt_reg[i];
        exts.emplace_back(fmt.id, fmt.desc, fmt.ext_list);
    }

    auto dll = dllObj.dll;
    DecoderCreator creator = [dll](uint32_t st) -> CAudioDecoder* {
        std::unique_ptr<CPluginDecoder> pDecoder = std::make_unique<CPluginDecoder>(st);
        pDecoder->AttachModule(dll);
        return pDecoder.release();
    };
    ParserFunc parser = [dll](const std::wstring& filename) mutable {
        return dll->ParseFileTypeID(filename);
    };
    ConfigFunc config = [dll](HWND hWnd) mutable {
        dll->ConfigPlugin(hWnd);
    };

    m_DecoderItems.push_back(DecoderMapItem{ dllObj.name, dllObj.publisher, Utf16ToUtf8(dllObj.dll->GetDllHostName()), DECODE_TYPE_PLUGIN,
        std::move(creator), std::move(parser), std::move(config), exts });
    for (const auto& k : exts)
    {
        m_DecoderMap.SetDecoderMap(k.st, k.desc, dllObj.name);
    }

    return { true, "" };
}

std::tuple<bool, std::string> CDecoderFactory::RemovePluginObject(const std::string& decoderName)
{
    auto idFind = std::find_if(m_DecoderItems.begin(), m_DecoderItems.end(), [&decoderName](const DecoderMapItem& item) {
        return (item.name == decoderName);
        });
    if (idFind == m_DecoderItems.end())
    {
        return { false, std::format("decoder not existed: {}", decoderName) };
    }

    if (idFind->type != DECODE_TYPE_PLUGIN)
    {
        return { false, std::format("decoder is not plugin: {}", decoderName) };
    }

    m_DecoderItems.erase(idFind);
    m_DecoderMap.RemoveDecoderMap(decoderName);

    return { true, "" };
}

std::vector<DecoderItem> CDecoderFactory::GetDecoders() const
{
    std::vector<DecoderItem> result;

    result.reserve(m_DecoderItems.size());
    for (const auto& v : m_DecoderItems)
    {
        result.emplace_back(DecoderItem{ v.name, v.type, v.hostfile });
    }

    return result;
}

std::vector<DecoderSettingItem> CDecoderFactory::GetDecoderSettings() const
{
    std::vector<DecoderSettingItem> result;

    // Build default decoder mapping list.
    for (const auto& v : m_DecoderItems)
    {
        for (const auto& k : v.exts)
        {
            auto itFind = std::find_if(result.begin(), result.end(), 
                [st = k.st](const DecoderSettingItem& fei) {
                    return (fei.st == st);
                });
            if(itFind == result.end())
                result.emplace_back(k.st, k.desc, k.extList, v.name);
        }
    }
    // Override with user custom mapping.
    m_DecoderMap.EnumCustomDecoderMap([&result](uint32_t st, const std::string& desc, const std::string& decodername) {
        auto itFind = std::find_if(result.begin(), result.end(),
            [st, &desc](const DecoderSettingItem& fei) {
            return ((fei.st == st) && (fei.desc == desc));
        });
        if (itFind != result.end()) {
            itFind->decodername = decodername;
        }
     });

    return result;
}

bool CDecoderFactory::SetCustomDecoderMap(uint32_t fileFmt, const std::string& desc, const std::string& decoderName)
{
    return m_DecoderMap.AddCustomDecoderMap(fileFmt, desc, decoderName);
}

void CDecoderFactory::RemoveDecoderMap(const std::string& decoderName)
{
    m_DecoderMap.RemoveCustomDecoderMap(decoderName);
}
