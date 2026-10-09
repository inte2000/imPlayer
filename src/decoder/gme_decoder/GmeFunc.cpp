/*
20260522 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_66.txt
*/
#include <gme/gme.h>

#include <array>
#include <cstring>

#include "AudioInfo.h"
#include "GmeFunc.h"

namespace {

constexpr uint32_t GME_FMT_AY_OFFSET = 0;
constexpr uint32_t GME_FMT_GBS_OFFSET = 1;
constexpr uint32_t GME_FMT_GYM_OFFSET = 2;
constexpr uint32_t GME_FMT_HES_OFFSET = 3;
constexpr uint32_t GME_FMT_KSS_OFFSET = 4;
constexpr uint32_t GME_FMT_NSF_OFFSET = 5;
constexpr uint32_t GME_FMT_NSFE_OFFSET = 6;
constexpr uint32_t GME_FMT_SAP_OFFSET = 7;
constexpr uint32_t GME_FMT_SPC_OFFSET = 8;

uint32_t g_formatIdBase = StreamFormatPlusBegin;

gme_type_t GmeAyType() { return gme_identify_extension(".ay"); }
gme_type_t GmeGbsType() { return gme_identify_extension(".gbs"); }
gme_type_t GmeGymType() { return gme_identify_extension(".gym"); }
gme_type_t GmeHesType() { return gme_identify_extension(".hes"); }
gme_type_t GmeKssType() { return gme_identify_extension(".kss"); }
gme_type_t GmeNsfType() { return gme_identify_extension(".nsf"); }
gme_type_t GmeNsfeType() { return gme_identify_extension(".nsfe"); }
gme_type_t GmeSapType() { return gme_identify_extension(".sap"); }
gme_type_t GmeSpcType() { return gme_identify_extension(".spc"); }
gme_type_t GmeVgmType() { return gme_identify_extension(".vgm"); }
gme_type_t GmeVgzType() { return gme_identify_extension(".vgz"); }

uint32_t GmeStreamFmtByType(gme_type_t musicType)
{
    if (musicType == GmeAyType()) {
        return GmeFormatAy();
    }
    if (musicType == GmeGbsType()) {
        return GmeFormatGbs();
    }
    if (musicType == GmeGymType()) {
        return GmeFormatGym();
    }
    if (musicType == GmeHesType()) {
        return GmeFormatHes();
    }
    if (musicType == GmeKssType()) {
        return GmeFormatKss();
    }
    if (musicType == GmeNsfType()) {
        return GmeFormatNsf();
    }
    if (musicType == GmeNsfeType()) {
        return GmeFormatNsfe();
    }
    if (musicType == GmeSapType()) {
        return GmeFormatSap();
    }
    if (musicType == GmeSpcType()) {
        return GmeFormatSpc();
    }
    if ((musicType == GmeVgmType()) || (musicType == GmeVgzType())) {
        return GmeFormatVgm();
    }

    return StreamFormatUnknown;
}

} // namespace

void SetGmeCustomFormatBase(uint32_t formatIdBase)
{
    g_formatIdBase = formatIdBase;
}

uint32_t GmeFormatAy() { return g_formatIdBase + GME_FMT_AY_OFFSET; }
uint32_t GmeFormatGbs() { return g_formatIdBase + GME_FMT_GBS_OFFSET; }
uint32_t GmeFormatGym() { return g_formatIdBase + GME_FMT_GYM_OFFSET; }
uint32_t GmeFormatHes() { return g_formatIdBase + GME_FMT_HES_OFFSET; }
uint32_t GmeFormatKss() { return g_formatIdBase + GME_FMT_KSS_OFFSET; }
uint32_t GmeFormatNsf() { return g_formatIdBase + GME_FMT_NSF_OFFSET; }
uint32_t GmeFormatNsfe() { return g_formatIdBase + GME_FMT_NSFE_OFFSET; }
uint32_t GmeFormatSap() { return g_formatIdBase + GME_FMT_SAP_OFFSET; }
uint32_t GmeFormatSpc() { return g_formatIdBase + GME_FMT_SPC_OFFSET; }
uint32_t GmeFormatVgm() { return StreamFormatVgmVgz; }

uint32_t ParseStreamFormatByGmeFile(const char* filenameUtf8)
{
    if ((filenameUtf8 == nullptr) || (filenameUtf8[0] == '\0')) {
        return StreamFormatUnknown;
    }

    gme_type_t musicType = nullptr;
    gme_err_t err = gme_identify_file(filenameUtf8, &musicType);
    if ((err != nullptr) || (musicType == nullptr)) {
        return StreamFormatUnknown;
    }

    return GmeStreamFmtByType(musicType);
}

uint32_t ParseStreamFormatByGmeStream(CDataStream* pStream)
{
    if (pStream == nullptr) {
        return StreamFormatUnknown;
    }

    const DataStreamStyle style = pStream->GetStyle();
    if (((style & dsStyleSeekable) == 0) || ((style & dsStyleTellPos) == 0)) {
        return StreamFormatUnknown;
    }

    constexpr std::size_t HEADER_SIZE = 64;
    std::array<unsigned char, HEADER_SIZE> header = {};

    const std::size_t oldPos = pStream->Tell();
    pStream->Seek(SeekBase::Begin, 0);
    const uint32_t readCount = pStream->Read(header.data(), static_cast<uint32_t>(header.size()));
    pStream->Seek(SeekBase::Begin, static_cast<long long>(oldPos));
    if (readCount <= 4) {
        return StreamFormatUnknown;
    }

    return GmeStreamFmtByName(gme_identify_header(header.data()));
}

uint32_t GmeStreamFmtByName(const char* name)
{
    if ((name == nullptr) || (name[0] == '\0')) {
        return StreamFormatUnknown;
    }

    if (std::strcmp(name, "AY") == 0) {
        return GmeFormatAy();
    }
    if (std::strcmp(name, "GBS") == 0) {
        return GmeFormatGbs();
    }
    if (std::strcmp(name, "GYM") == 0) {
        return GmeFormatGym();
    }
    if (std::strcmp(name, "HES") == 0) {
        return GmeFormatHes();
    }
    if (std::strcmp(name, "KSS") == 0) {
        return GmeFormatKss();
    }
    if (std::strcmp(name, "NSF") == 0) {
        return GmeFormatNsf();
    }
    if (std::strcmp(name, "NSFE") == 0) {
        return GmeFormatNsfe();
    }
    if (std::strcmp(name, "SAP") == 0) {
        return GmeFormatSap();
    }
    if (std::strcmp(name, "SPC") == 0) {
        return GmeFormatSpc();
    }
    if ((std::strcmp(name, "VGM") == 0) || (std::strcmp(name, "VGZ") == 0)) {
        return GmeFormatVgm();
    }

    return StreamFormatUnknown;
}

const char* GmeStreamFormatName(uint32_t streamFmt)
{
    if (streamFmt == GmeFormatAy()) {
        return "AY";
    }
    if (streamFmt == GmeFormatGbs()) {
        return "GBS";
    }
    if (streamFmt == GmeFormatGym()) {
        return "GYM";
    }
    if (streamFmt == GmeFormatHes()) {
        return "HES";
    }
    if (streamFmt == GmeFormatKss()) {
        return "KSS";
    }
    if (streamFmt == GmeFormatNsf()) {
        return "NSF";
    }
    if (streamFmt == GmeFormatNsfe()) {
        return "NSFE";
    }
    if (streamFmt == GmeFormatSap()) {
        return "SAP";
    }
    if (streamFmt == GmeFormatSpc()) {
        return "SPC";
    }
    if (streamFmt == GmeFormatVgm()) {
        return "VGM/VGZ";
    }

    return "Game music";
}
