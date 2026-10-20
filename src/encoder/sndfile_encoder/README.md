# libsndfile encoder plugin

Build target: `sndfile_encoder`. Output: `bin/plugins/sndfile_encoder.opeplug`.
The plugin links the vcpkg `SndFile` target and exports the functions in
`sndfile_encoder.def` with C linkage and `WINAPI`.

## Lifecycle

1. Query formats and format-specific parameter definitions.
2. `Plug_OnInitialize` allocates a context whose first member is
   `AudioEncodeContextHeader` (`size` is the size of this header).
3. Optionally set metadata, then call `Plug_OpenEncoder`. The output stream is
   host-owned and must be empty, writable, seekable and able to report its position.
   `mediaStreamIdx` must be zero. On success, `hdr.transFmt` describes the preferred
   PCM input storage, rate and channel count.
4. `Plug_StartEncoding` selects the actual interleaved input PCM format. Rate,
   channel count and layout must match the preferred format; there is no resampling
   or channel mixing. `Plug_EncodeFrames` returns the number of frames accepted.
5. `Plug_StopEncoding` finalizes the file, including compressed data and headers.
   Start cannot resume a finalized file. `Plug_CloseEncoder` closes the controller
   and clears `hdr.transFmt`; the context can then open a new output stream.
6. `Plug_OnRelease` destroys the context, finalizing an open file if necessary.
   The host must keep the stream alive until Close or Release finishes.

Integer status results use zero for success and a negative value for failure.
`Plug_GetErrMessage` retrieves the current thread's last error, including errors
from void functions and partial writes. Exceptions from stream callbacks do not
cross the libsndfile C boundary.

## Parameters

Parameter names and `EncoderFormatDefine` are shared core headers. Missing
parameters use the same format-specific defaults returned by the query function.

- `CodeFormat`: an `AudioDataFormat` value describing the output encoding family
  (integer PCM, floating PCM, Vorbis, Opus, ADPCM, ALAC, etc.).
- `BitsPerSample`: output PCM precision, or preferred input precision for a
  compressed codec. For integer PCM, the requested precision selects the PCM
  subtype; for floating PCM, only 32 and 64 are valid.
- `SampleRates`, `Channels`: output rate and channel count. Container/codec
  restrictions are additionally checked by libsndfile; a query's options are not
  a promise that every Cartesian combination is valid.
  `Channels` defaults to 2, both in queried definitions and when omitted at Open.
  Mono-only formats (such as WVE) require an explicit channel count of 1
  (`--channel=1` in the conversion CLI).
- `SndfileSubtype`: zero selects the codec from the above parameters. A nonzero
  value explicitly selects one of the native libsndfile subtypes returned by the
  query, including codecs without a matching `AudioDataFormat` enum value.
- `CompressLevel`: FLAC compression, 0 through 8.
- `Quality`: Ogg/MPEG encoding quality, 0.0 through 1.0.
- `ExtWav`: optional WAV compatibility flag selecting WAVEFORMATEXTENSIBLE.

24-bit output requests `PCM_S32` input: samples are left-aligned in 32-bit words,
`bitsPerSample` is 32 and `blockAlign` is `4 * numChannels`. Start also accepts
other supported interleaved PCM storage types with matching rate and channels.
Eight-bit input is widened to signed 16-bit; `PCM_64` input is normalized through
double precision. libsndfile has no signed 64-bit integer **output** subtype, so
that output combination is rejected; 64-bit floating output is supported.
Packed 24-bit and planar input are not accepted.

## Format and metadata coverage

The catalog maps all 25 single-stream major formats in libsndfile 1.2.2, including
WAV/WAVEX/W64/RF64, AIFF, AU, RAW, FLAC, Ogg, MP3, CAF, MAT4/MAT5 and the legacy
sample formats. Codec availability is queried from the installed library.
SD2 is excluded because it needs a separate named resource fork; MPEG layers I
and II are excluded because libsndfile only decodes those layers.
The libsndfile 1.2.2 AIFF/DWVW12 combination is also filtered: that version
advertises it but does not initialize a writer for it.

Title, artist, album, comment, date, track number, genre and software tags are
forwarded as UTF-8 libsndfile strings. Metadata support varies by container;
unsupported optional fields are ignored by the library. Set metadata before
Start for formats that write it in their opening headers.
