#ifndef PLAY_BACK_CALL_BACK_H
#define PLAY_BACK_CALL_BACK_H

#include <functional>
#include "AudioInfo.h"

enum class PlayControl
{
    Play = 1,
    Pause = 2,
    Stop = 3
};

class PlaybackCallback
{
public:
    virtual void OnAudioBegin(const AudioFormat& audioFmt, const std::string& extraInfo, const std::wstring& name, float totalSeconds) = 0;
    virtual void OnAudioUpdate(float curSeconds, float *powerBands, int bands) = 0;
    
    virtual bool OnAudioEnd() = 0;
    virtual void OnControlEvent(PlayControl ctrl) = 0;
    virtual void OnVolumeChanged(BOOL bMute, int vol) = 0;
};

#endif //PLAY_BACK_CALL_BACK_H
