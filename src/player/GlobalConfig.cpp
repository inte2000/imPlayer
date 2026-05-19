/*
20260428 修改路径拼接错误
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_28.txt

20260428 添加MakeupDecoderPlugPathname函数
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_29.txt
*/
#include "framework.h"
#include <filesystem>
#include <string>
#include "StdFileSystem.h"
#include "GlobalConfig.h"
#include "UnicodeConvert.h"


std::string GetSpeakerConfigFilePathname()
{
    std::filesystem::path pathname = GetApplicationBasePath();
    pathname /= "config";
    pathname /= "speaker.config";

    std::u8string t1 = pathname.u8string();
    std::string u8Pathname{ reinterpret_cast<char *>(t1.data()), t1.size() };
    return u8Pathname;
}

std::string GetPluginConfigFilePathname()
{
    std::filesystem::path pathname = GetApplicationBasePath();
    pathname /= "config";
    pathname /= "plugin.config";

    std::u8string t1 = pathname.u8string();
    std::string u8Pathname{ reinterpret_cast<char *>(t1.data()), t1.size() };
    return u8Pathname;
}

std::string GetDecoderConfigFilePathname()
{
    std::filesystem::path pathname = GetApplicationBasePath();
    pathname /= "config";
    pathname /= "decoder.config";

    std::u8string t1 = pathname.u8string();
    std::string u8Pathname{ reinterpret_cast<char *>(t1.data()), t1.size() };
    return u8Pathname;
}

std::string MakeupDecoderPlugPathname(const std::string& hostname)
{
    if(std::filesystem::exists(hostname))
        return hostname;
    
    std::filesystem::path pathname = GetApplicationBasePath();
    pathname /= "plugins";
    pathname /= hostname;

    std::u8string t1 = pathname.u8string();
    std::string u8Pathname{ reinterpret_cast<char *>(t1.data()), t1.size() };
    return u8Pathname;
}