#ifndef LIBARCHIVE_API_H
#define LIBARCHIVE_API_H

#include <cstddef>

extern "C" {
struct archive;
struct archive_entry;

archive* archive_read_new(void);
int archive_read_support_filter_all(archive* archiveHandle);
int archive_read_support_format_all(archive* archiveHandle);
int archive_read_open_filename_w(archive* archiveHandle, const wchar_t* filename, std::size_t blockSize);
int archive_read_next_header(archive* archiveHandle, archive_entry** entry);
long long archive_read_data(archive* archiveHandle, void* buffer, std::size_t size);
int archive_read_data_skip(archive* archiveHandle);
int archive_read_free(archive* archiveHandle);
}

constexpr int LIBARCHIVE_OK = 0;

#endif // LIBARCHIVE_API_H
