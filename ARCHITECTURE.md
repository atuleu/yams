# YAMS Architecture & Design Decisions

This document captures the architectural decisions, technology stack choices, and design rationale for YAMS (Yet Another Media Server).

## Project Scope & Philosophy

YAMS is a **minimalistic, purpose-built media server** for small venue productions. The design philosophy is:

- **One-size-fits-all**: No plugin system, bundled solution with opinionated defaults
- **Limited scope**: 1-3 video layers, simple compositing (opacity, speed, transitions)
- **Hardware diversity**: Support AMD/Intel/NVIDIA GPUs, integrated graphics, and Raspberry Pi
- **Production-ready**: Windows as primary deployment platform, robust CI/CD pipeline
- **Simple is better**: Avoid over-engineering for features we don't need

**Target use cases:**
- Display 1-2 video files or static images
- Text overlay capabilities
- Control via lighting protocols (OSC, sACN, ArtNet)
- Integration with existing lighting software in production environments

**Out of scope:**
- Complex effects pipeline (use a more complete media server / VJ-ing software)
- Plugin architecture or extensibility beyond core features
- Multi-machine synchronized playback

## Technology Stack

### Core Technologies

| Component         | Choice         | Version   |
|-------------------|----------------|-----------|
| **Language**      | C++            | C++17     |
| **GUI Framework** | Qt             | 6.5.3+    |
| **Media Backend** | GStreamer      | 1.22+     |
| **Graphics API**  | OpenGL         | 3.3 Core+ |
| **Build System**  | CMake          | 3.20+     |
| **Testing**       | GoogleTest     | Latest    |
| **CI/CD**         | GitHub Actions | -         |

### Key Stack Decisions

#### 1. GStreamer vs FFmpeg → **GStreamer (Final)**

**Rationale:**
- ✅ **Hardware acceleration**: First-class support across platforms
  - Windows: D3D11VA via `d3d11` plugin
  - Linux: VA-API for Intel/AMD, NVDEC for NVIDIA
  - Raspberry Pi: V4L2 hardware decoding
- ✅ **Multi-stream parallelism**: Each pipeline auto-manages threading
- ✅ **"Battery included"**: `decodebin` auto-detects best decoder for hardware
- ✅ **Hardware diversity**: Auto-adapts to AMD/Intel/NVIDIA/RPi without code changes
- ✅ **Prior experience**: Used successfully in artemis, charis, VerTIGo projects
- ✅ **Pipeline flexibility**: Simple to chain decode → color conversion → upload to GL

**Trade-offs accepted:**
- Larger deployment footprint (~100-150MB vs ~30MB for FFmpeg) - **not a concern**
- Plugin dependency management - **mitigated by bundled deployment**
- GLib event loop integration - **solved via polling approach (see below)**

**FFmpeg was rejected because:**
- Manual hardware acceleration per platform (DXVA2, D3D11VA, VA-API, etc.)
- Manual threading for multi-stream decoding
- More code to achieve same functionality
- Weaker Raspberry Pi support

#### 2. Qt Event Loop Integration with GStreamer

**Challenge**: GStreamer's message bus typically uses GLib main loop, but we use Qt's event loop.

**Solution**: **QTimer polling approach**
```cpp
QTimer* gstTimer = new QTimer(this);
connect(gstTimer, &QTimer::timeout, [bus]() {
    while (GstMessage* msg = gst_bus_pop(bus)) {
        handleMessage(msg);
        gst_message_unref(msg);
    }
});
gstTimer->start(16); // Poll every ~16ms (60Hz)
```

**Rationale:**
- ✅ Simple, proven, cross-platform
- ✅ No GLib dependency complications on Windows
- ✅ GstBus has internal message buffering (no message loss)
- ✅ 16ms polling latency is negligible for media playback
- ❌ Not "pure" event-driven (acceptable trade-off for simplicity)

**Rejected alternatives:**
- GLib main loop integration with Qt: Complex, fragile on Windows
- File descriptor watch: Linux-only, doesn't help Windows target

#### 3. Dependency Management → **Manual Download + CI Caching**

**Approach:**
- Qt: Install via `aqt` (Another Qt Installer) CLI tool
- GStreamer: Official MSVC `.msi` installer for Windows
- Both cached in GitHub Actions for fast rebuilds

**Rationale:**
- ✅ Only 2 dependencies (Qt + GStreamer) - no need for complex package manager
- ✅ vcpkg had issues in past projects - avoid
- ✅ GitHub Actions caching is simple and effective
- ✅ Predictable, controlled dependency versions

**Windows DLL bundling strategy:**
```
yams/
├── yams.exe
├── Qt6Core.dll, Qt6Gui.dll, Qt6OpenGL.dll
├── gstreamer-1.0-0.dll, glib-2.0-0.dll
└── lib/gstreamer-1.0/
    ├── gstcoreelements.dll
    ├── gstd3d11.dll  # Windows hardware accel
    └── ... (codec plugins)
```
- Use `windeployqt.exe` for Qt DLLs (automated)
- Script GStreamer DLL copying in CMake/CI
- Single-directory deployment (simple, effective)

#### 4. Testing Strategy → **GoogleTest + Qt Integration**

**Framework**: GoogleTest for all tests (unit + integration)

**Rationale:**
- ✅ Prior successful integration with Qt GUI testing (myrmidon project)
- ✅ Works with CTest for unified test running
- ✅ Excellent CI integration and reporting

**Test categories:**
1. **Unit tests**: Pure logic, no Qt widgets (run everywhere)
2. **Integration tests**: Qt widgets + GUI interactions
   - Linux: Run with Xvfb (headless display)
   - Windows: Native display on Windows runners
3. **OpenGL tests**: May require software rendering in CI
   - GStreamer falls back to software decoding (no GPU in CI)
   - Validates logic, but hardware acceleration tested manually

#### 5. CI/CD Strategy → **GitHub Actions with Windows Runners**

**Pipeline:**
- **Linux build**: Development platform, integration tests with Xvfb
- **Windows build**: Primary production target, native tests, packaging

**GitHub Actions limits (public repo):**
- ✅ Unlimited minutes for public repositories
- ✅ 2-core CPU, 7GB RAM, 14GB SSD (sufficient)
- ✅ Max 6 hours per job (more than enough)
- ❌ No GPU (hardware decoding tested manually on real hardware)

**Packaging:**
- Windows: `.zip` artifact with `.exe` + all DLLs
- Linux: AppImage or tarball (lower priority)

#### 6. Platform Priority → **Windows First, Linux for Development**

**Windows:**
- **Primary production platform** - all features must work on Windows
- Professional lighting software runs on Windows
- MSVC compiler (2019 or later)
- D3D11VA hardware acceleration

**Linux:**
- Development platform (developer familiar with Linux)
- VA-API hardware acceleration (Intel/AMD)
- CI testing environment
- Secondary deployment target

**Hardware targets:**
- AMD integrated GPU (VA-API on Linux, D3D11VA on Windows)
- Intel mini PC (VA-API on Linux, D3D11VA on Windows)
- NVIDIA discrete GPU (NVDEC/NVENC on both platforms)
- Raspberry Pi (V4L2 hardware decoding via GStreamer)

**This diversity is a key reason for choosing GStreamer** - auto-detection of hardware capabilities.

## Application Architecture

### High-Level Structure

```
┌─────────────────────────────────────────────────────────────┐
│                     YAMS Application                        │
├─────────────────────────────────────────────────────────────┤
│  Qt Main Event Loop (QApplication)                          │
│    ├─ QTimer (16ms) → Poll GStreamer message buses          │
│    ├─ Control Window (QMainWindow)                          │
│    │    ├─ Asset management UI                              │
│    │    ├─ Control bindings configuration                   │
│    │    ├─ Playback controls                                │
│    │    └─ Preview widget (QOpenGLWidget - optional)        │
│    └─ Output Window (QOpenGLWindow)                         │
│         └─ Fullscreen compositor (1-3 layers)               │
├─────────────────────────────────────────────────────────────┤
│  GStreamer Pipelines (1-3 instances)                        │
│    ├─ Pipeline 1: uridecodebin → glupload → appsink         │
│    ├─ Pipeline 2: uridecodebin → glupload → appsink         │
│    └─ Pipeline 3: uridecodebin → glupload → appsink         │
├─────────────────────────────────────────────────────────────┤
│  OpenGL Compositor (3.3 Core)                               │
│    ├─ Layer textures (from GStreamer)                       │
│    ├─ Fragment shaders (opacity, blend modes)               │
│    └─ Transition effects (crossfade, etc.)                  │
├─────────────────────────────────────────────────────────────┤
│  Control Protocol Handlers                                  │
│    ├─ OSC Server (UDP, /layer/N/opacity, etc.)              │
│    ├─ sACN Receiver (DMX channel mapping)                   │
│    └─ ArtNet Receiver (DMX channel mapping)                 │
└─────────────────────────────────────────────────────────────┘
```

### Window Architecture

**Dual-window design:**
1. **Control Window** (`QMainWindow`):
   - Always visible during operation
   - Asset list, playback controls, binding configuration
   - Real-time feedback of current state
   - Optional small preview of output

2. **Output Window** (`QOpenGLWindow`):
   - Dedicated fullscreen display for production
   - Separate top-level window (can be on secondary monitor)
   - Direct rendering (no extra framebuffer copy overhead)
   - This is where compositor renders final output

**Rationale for dual-window:**
- ✅ Real-life production scenario: operators need control feedback + output display
- ✅ `QOpenGLWindow` for output = better performance (direct rendering)
- ✅ Separation of concerns: GUI logic vs rendering logic
- ❌ No headless mode (acceptable - GUI always required for this use case)

### OpenGL Integration

**Approach:**
- Control window: `QMainWindow` with optional `QOpenGLWidget` preview
- Output window: `QOpenGLWindow` (standalone, fullscreen-capable)
- OpenGL 3.3 Core profile (maximum compatibility)

**Compositing pipeline:**
1. GStreamer decodes video to GPU memory (D3D11/VA-API/etc.)
2. `glupload` element uploads frame to OpenGL texture
3. `appsink` delivers texture handle to Qt application
4. Fragment shader composites layers with opacity/blend modes
5. Render to framebuffer in `QOpenGLWindow`

### Threading Model

**High-level threading:**
- **Main thread**: Qt event loop, UI, OpenGL rendering (output window)
- **GStreamer threads**: Automatic (1 pipeline = 1+ internal threads)
- **Protocol handlers**: Likely in main thread (OSC/sACN/ArtNet are UDP, non-blocking)

**Synchronization:**
- GStreamer messages polled via QTimer (main thread)
- Control commands (OSC/sACN/ArtNet) → Qt signals → main thread actions
- OpenGL rendering stays on main thread (Qt requirement)

**This will be refined during implementation** - keeping it simple initially.

## Project Structure

```
yams/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # Project overview
├── AGENTS.md                   # Developer guidelines
├── ARCHITECTURE.md             # This file
├── LICENSE                     # TBD
├── .github/
│   └── workflows/
│       ├── build-linux.yml     # Linux CI build + tests
│       └── build-windows.yml   # Windows CI build + tests + packaging
├── src/
│   └── yams/                   # Layout to be defined
├── tests/                      # E2E tests here. Unit test defined in src/
├── resources/
│   ├── shaders/                # GLSL vertex/fragment shaders
│   └── icons/                  # UI icons (if needed)
└── scripts/
    └── ci/                     # CI helper scripts (DLL bundling, etc.)
```

## Development Phases

See [ROADMAP.md](ROADMAP.md) for detailed phase breakdown, task lists, and current progress.

**High-level phase overview**:
- **Phase 0**: Foundation - Stack validation (Qt + GStreamer + OpenGL) and CI/CD setup
- **Phase 1**: Single layer media playback with hardware acceleration
- **Phase 2**: Multi-layer composition, opacity control, and transitions
- **Phase 3**: Live control architecture with keyboard bindings and UI feedback
- **Phase 4**: GUI styling for live events (custom dark theme)
- **Phase 5**: Protocol support (OSC, sACN, MIDI, ArtNet)
- **Phase 6**: CITP support for lighting console interoperability (GrandMA 3/2)

Each phase includes full Linux and Windows testing/deployment. Flatpak packaging added in Phase 0.

## Design Constraints & Trade-offs

### Accepted Constraints
1. **No plugin system**: Monolithic, bundled solution
2. **1-3 layers maximum**: No arbitrary layer count
3. **Simple compositing**: Opacity, speed, basic transitions only
4. **Windows-first**: Linux is secondary deployment target
5. **GUI required**: No headless mode (control window always visible)

### Accepted Trade-offs
1. **GStreamer footprint**: Larger deployment size for hardware accel benefits
2. **QTimer polling**: Not "pure" event-driven, but simple and reliable
3. **Manual dependency management**: Simpler than package managers for 2 dependencies
4. **No CI GPU testing**: Hardware acceleration tested manually on real hardware

### Non-Negotiable Requirements
1. **Hardware acceleration**: Must work on AMD/Intel/NVIDIA/RPi
2. **Windows reliability**: Primary production platform must be rock-solid
3. **Real-time control**: OSC/sACN/ArtNet must be responsive (low latency)
4. **Dual-window design**: Control window + output window always present

## Hardware Acceleration Strategy

### Per-Platform Approach

| Platform             | Hardware Accel  | GStreamer Plugin    | Notes                                         |
|----------------------|-----------------|---------------------|-----------------------------------------------|
| **Windows (AMD)**    | D3D11VA         | `d3d11`             | Primary production path                       |
| **Windows (Intel)**  | D3D11VA         | `d3d11`             | Well-supported                                |
| **Windows (NVIDIA)** | D3D11VA / NVDEC | `d3d11` / `nvcodec` | Both work, prefer `d3d11` for consistency     |
| **Linux (AMD)**      | VA-API          | `vaapi`             | Mature, well-tested                           |
| **Linux (Intel)**    | VA-API          | `vaapi`             | Excellent support (prior experience)          |
| **Linux (NVIDIA)**   | NVDEC           | `nvcodec`           | Proprietary driver required                   |
| **Raspberry Pi**     | V4L2            | `v4l2`              | VideoCore GPU acceleration                    |

### Auto-Detection Strategy

GStreamer's `decodebin` automatically selects the best decoder:
```cpp
// Pseudo-code: No platform-specific code needed!
pipeline = gst_parse_launch("uridecodebin uri=file:///video.mp4 ! glupload ! appsink", NULL);
// GStreamer picks: d3d11h264dec on Windows, vaapih264dec on Linux, etc.
```

**Fallback**: If hardware acceleration unavailable, GStreamer falls back to software decoding (libav).

## Open Questions (To Be Resolved During Implementation)

1. **Text rendering**: Will use OpenGL-based text rendering.
2. **Transition effects**: Shader-based.
3. **Asset management**: GUI based.
4. **Control binding UI**: How to map OSC/sACN/ArtNet to layer parameters?
5. **Error handling**: How to gracefully handle missing codecs, invalid files, etc.?
6. **Performance targets**: What's acceptable frame rate at 1920x1080 with 3 layers?

These will be addressed during detailed planning and prototyping.

---

**Document Status**: Living document, updated as decisions are made.  
**Last Updated**: 2026-01-26  
**Next Review**: After each phase completion (see [ROADMAP.md](ROADMAP.md))
