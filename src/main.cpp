#include <iostream>
#include "cmdline.h"
#include "UnicodeConvert.h"
#include "PlayInterface.h"
#include "DecoderFactory.h"
#include "GlobalConfig.h"
#include "ScopeGuard.h"

bool MakeParser(cmdline::parser& a)
{
    a.add("play", 'p', "play media file or playlist");
    a.add("convert", 'c', "convert media file format");
    a.add("tui", '\0', "launch TUI interface");
    a.add<std::string>("devicetype", 't', "device type", false, "Native", cmdline::oneof<std::string>("Native", "Plusin"));
    a.add<std::string>("devicename", 'n', "device name", false, "Wasapi (Share mode)", cmdline::oneof<std::string>("Wasapi (Share mode)", "Wasapi (Exclusive mode)", "DirectSound"));
    a.add<std::string>("deviceid", 'i', "device id", false, "");
    a.add<std::string>("speakerlayout", 'o', "speaker layout config file", false, "");
    a.add<std::string>("filename", 'f', "media file name", false, "");
    a.add<std::string>("playlist", 'l', "playlist file name", false, "");

    return true;
}


int main(int argc, char *argv[])
{
    cmdline::parser parser;
    if (!MakeParser(parser))
    {
        std::cout << "Fail to create command line parser!" << std::endl;
        return -1;
    }
    
    std::string deviceType, devideName, deviceId;
    try
    {
        parser.parse_check(argc, argv); 
        
		CDecoderFactory& factory = CDecoderFactory::GetInstance();
		factory.SetAppVersion(1, 0);
		//std::string pluginCfg = GetPluginConfigFilePathname();
		//factory.LoadPluginConfig(pluginCfg); 
		std::string decodercfg = GetDecoderConfigFilePathname();
		factory.LoadCustomDecoderConfig(decodercfg);
    
        if (parser.exist("play"))
        {
            deviceType = parser.get<std::string>("devicetype");
            devideName = parser.get<std::string>("devicename");
            deviceId = parser.get<std::string>("deviceid");   
            SetupDevice(deviceType, devideName, deviceId);

            bool bPlaylist = false;
            std::string filename;
            if(parser.exist("playlist"))
            {
                filename = parser.get<std::string>("playlist");
                bPlaylist = true;
            }
            else
            {
                filename = parser.get<std::string>("filename");
                bPlaylist = false;
            }
            std::string speakerCfg = parser.get<std::string>("speakerlayout");
            //std::cout << "audio source name: " << audioSource->GetName() << std::endl;
            if(parser.exist("tui"))
            {
                StartPlayingTuiInterface(filename, bPlaylist, speakerCfg);
            }
            else
            {
                std::cout << "Play media file: " << filename << std::endl;
                StartPlayingInterface(filename, bPlaylist, speakerCfg);
            }
        }        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}