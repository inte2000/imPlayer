#ifndef _PLAY_INTERFACE_H
#define _PLAY_INTERFACE_H


#include <string>

void SetupDevice(const std::string& type, const std::string& deviceName, const std::string& deviceId);
void StartPlayingInterface(const std::string& filename, bool bPlaylist, const std::string& speakerLayout);
void StartPlayingTuiInterface(const std::string& filename, bool bPlaylist, const std::string& speakerLayout);

#endif //_PLAY_INTERFACE_H
