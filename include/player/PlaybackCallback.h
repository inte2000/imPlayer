#ifndef PLAY_BACK_CALL_BACK_H
#define PLAY_BACK_CALL_BACK_H

#include <functional>
#include "AudioInfo.h"
#include "MediaTag.h"

enum class PlayControl
{
    Play = 1,
    Pause = 2,
    Stop = 3
};

class PlaybackCallback
{
public:
    virtual void OnAudioBegin(uint32_t streamIdx, const CMediaTag& metaInfo, const std::wstring& name, float totalSeconds) = 0;
    virtual void OnAudioUpdate(float curSeconds, float *powerBands, int bands) = 0;
    //若回调兑现启动了新的 audio source，则返回 true，否则返回 false，playback 就关闭当前已经播放完成的 source
    virtual bool OnAudioEnd(bool lastStream) = 0;
    virtual void OnControlEvent(PlayControl ctrl) = 0;
    virtual void OnVolumeChanged(BOOL bMute, int vol) = 0;
};

#endif //PLAY_BACK_CALL_BACK_H
