/*
20260526 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_81.txt
*/
#include <fstream>
#include <functional>

#include <ogg/ogg.h>

#include "UnicodeConvert.h"
#include "AudioInfo.h"
#include "OggFunc.h"

namespace {

uint32_t ParseOggStream(std::function<uint32_t(void*, uint32_t)> reader)
{
    ogg_sync_state oy = {};
    ogg_sync_init(&oy);

    uint32_t result = StreamFormatUnknown;
    const int CHUNK = 4096;
    bool done = false;

    while (!done)
    {
        char* writeBuf = ogg_sync_buffer(&oy, CHUNK);
        const uint32_t got = reader(writeBuf, CHUNK);
        if (got == 0) {
            break;
        }
        ogg_sync_wrote(&oy, static_cast<long>(got));

        ogg_page page = {};
        while (ogg_sync_pageout(&oy, &page) == 1)
        {
            if (ogg_page_bos(&page) == 0) {
                continue;
            }

            ogg_stream_state os = {};
            if (ogg_stream_init(&os, ogg_page_serialno(&page)) != 0) {
                continue;
            }

            ogg_stream_pagein(&os, &page);
            ogg_packet packet = {};
            while (ogg_stream_packetout(&os, &packet) == 1)
            {
                if ((packet.bytes >= 7) && (packet.packet != nullptr)
                    && (packet.packet[0] == 1)
                    && (packet.packet[1] == 'v')
                    && (packet.packet[2] == 'o')
                    && (packet.packet[3] == 'r')
                    && (packet.packet[4] == 'b')
                    && (packet.packet[5] == 'i')
                    && (packet.packet[6] == 's')) {
                    result = StreamFormatOgg;
                    done = true;
                    break;
                }
                done = true;
                break;
            }

            ogg_stream_clear(&os);
            if (done) {
                break;
            }
        }
    }

    ogg_sync_clear(&oy);
    return result;
}

}

uint32_t ParseStreamFormatByLibogg(const char* filenameUtf8, CDataStream* pStream)
{
    if ((filenameUtf8 != nullptr) && (filenameUtf8[0] != '\0')) {
        const std::wstring filename = UTtf8ToUtf16Le(filenameUtf8);
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return StreamFormatUnknown;
        }

        return ParseOggStream([&file](void* dst, uint32_t bytes) -> uint32_t {
            file.read(static_cast<char*>(dst), static_cast<std::streamsize>(bytes));
            return static_cast<uint32_t>(file.gcount());
        });
    }

    if (pStream == nullptr) {
        return StreamFormatUnknown;
    }
    const DataStreamStyle style = pStream->GetStyle();
    if (((style & dsStyleSeekable) == 0) || ((style & dsStyleTellPos) == 0)) {
        return StreamFormatUnknown;
    }

    const std::size_t oldPos = pStream->Tell();
    pStream->Seek(SeekBase::Begin, 0);
    const uint32_t result = ParseOggStream([pStream](void* dst, uint32_t bytes) -> uint32_t {
        return pStream->Read(dst, bytes);
    });
    pStream->Seek(SeekBase::Begin, static_cast<long long>(oldPos));
    return result;
}
