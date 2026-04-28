#include "framework.h"
#include <filesystem>
#include <string>
#include "StdFileSystem.h"
#include "GlobalConfig.h"
#include "UnicodeConvert.h"


std::string GetSpeakerConfigFilePathname()
{
    std::filesystem::path pathname = GetApplicationPathname();
    pathname /= "config";
    pathname /= "speaker.config";

    std::u8string t1 = pathname.u8string();
    std::string u8Pathname{ reinterpret_cast<char *>(t1.data()), t1.size() };
    return u8Pathname;
}

std::string GetPluginConfigFilePathname()
{
    std::filesystem::path pathname = GetApplicationPathname();
    pathname /= "config";
    pathname /= "plugin.config";

    std::u8string t1 = pathname.u8string();
    std::string u8Pathname{ reinterpret_cast<char *>(t1.data()), t1.size() };
    return u8Pathname;
}

std::string GetDecoderConfigFilePathname()
{
    std::filesystem::path pathname = GetApplicationPathname();
    pathname /= "config";
    pathname /= "decoder.config";

    std::u8string t1 = pathname.u8string();
    std::string u8Pathname{ reinterpret_cast<char *>(t1.data()), t1.size() };
    return u8Pathname;
}

