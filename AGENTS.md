# iPlayer Agent Guidelines

## Build Commands

### CMake Configuration
```bash
# Configure build (must use MSVC on Windows)
using "cmake-config-debug (MSVC C++20)" task, which defined in .vscode/tasks.json

# Build all
using "cmake-build-debug (MSVC C++20)" task, which defined in .vscode/tasks.json
```

### Running the Application
```bash
# Build output location: build/bin/
./build/bin/imPlayer.exe [options]

# Example: Play a file
./build/bin/imPlayer.exe --play --filename=audio.mp3
```

### Testing
No dedicated test framework configured. Manual testing via command-line interface.

---

## Code Style Guidelines

### Naming Conventions
- **Classes**: CamelCase with `C` prefix (e.g., `CPlayback`, `CDecoderFactory`)
- **Member variables**: `m_` prefix + camelCase (e.g., `m_audioFmt`, `m_status`)
- **Functions**: PascalCase (e.g., `Create`, `UpdatePlayback`)
- **Local variables**: camelCase (e.g., `audioBuf`, `totalFrames`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `AUDIO_BUFFER_FRAMES`)
- **Enums**: PascalCase (e.g., `AudioDataFormat`, `Mp3Version`)
- **Private methods**: PascalCase (same as public)

### Include Ordering
```cpp
// 1. System/library headers
#include <iostream>
#include <thread>
#include <algorithm>

// 2. Project headers with quotes
#include "framework.h"
#include "Playback.h"
#include "UnicodeConvert.h"
```

### Formatting
- **Indentation**: 4 spaces (no tabs)
- **Braces**:
  - Functions/classes: Opening brace on new line
  - Loops/conditionals: Opening brace on same line
- **Line length**: No strict limit, but keep reasonable
- **Pointers**: `Type* var` or `Type *var` (consistent within file)

### Code Organization
```
include/
  core/       # Core audio utilities (AudioInfo, ConvertFormat, etc.)
  decoder/    # Decoder interfaces and factory
  encoder/    # Encoder interfaces and factory
  player/     # Playback engine and audio devices
src/
  core/       # Core implementations
  decoder/    # Decoder implementations
  encoder/    # Encoder implementations
  player/     # Player implementations (WASAPI, DirectSound)
```

### Error Handling
- Use `std::exception` for runtime errors
- Try-catch blocks in `main()` and high-level functions
- Check pointer validity with asserts where appropriate
- Example:
```cpp
try {
    parser.parse_check(argc, argv);
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
}
```

### Comments
- Inline comments: `// comment` (mostly Chinese in this codebase)
- Multi-line comments for major sections
- No strict documentation format required

---

## Key Dependencies

### External Libraries (via CMake find_package)
- **FFmpeg**: Audio/video decoding
- **libcdio**: CD audio reading
- **nlohmann/json**: JSON parsing for metadata
- **ICU**: Unicode conversion and normalization
- **libsndfile**: Audio file I/O
- **CLI11**: Command-line argument parsing

### Windows APIs
- **WASAPI**: Windows Audio Session API (low-latency)
- **DirectSound**: Legacy audio output
- **WinMM**: Multimedia APIs
- **MMCSS**: Multimedia Class Scheduler Service for real-time audio

### Compilation Settings
- **C++ Standard**: C++20
- **Compiler**: MSVC only (Windows-only project)
- **Warnings**: `/W3` with `/permissive-` for strict conformance
- **Preprocessor**:
  - `_CRT_SECURE_NO_WARNINGS`
  - `_UNICODE` / `UNICODE`

---

## Architecture Notes

### Decoder Pattern
Factory pattern for decoder selection (see `CDecoderFactory`):
- `StreamFormatMp3` → `CMp3Decoder`
- `StreamFormatFlac` → `CFfmpegDecoder`
- Format → decoder mapping via lambda registration

### Audio Pipeline
1. **CAudioSource**: File/playlist input
2. **CAudioDecoder**: Decode to PCM (MP3, FFmpeg, Native)
3. **CPlayback**: Playback state management
4. **CAudioDevice**: Audio output (WASAPI/DirectSound)

### Smart Pointer Usage
- `std::unique_ptr`: Exclusive ownership (decoders, audio devices)
- `std::shared_ptr`: Shared ownership (playback instances)
- `std::weak_ptr`: Break cycles (async callbacks)

---

## Important Constraints
- **Windows-only**: MSVC required, no cross-platform build
- **Need tests**: Automatically add test cases for new code
- **C++20 features**: Use `std::format`, `std::filesystem` where applicable
- **Audio threading**: Real-time constraints in audio callbacks
- **Unicode**: Project uses UTF-8 internally, Windows UTF-16 APIs wrapped
- **Compile**: Terminate the current task if the same compilation error occurs nine times consecutively
- **build**: Using the parameters of the "cmake-build-debug (MSVC C++20)" task in the tasks.json file of VSCode to initiate the CMake compilation verification
- **Code modification**: When modifying code involving the use of the DataStream interface, read the design philosophy of this interface from the datastream.txt file as part of the task description

