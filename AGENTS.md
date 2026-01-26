# AGENTS.md - Developer Guidelines for YAMS

## Project Overview

YAMS (Yet Another Media Server) is a minimalistic media server for small venues built with C++.

**Core Technologies:**
- C++ (C++17 or later)
- Qt for GUI components
- FFmpeg/GStreamer for media handling
- OpenGL for compositing and rendering
- CMake for build system
- GoogleTest/Catch2 for testing

**Key Features:**
- Support for 1-3 layers
- Linux and Windows compatibility
- Simple compositing: opacity, playback speed, transitions
- OSC, sACN, and ArtNet control protocols

## Build Commands

### Initial Setup
```bash
mkdir build && cd build
cmake ..
```

### Build
```bash
# Full build
cmake --build .

# Parallel build (faster)
cmake --build . -j$(nproc)

# Release build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .
```

### Clean Build
```bash
rm -rf build && mkdir build && cd build && cmake .. && cmake --build .
```

## Test Commands

### Run All Tests
```bash
# From build directory
ctest

# With verbose output
ctest --verbose

# Run tests in parallel
ctest -j$(nproc)
```

### Run Single Test
```bash
# Run specific test by name
ctest -R TestName

# Run specific test executable directly
./build/tests/test_media_layer

# Run with GoogleTest filter
./build/tests/test_media_layer --gtest_filter=MediaLayerTest.OpacityControl
```

### Test with Coverage (if configured)
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
cmake --build .
ctest
# Generate coverage report (requires lcov/gcovr)
```

## Linting and Formatting

### ClangFormat
```bash
# Format single file
clang-format -i src/media_layer.cpp

# Format all source files
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting without modifying
clang-format --dry-run --Werror src/media_layer.cpp
```

### Clang-Tidy
```bash
# Lint single file
clang-tidy src/media_layer.cpp -- -Iinclude

# Lint all files
find src -name "*.cpp" | xargs clang-tidy -p build
```

## Code Style Guidelines

### File Organization
- **Headers**: `src/yams/*.hpp`
- **Implementation**: `src/yams/*.cpp`
- **Tests**: `tests/*_test.cpp`
- **Shaders**: `resources/shaders/*.glsl`

### Naming Conventions
- **Classes/Structs**: `PascalCase` (e.g., `MediaLayer`, `CompositeEngine`)
- **Functions/Methods**: `camelCase` (e.g., `setOpacity()`, `playMedia()`)
- **Variables**: `camelCase` (e.g., `layerCount`, `currentFrame`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_LAYERS`, `DEFAULT_OPACITY`)
- **Private members**: prefix with `d_` (e.g., `d_opacity`, `d_frameBuffer`)
- **Files**: `PascalCase` (e.g., `MediaLayer.cpp`, `OSCController.hpp`)

### Import/Include Order
1. Corresponding header (for .cpp files)
2. C system headers (`<cstdio>`, `<cmath>`)
3. C++ standard library (`<vector>`, `<memory>`, `<string>`)
4. Third-party libraries (`<QApplication>`, `<GL/gl.h>`)
5. Project headers (`"yams/MediaLayer.hpp"`)

Use blank lines between groups. Use angle brackets for system/external includes, quotes for project includes.

```cpp
#include "yams/MediaLayer.hpp"

#include <cmath>
#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include "yams/Compositor.hpp"
#include "yams/OSCHandler.hpp"
```

### Type Usage
- Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers
- Use `const` and `constexpr` where applicable
- Use `auto` for iterator types and obvious type deduction
- Avoid `using namespace` in headers; acceptable in .cpp files for std
- Use fixed-width types for platform-independent code (`int32_t`, `uint8_t`)
- Use Qt types when interfacing with Qt APIs (`QString`, `QList`)

### Error Handling
- Use exceptions for exceptional conditions
- Use Qt's error handling (signals/slots) for asynchronous operations
- Log errors appropriately (consider Qt's logging categories)
- Validate input parameters, especially from network protocols (OSC/sACN/ArtNet)
- Use `std::optional` for values that may not exist
- Document expected exceptions in function comments

```cpp
// Good: Clear error handling
std::optional<MediaFrame> loadFrame(const QString& path) {
    if (!QFile::exists(path)) {
        qWarning() << "Media file not found:" << path;
        return std::nullopt;
    }
    // ... load frame
}
```

### Modern C++ Practices
- Use RAII for resource management
- Prefer stack allocation over heap when possible
- Use `nullptr` instead of `NULL` or `0`
- Use range-based for loops when appropriate
- Use `enum class` instead of plain `enum`
- Mark single-argument constructors as `explicit`
- Use `override` and `final` keywords appropriately
- Avoid manual memory management; use smart pointers

### Documentation
- Document public APIs with Doxygen-style comments
- Explain "why" in comments, not "what" (code shows what)
- Document thread safety and ownership semantics
- Include usage examples for complex APIs

## Performance Considerations

- Profile before optimizing (use `perf`, `gprof`, or Qt Creator profiler)
- Minimize OpenGL state changes
- Use Qt's signal/slot queued connections for cross-thread communication
- Consider memory alignment for real-time media processing
- Batch media operations where possible
- Use Qt's object pool patterns for frequently allocated objects

## Platform-Specific Notes

### Linux
- Test with both X11 and Wayland
- Verify PulseAudio/ALSA compatibility

### Windows
- Use MinGW or MSVC compiler
- Test DirectShow integration if using FFmpeg

## Git Workflow

- Write clear, concise commit messages
- Keep commits atomic and focused
- Test before committing
- Avoid committing build artifacts or IDE files
