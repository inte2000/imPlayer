#ifndef SNDFILE_FUNC_H
#define SNDFILE_FUNC_H

#include <string>
#include <string_view>
#include "AudioInfo.h"
#include <sndfile.h>

std::string_view SndfileGetFileTypeName(uint32_t type);
AudioDataFormat SndfileTransSubType(int sndsubtype);
int SndfileGetSubType(AudioDataFormat format);
uint32_t SndfileStreamFmtToFileType(uint32_t streamFmt);
float LibsndFileGetSeconds(const std::wstring& filename);


#endif //SNDFILE_FUNC_H
