# YAMS Development Roadmap

This document outlines the development phases for YAMS, with detailed task breakdown for each phase. Each phase includes Linux and Windows testing/deployment to ensure cross-platform compatibility throughout development.

**Current Phase**: Phase 0.2 (GStreamer Integration) - Starting
**Last Updated**: 2026-01-27

---

## Phase 0: Foundation - Stack Validation & CI/CD

**Goal**: Validate the complete technology stack (Qt + GStreamer + OpenGL) with minimal "Hello World" applications and establish CI/CD pipeline for Linux and Windows deployment from the start.

**Success Criteria**:
- ✅ CMake project builds on Linux and Windows in CI
- ✅ CI/CD pipeline produces Windows installer and Flatpak artifacts
- ✅ Qt application runs and displays window on both platforms
- ✅ GStreamer pipeline initializes and decodes test video
- ✅ OpenGL context created and renders basic geometry
- ✅ All three components work together (even if trivially)
- ✅ Basic tests run in CI for both platforms

### Phase 0.1: CMake + Qt Hello World + CI/CD Setup

**Goal**: Get minimal Qt app building in CI with Windows installer and Flatpak from day one.

**Tasks**:
- [x] Create CMakeLists.txt root project
- [x] Add Qt6 dependency detection (find_package)
- [x] Create minimal Qt application displaying "Hello YAMS" window
- [x] Add basic logging output (using slog++)
- [x] Add GoogleTest dependency and create sample test (tests build system)
- [x] **Linux CI**: Setup GitHub Actions workflow
  - [x] Install Qt6 and build dependencies
  - [x] Build project
  - [x] Run tests
  - [x] Cache dependencies for faster rebuilds
- [x] **Windows CI**: Setup GitHub Actions workflow
  - [x] Install Qt via aqt
  - [x] Build with MSVC
  - [x] Run tests
  - [x] Bundle NSIS Installer
  - [x] Cache Qt installation
- [x] **Flatpak CI**: Setup build workflow
  - [x] Create Flatpak manifest
  - [x] Build Flatpak in CI
  - [x] Produce Flatpak artifact
- [x] **Manual testing**: Download Windows installer artifact, verify it runs
- [x] **Manual testing**: Install Flatpak artifact, verify it runs

**Deliverables**:
- Minimal Qt application that displays a window
- CMake build system that finds Qt6
- Basic test infrastructure (GoogleTest + CTest)
- GitHub Actions CI for Linux (tests pass)
- GitHub Actions CI for Windows (produces .exe installer)
- GitHub Actions CI for Flatpak (produces .flatpak artifact)
- All three artifacts tested manually and working

**Phase 0.1 Complete When**: CI is green on all platforms, all three artifacts downloadable and runnable.

**Status**: ✅ **COMPLETE** (v0.1.0 - 2026-01-27)

---

### Phase 0.2: Add GStreamer Integration

**Goal**: Initialize GStreamer, create a simple pipeline, integrate with Qt event loop.

**Tasks**:
- [ ] Add GStreamer dependency detection in CMake (Linux and Windows)
- [ ] Initialize GStreamer in main()
- [ ] Create simple test pipeline (playbin with test video URI)
- [ ] Implement Qt event loop integration (QTimer polling GStreamer bus)
- [ ] Log GStreamer messages (state changes, errors, warnings)
- [ ] Add test: Verify GStreamer initializes without errors
- [ ] Add test: Verify pipeline can be created and transitions to PLAYING state
- [ ] **Linux CI**: Install GStreamer dev packages
- [ ] **Windows CI**: Install GStreamer MSVC runtime, cache installation
- [ ] **Windows CI**: Bundle GStreamer DLLs and plugins with executable
- [ ] **Flatpak**: Add GStreamer to Flatpak manifest
- [ ] Verify pipeline runs on all three artifacts (check logs)

**Deliverables**:
- GStreamer initialization and cleanup integrated into application
- Qt/GStreamer event loop integration (polling approach from ARCHITECTURE.md)
- Simple pipeline that can play a test video (even if not displayed yet)
- Tests verify GStreamer integration works
- All three CI artifacts updated and working with GStreamer

**Phase 0.2 Complete When**: GStreamer pipeline runs successfully on Linux, Windows, and Flatpak builds.

---

### Phase 0.3: Add OpenGL Rendering

**Goal**: Create OpenGL context, render basic geometry (triangle), verify on all platforms.

**Tasks**:
- [ ] Create QOpenGLWindow subclass
- [ ] Initialize OpenGL 3.3 Core context
- [ ] Implement basic rendering (colored triangle with vertex/fragment shader)
- [ ] Load and compile shaders from resource files
- [ ] Display window at 1280x720 resolution
- [ ] Add test: Verify OpenGL context creation succeeds
- [ ] Add test: Verify shader compilation succeeds
- [ ] **Linux**: Test on X11 (and Wayland if available)
- [ ] **Windows**: Verify OpenGL context creation (may use ANGLE as fallback)
- [ ] Update all CI artifacts to include shaders in resources
- [ ] Verify triangle renders on all platforms (visual check, no automated test yet)

**Deliverables**:
- OpenGL 3.3 context initialized in QOpenGLWindow
- Basic shader pipeline (vertex + fragment shader)
- Colored triangle renders on screen
- Tests verify OpenGL initialization works
- All CI artifacts render triangle successfully

**Phase 0.3 Complete When**: Triangle visible on Linux, Windows, and Flatpak builds.

---

### Phase 0.4: Integration - Qt + GStreamer + OpenGL Together

**Goal**: Combine all three components into dual-window architecture (control + output).

**Tasks**:
- [ ] Create dual-window setup:
  - [ ] Control window (QMainWindow) showing GStreamer status
  - [ ] Output window (QOpenGLWindow) rendering triangle
- [ ] GStreamer pipeline status updates control window label
- [ ] Both windows display simultaneously
- [ ] Graceful application shutdown (cleanup GStreamer, close GL contexts)
- [ ] Add test: Verify dual-window initialization
- [ ] Add test: Verify graceful shutdown (no crashes/leaks)
- [ ] **All platforms**: Verify both windows appear and work correctly
- [ ] Update CI to package complete dual-window application

**Deliverables**:
- Dual-window application architecture working
- GStreamer + Qt event loop + OpenGL all running together
- Clean application lifecycle (init + shutdown)
- Tests verify integration points
- All CI artifacts demonstrate full stack integration

**Phase 0.4 Complete When**: Dual-window app runs on all platforms with GStreamer status + OpenGL rendering.

---

**Phase 0 Complete When**: All sub-phases complete, CI green, all artifacts tested and working.

---

## Phase 1: Single Layer Media Playback

**Goal**: Display a single video file in OpenGL window with hardware-accelerated decoding, playback controls, and looping.

**Success Criteria**:
- ✅ Video file loads and plays at correct frame rate (60fps target)
- ✅ Hardware acceleration confirmed on target platforms
- ✅ Video frames rendered as OpenGL texture in output window
- ✅ Playback controls (play/pause/stop/seek) functional
- ✅ Looping works correctly
- ✅ Tests verify playback functionality

### Phase 1.1: GStreamer to OpenGL Pipeline

**Goal**: Decode video frames and upload to OpenGL textures.

**Tasks**:
- [ ] Create pipeline: uridecodebin → videoconvert → glupload → appsink
- [ ] Configure appsink to deliver GL texture handles (or raw frames for texture upload)
- [ ] Handle new-sample signal from appsink
- [ ] Share GL context between GStreamer and Qt (platform-specific research)
- [ ] Upload video frame to OpenGL texture
- [ ] Add test: Verify pipeline creates successfully with video file
- [ ] Add test: Verify frames are delivered from appsink
- [ ] **Linux**: Test with VA-API hardware decoding (Intel/AMD)
- [ ] **Windows**: Test with D3D11VA hardware decoding
- [ ] Verify frames appear as textures in OpenGL context
- [ ] Log which decoder GStreamer selects (for hardware accel validation)

**Deliverables**:
- GStreamer pipeline that decodes video to OpenGL textures
- Frame delivery mechanism from GStreamer to Qt/OpenGL
- Tests verify pipeline and frame delivery
- Hardware acceleration working (validated via logs)

---

### Phase 1.2: Video Texture Rendering

**Goal**: Render video frames as textures in output window.

**Tasks**:
- [ ] Create fullscreen quad geometry (two triangles)
- [ ] Write vertex shader for textured quad
- [ ] Write fragment shader for texture sampling
- [ ] Bind video texture and render quad in paintGL()
- [ ] Handle aspect ratio (letterbox/pillarbox vs stretch - make configurable)
- [ ] Synchronize frame updates with GL rendering loop
- [ ] Add test: Verify texture binding works
- [ ] Measure rendering performance (FPS counter)
- [ ] Ensure 60fps at 1080p playback
- [ ] **All platforms**: Verify video displays correctly

**Deliverables**:
- Video playback displayed in OpenGL window
- Correct aspect ratio handling
- Smooth 60fps playback at 1080p
- Tests verify rendering functionality
- Performance meets targets

---

### Phase 1.3: Playback Controls

**Goal**: Implement play, pause, stop, seek, and loop controls.

**Tasks**:
- [ ] Implement play/pause (GStreamer state transitions)
- [ ] Implement stop (reset to beginning)
- [ ] Implement seek (position control via GStreamer seek events)
- [ ] Implement looping (detect EOS message, seek to start)
- [ ] Add UI controls to control window:
  - [ ] Play/Pause/Stop buttons
  - [ ] Seek slider (shows position, allows scrubbing)
  - [ ] Loop checkbox
  - [ ] Time display (current / duration)
- [ ] Connect UI to GStreamer via signals/slots
- [ ] Handle errors gracefully (missing file, unsupported codec, etc.)
- [ ] Add tests: Verify play/pause state transitions
- [ ] Add tests: Verify seek functionality
- [ ] Add tests: Verify loop behavior
- [ ] Add tests: Error handling (invalid file)

**Deliverables**:
- Complete playback control functionality
- User interface for controlling playback
- Error handling for common failure cases
- Tests verify all playback features work

---

### Phase 1.4: Hardware Acceleration Validation

**Goal**: Confirm hardware decoding works on all target platforms.

**Tasks**:
- [ ] Add logging to identify selected decoder (vaapih264dec, d3d11h264dec, etc.)
- [ ] Test on Linux platforms:
  - [ ] Intel integrated GPU (VA-API)
  - [ ] AMD integrated GPU (VA-API)
  - [ ] NVIDIA discrete GPU (NVDEC) - if available
- [ ] Test on Windows platforms:
  - [ ] AMD GPU (D3D11VA)
  - [ ] Intel GPU (D3D11VA)
  - [ ] NVIDIA GPU (D3D11VA or NVDEC)
- [ ] Test on Raspberry Pi (V4L2) - if available
- [ ] Measure CPU usage during playback (<20% for 1080p is target)
- [ ] Document findings (which platforms use hardware, which fall back to software)
- [ ] Update ARCHITECTURE.md if platform-specific issues found

**Deliverables**:
- Hardware acceleration confirmed on target platforms
- CPU usage measurements documented
- Platform-specific notes (if any)

---

**Phase 1 Complete When**: Single video plays smoothly with hardware acceleration, all controls work, tests pass on all platforms.

---

## Phase 2: Multi-Layer Composition & Transitions

**Goal**: Support 1-3 simultaneous video layers with opacity control, alpha blending, playback speed control, and crossfade transitions.

**Success Criteria**:
- ✅ 3 layers play simultaneously at 60fps (1080p)
- ✅ Per-layer opacity control (0.0 - 1.0)
- ✅ Proper alpha blending (back-to-front compositing)
- ✅ Crossfade transition between media on same layer
- ✅ Playback speed control per layer (0.5x - 2.0x)
- ✅ Tests verify all composition features

### Phase 2.1: Multi-Layer Architecture

**Goal**: Support 3 independent video layers, each with own pipeline.

**Tasks**:
- [ ] Refactor to support multiple media layers (1-3)
- [ ] Each layer has independent GStreamer pipeline
- [ ] Each layer outputs to separate OpenGL texture
- [ ] Manage layer lifecycle (create, destroy, update)
- [ ] Update control window to show controls for all 3 layers
- [ ] Add test: Create and destroy layers
- [ ] Add test: 3 layers with different videos play simultaneously
- [ ] Verify performance: 3 layers at 60fps

**Deliverables**:
- Multi-layer support (up to 3 layers)
- Independent pipeline per layer
- UI shows all 3 layers
- Tests verify multi-layer functionality
- Performance target met (3 layers @ 60fps)

---

### Phase 2.2: Opacity & Alpha Blending

**Goal**: Per-layer opacity control with correct alpha compositing.

**Tasks**:
- [ ] Add opacity parameter to each layer (0.0 - 1.0)
- [ ] Update fragment shader to apply per-layer opacity
- [ ] Implement back-to-front blending order (layer 1 → 2 → 3)
- [ ] Enable OpenGL blending (GL_BLEND with appropriate blend func)
- [ ] Add opacity sliders to UI (one per layer)
- [ ] Add test: Verify opacity values apply correctly
- [ ] Add test: Verify blending order (layer 3 on top)
- [ ] Visual verification: Overlapping layers blend correctly

**Deliverables**:
- Opacity control per layer
- Correct alpha blending and layer ordering
- UI controls for opacity
- Tests verify opacity and blending

---

### Phase 2.3: Playback Speed Control

**Goal**: Independent playback speed per layer (0.5x - 2.0x).

**Tasks**:
- [ ] Implement playback rate control in GStreamer (seek with rate parameter)
- [ ] Add speed parameter to each layer (0.5x - 2.0x range)
- [ ] Handle audio (mute when speed != 1.0, or disable audio entirely for simplicity)
- [ ] Add speed controls to UI (sliders per layer)
- [ ] Add test: Verify playback speed changes
- [ ] Visual verification: Different speeds on different layers

**Deliverables**:
- Playback speed control per layer
- UI controls for speed
- Tests verify speed functionality

---

### Phase 2.4: Crossfade Transitions

**Goal**: Smooth crossfade transition when changing media on a layer.

**Tasks**:
- [ ] Design transition mechanism (e.g., layer.transitionTo(newMedia, duration))
- [ ] Implement crossfade:
  - [ ] Load new media in background
  - [ ] Animate opacity: old media 1.0→0.0, new media 0.0→1.0
  - [ ] Swap to new media when transition completes
- [ ] Use Qt animation framework (QPropertyAnimation or QTimer)
- [ ] Add transition controls to UI
- [ ] Add test: Verify transition completes successfully
- [ ] Add test: Verify transition timing is correct
- [ ] Visual verification: Smooth crossfade over specified duration

**Deliverables**:
- Crossfade transition functionality
- UI controls to trigger transitions
- Tests verify transition behavior

---

### Phase 2.5: Performance Optimization & Validation

**Goal**: Ensure 60fps with 3 layers at 1080p, optimize if needed.

**Tasks**:
- [ ] Profile rendering performance (GPU/CPU usage)
- [ ] Measure frame rate with 3 active layers
- [ ] Optimize if below 60fps:
  - [ ] Cache compiled shaders
  - [ ] Minimize GL state changes
  - [ ] Optimize texture uploads
- [ ] Test on lower-end hardware (integrated GPU)
- [ ] Document performance benchmarks
- [ ] Add performance regression test (if feasible)

**Deliverables**:
- Performance meets 60fps target with 3 layers
- Optimizations applied (if needed)
- Performance benchmarks documented

---

**Phase 2 Complete When**: 3 layers with opacity, speed, and transitions work smoothly at 60fps on all platforms, tests pass.

---

## Phase 3: Live Control Architecture & UI Feedback

**Goal**: Establish modular control input system starting with keyboard bindings, with architecture to support OSC/sACN/ArtNet later. Provide real-time visual feedback of all layer states.

**Success Criteria**:
- ✅ Keyboard bindings control layers
- ✅ Control input system is modular (easy to add new input sources)
- ✅ Control window shows live feedback (layer status, current media, etc.)
- ✅ Configuration UI for mapping keys to actions
- ✅ Asset management (media library)
- ✅ Tests verify control system

### Phase 3.1: Control Abstraction Layer

**Goal**: Create modular control input system that supports multiple input sources.

**Tasks**:
- [ ] Design control abstraction (action-based system)
- [ ] Define control actions (PlayLayer1, SetOpacityLayer2, etc.)
- [ ] Create binding system (maps input events → actions → layer commands)
- [ ] Implement keyboard input handler
- [ ] Add test: Verify action dispatching works
- [ ] Add test: Keyboard events generate correct actions

**Deliverables**:
- Modular control input architecture
- Keyboard input handler integrated
- Tests verify control system

---

### Phase 3.2: Keyboard Bindings & Configuration

**Goal**: Default keyboard controls with user customization.

**Tasks**:
- [ ] Define default key mappings (layer select, play/pause, opacity, speed, etc.)
- [ ] Implement key handling
- [ ] Create binding configuration UI (view/edit key mappings)
- [ ] Save/load bindings from config file (JSON or INI)
- [ ] Add "Reset to defaults" functionality
- [ ] Add test: Verify keyboard controls trigger correct actions
- [ ] Add test: Verify custom bindings save/load correctly

**Deliverables**:
- Keyboard control with default bindings
- Configuration UI for customizing bindings
- Bindings persist across application restarts
- Tests verify keyboard and configuration

---

### Phase 3.3: Live Feedback UI

**Goal**: Real-time visual feedback of layer states in control window.

**Tasks**:
- [ ] Design control window layout (layer panels, status displays)
- [ ] Display per-layer information:
  - [ ] Current media name
  - [ ] Playback state (playing/paused/stopped)
  - [ ] Opacity value (numeric + slider)
  - [ ] Speed value (numeric + slider)
  - [ ] Timeline position (progress bar + time)
- [ ] Update UI in real-time (connect to layer state signals)
- [ ] Add visual indicator for focused layer
- [ ] Add test: Verify UI updates when layer state changes

**Deliverables**:
- Live feedback UI showing all layer states
- Real-time updates as layers change
- Tests verify UI updates correctly

---

### Phase 3.4: Asset Management (Media Library)

**Goal**: UI for managing media files and assigning to layers.

**Tasks**:
- [ ] Create media library UI (list of available media files)
- [ ] Add "Add Media" functionality (file picker)
- [ ] Store media paths in config/database
- [ ] Implement drag-and-drop or click-to-assign media to layers
- [ ] Display media metadata (duration, resolution, codec)
- [ ] Add test: Add/remove media from library
- [ ] Add test: Assign media to layer

**Deliverables**:
- Media library UI
- Add/remove/assign media functionality
- Tests verify asset management

---

**Phase 3 Complete When**: Keyboard controls work, modular control architecture in place, live feedback UI functional, asset management working, tests pass.

---

## Phase 4: GUI Styling for Live Events

**Goal**: Apply custom dark theme optimized for live event operation, ensuring consistent appearance across platforms.

**Success Criteria**:
- ✅ Dark color scheme (near-black background, high-contrast text)
- ✅ Large, clear controls suitable for live operation
- ✅ Custom stylesheet works identically on Linux and Windows
- ✅ Professional appearance suitable for production use

### Phase 4.1: Design System & Color Palette

**Goal**: Define visual design system (colors, typography, spacing).

**Tasks**:
- [ ] Define color palette (background, surface, primary, secondary, text colors)
- [ ] Define typography (fonts, sizes for different UI elements)
- [ ] Define spacing scale (consistent padding/margins)
- [ ] Document design system (can be in ARCHITECTURE.md or separate doc)

**Deliverables**:
- Color palette defined
- Typography and spacing guidelines
- Design system documented

---

### Phase 4.2: Qt Stylesheet Implementation

**Goal**: Apply dark theme via Qt stylesheets.

**Tasks**:
- [ ] Create Qt stylesheet (.qss file) with dark theme
- [ ] Style all widget types (buttons, sliders, labels, etc.)
- [ ] Include hover/pressed/disabled states
- [ ] Apply stylesheet globally in application
- [ ] Test on Linux and Windows (ensure identical appearance)

**Deliverables**:
- Dark theme stylesheet
- Consistent appearance on all platforms

---

### Phase 4.3: Custom Widgets & Polish

**Goal**: Create custom widgets for professional live event control.

**Tasks**:
- [ ] Design and implement custom widgets as needed (faders, indicators, etc.)
- [ ] Create responsive layout (works at different window sizes)
- [ ] Add icons to buttons and UI elements
- [ ] Create application icon
- [ ] Ensure sufficient contrast for low-light environments
- [ ] Add tooltips to all controls

**Deliverables**:
- Custom widgets (if needed)
- Responsive layout
- Icons and visual polish
- Professional appearance suitable for live events

---

**Phase 4 Complete When**: Dark theme applied, UI looks professional and consistent, suitable for live event use on all platforms.

---

## Phase 5: Protocol Support - OSC / sACN / MIDI / ArtNet

**Goal**: Add network protocol support for external control from lighting consoles and control software.

**Success Criteria**:
- ✅ OSC commands control all layer functions
- ✅ sACN (DMX over Ethernet) controls via channel mapping
- ✅ MIDI control support (optional)
- ✅ ArtNet controls via channel mapping
- ✅ All protocols work alongside existing controls
- ✅ Tests verify protocol implementations

### Phase 5.1: OSC Protocol

**Goal**: OSC server for remote control.

**Tasks**:
- [ ] Research/choose OSC library (liblo, oscpack, or custom)
- [ ] Implement OSC server (UDP listener on configurable port)
- [ ] Define OSC address space (/layer/N/play, /layer/N/opacity, etc.)
- [ ] Integrate OSC input with control abstraction layer
- [ ] Add OSC configuration UI (port, enable/disable)
- [ ] Add test: Send OSC message, verify action triggered
- [ ] Test with real OSC client (TouchOSC, QLab, or CLI tool)

**Deliverables**:
- OSC protocol support
- OSC configuration UI
- OSC address space documented
- Tests verify OSC functionality

---

### Phase 5.2: sACN (E1.31) Protocol

**Goal**: sACN receiver for DMX-style control.

**Tasks**:
- [ ] Research/choose sACN library or implement protocol
- [ ] Implement sACN receiver (multicast UDP)
- [ ] Define configurable DMX channel mapping
- [ ] Integrate sACN input with control abstraction layer
- [ ] Add sACN configuration UI (universe, channel mappings)
- [ ] Add test: Send sACN packet, verify action triggered
- [ ] Test with real sACN sender (lighting console or software)

**Deliverables**:
- sACN protocol support
- sACN configuration UI
- Channel mapping system
- Tests verify sACN functionality

---

### Phase 5.3: MIDI Protocol (Optional)

**Goal**: MIDI input support (if deemed useful).

**Tasks**:
- [ ] Decide if MIDI support is needed
- [ ] If yes: Research/choose MIDI library
- [ ] Implement MIDI input handler
- [ ] Define MIDI mapping (notes, CC, program change)
- [ ] Integrate with control abstraction layer
- [ ] Add MIDI configuration UI
- [ ] Add tests for MIDI functionality

**Deliverables**:
- MIDI support (if implemented)
- MIDI configuration UI
- Tests verify MIDI functionality

---

### Phase 5.4: ArtNet Protocol

**Goal**: ArtNet receiver for DMX-style control.

**Tasks**:
- [ ] Research/choose ArtNet library or implement protocol
- [ ] Implement ArtNet receiver (UDP, port 6454)
- [ ] Define configurable DMX channel mapping (similar to sACN)
- [ ] Integrate ArtNet input with control abstraction layer
- [ ] Add ArtNet configuration UI (universe, channel mappings)
- [ ] Add test: Send ArtNet packet, verify action triggered
- [ ] Test with real ArtNet sender (lighting console or software)

**Deliverables**:
- ArtNet protocol support
- ArtNet configuration UI
- Channel mapping system
- Tests verify ArtNet functionality

---

### Phase 5.5: Protocol Configuration & Testing

**Goal**: Unified protocol configuration UI, integration testing.

**Tasks**:
- [ ] Create unified settings dialog for all protocols
- [ ] Save/load protocol configurations
- [ ] Test multiple protocols simultaneously (OSC + sACN, etc.)
- [ ] Stress test: Rapid updates from protocols (verify no lag/drops)
- [ ] Measure and document control latency (protocol input → visual change)
- [ ] Integration tests for all protocols

**Deliverables**:
- Unified protocol configuration UI
- All protocols work simultaneously
- Latency measurements documented
- Integration tests verify protocol functionality

---

**Phase 5 Complete When**: All protocols (OSC, sACN, ArtNet, optionally MIDI) control YAMS, configuration works, tests pass.

---

## Phase 6: CITP Support & Lighting Console Interoperability

**Goal**: Implement CITP (Controller Interface Transport Protocol) for seamless integration with professional lighting consoles, specifically GrandMA 3 and optionally GrandMA 2.

**Success Criteria**:
- ✅ CITP/MSEX (Media Server Extensions) implemented
- ✅ GrandMA 3 discovers YAMS on network
- ✅ Thumbnail streaming to console
- ✅ Media library visible in console UI
- ✅ Console can select media and control playback via CITP
- ✅ Tests verify CITP functionality
- ✅ (Optional) GrandMA 2 compatibility

### Phase 6.1: CITP Protocol Research & Planning

**Goal**: Understand CITP specification and plan implementation.

**Tasks**:
- [ ] Study CITP specification (identify required protocol layers)
- [ ] Research existing CITP libraries (or decide to implement from scratch)
- [ ] Install GrandMA 3 onPC for testing
- [ ] Create CITP implementation plan
- [ ] Define test cases for CITP functionality

**Deliverables**:
- CITP protocol understanding documented
- Implementation plan
- GrandMA 3 onPC setup for testing

---

### Phase 6.2: CITP/PINF - Peer Discovery

**Goal**: YAMS advertises itself on network for console discovery.

**Tasks**:
- [ ] Implement CITP/PINF listener (UDP multicast)
- [ ] Respond to PeerLocation requests
- [ ] Advertise YAMS as media server
- [ ] Add test: Verify PINF messages sent correctly
- [ ] Test: GrandMA 3 discovers YAMS

**Deliverables**:
- CITP peer discovery implemented
- GrandMA 3 sees YAMS on network
- Tests verify discovery functionality

---

### Phase 6.3: CITP/MSEX - Media Server Extensions

**Goal**: Media library browsing and layer control via CITP.

**Tasks**:
- [ ] Implement CITP/MSEX protocol
- [ ] Report layer status to console
- [ ] Send media library information to console
- [ ] Handle media selection from console
- [ ] Handle playback commands from console
- [ ] Add tests: Verify MSEX messages
- [ ] Test: Browse media library in GrandMA 3
- [ ] Test: Assign media from console

**Deliverables**:
- CITP/MSEX implementation
- Media library browsable from console
- Console can control playback
- Tests verify MSEX functionality

---

### Phase 6.4: Thumbnail Generation & Streaming

**Goal**: Send media thumbnails to console for visual browsing.

**Tasks**:
- [ ] Generate thumbnail images from video files (extract frame with GStreamer)
- [ ] Cache thumbnails locally
- [ ] Implement thumbnail streaming via CITP
- [ ] Add test: Verify thumbnail generation
- [ ] Test: Thumbnails appear in GrandMA 3

**Deliverables**:
- Thumbnail generation system
- Thumbnail streaming via CITP
- Thumbnails visible in console
- Tests verify thumbnail functionality

---

### Phase 6.5: GrandMA Integration Testing

**Goal**: Full integration test with GrandMA 3.

**Tasks**:
- [ ] End-to-end test workflow:
  - [ ] Launch YAMS and GrandMA 3 onPC
  - [ ] Verify discovery
  - [ ] Browse media library
  - [ ] Assign media to layers from console
  - [ ] Control playback from console
  - [ ] Verify thumbnails display
- [ ] Test with real GrandMA 3 hardware (if available)
- [ ] Document setup instructions for users
- [ ] Create troubleshooting guide

**Deliverables**:
- Full CITP integration working with GrandMA 3
- User documentation for GrandMA 3 setup
- Troubleshooting guide

---

### Phase 6.6: GrandMA 2 Compatibility & Configuration

**Goal**: GrandMA 2 support (optional), unified CITP configuration UI.

**Tasks**:
- [ ] Decide if GrandMA 2 support is needed
- [ ] If yes: Research GrandMA 2 CITP differences
- [ ] If yes: Test with GrandMA 2 onPC
- [ ] If yes: Implement compatibility layer
- [ ] Create CITP configuration UI (enable/disable, device name, etc.)
- [ ] Add tests for configuration
- [ ] Save/load CITP settings

**Deliverables**:
- GrandMA 2 support (if implemented)
- CITP configuration UI
- Settings persist across restarts
- Tests verify configuration

---

**Phase 6 Complete When**: CITP works with GrandMA 3, media library browsable from console, full control via console, tests pass. v1.0.0 release ready!

---

## Future Phases (Beyond Initial Roadmap)

### Potential Future Features:
- **NDI Output**: Stream output as NDI source for integration with broadcast tools
- **Advanced Transitions**: Wipe, dissolve, 3D effects
- **Effect Layers**: Color correction, blur, edge detection
- **Timecode Sync**: SMPTE/LTC for synchronized playback
- **Multi-Display**: Output to multiple monitors/projectors
- **Scripting API**: Lua or Python for custom automation
- **Web UI**: Browser-based control interface
- **4K/8K Support**: Higher resolution playback
- **Audio Reactive**: VU meter-driven effects

These will be evaluated based on user feedback and project priorities.

---

## Testing & Deployment Checklist (Per Phase)

Each phase should complete this checklist before moving to the next:

- [ ] **Linux Build**: Compiles without warnings in CI
- [ ] **Windows Build**: Compiles without warnings in CI
- [ ] **Flatpak Build**: Builds successfully in CI
- [ ] **Linux Tests**: All tests pass in CI
- [ ] **Windows Tests**: All tests pass in CI
- [ ] **Linux Manual Testing**: Features work as expected (local)
- [ ] **Windows Manual Testing**: Download CI artifact, features work
- [ ] **Flatpak Manual Testing**: Install CI artifact, features work
- [ ] **Raspberry Pi Testing** (if applicable): Hardware accel verified
- [ ] **Code Review**: Self-review or peer review completed
- [ ] **Documentation**: User-facing docs updated (if applicable)
- [ ] **ARCHITECTURE.md**: Updated with any design changes
- [ ] **Git Commit**: Phase changes committed with clear message

---

## Version Milestones

Versioning scheme aligned with phases and sub-phases:

### Phase 0: Foundation (v0.1.x)
- **v0.1.0**: Phase 0.1 complete (CMake + Qt Hello World + CI/CD)
- **v0.1.1**: Phase 0.2 complete (GStreamer Integration)
- **v0.1.2**: Phase 0.3 complete (OpenGL Rendering)
- **v0.1.3**: Phase 0.4 complete (Qt + GStreamer + OpenGL Integration)

### Phase 1: Single Layer Media Playback (v0.2.x)
- **v0.2.0**: Phase 1.1 complete (GStreamer to OpenGL Pipeline)
- **v0.2.1**: Phase 1.2 complete (Video Texture Rendering)
- **v0.2.2**: Phase 1.3 complete (Playback Controls)
- **v0.2.3**: Phase 1.4 complete (Hardware Acceleration Validation)

### Phase 2: Multi-Layer Composition & Transitions (v0.3.x)
- **v0.3.0**: Phase 2.1 complete (Multi-Layer Architecture)
- **v0.3.1**: Phase 2.2 complete (Opacity & Alpha Blending)
- **v0.3.2**: Phase 2.3 complete (Playback Speed Control)
- **v0.3.3**: Phase 2.4 complete (Crossfade Transitions)
- **v0.3.4**: Phase 2.5 complete (Performance Optimization & Validation)

### Phase 3: Live Control Architecture & UI Feedback (v0.4.x)
- **v0.4.0**: Phase 3.1 complete (Control Abstraction Layer)
- **v0.4.1**: Phase 3.2 complete (Keyboard Bindings & Configuration)
- **v0.4.2**: Phase 3.3 complete (Live Feedback UI)
- **v0.4.3**: Phase 3.4 complete (Asset Management)

### Phase 4: GUI Styling for Live Events (v0.5.x)
- **v0.5.0**: Phase 4.1 complete (Design System & Color Palette)
- **v0.5.1**: Phase 4.2 complete (Qt Stylesheet Implementation)
- **v0.5.2**: Phase 4.3 complete (Custom Widgets & Polish)

### Phase 5: Protocol Support (v0.6.x)
- **v0.6.0**: Phase 5.1 complete (OSC Protocol)
- **v0.6.1**: Phase 5.2 complete (sACN Protocol)
- **v0.6.2**: Phase 5.3 complete (MIDI Protocol - optional)
- **v0.6.3**: Phase 5.4 complete (ArtNet Protocol)
- **v0.6.4**: Phase 5.5 complete (Protocol Configuration & Testing)

### Phase 6: CITP Support & Lighting Console Interoperability (v0.7.x → v1.0.0)
- **v0.7.0**: Phase 6.1 complete (CITP Protocol Research & Planning)
- **v0.7.1**: Phase 6.2 complete (CITP/PINF - Peer Discovery)
- **v0.7.2**: Phase 6.3 complete (CITP/MSEX - Media Server Extensions)
- **v0.7.3**: Phase 6.4 complete (Thumbnail Generation & Streaming)
- **v0.7.4**: Phase 6.5 complete (GrandMA Integration Testing)
- **v1.0.0**: Phase 6.6 complete (GrandMA 2 Compatibility & Configuration) - **First stable release**

---

**Roadmap Status Legend**:
- [ ] Not started
- [~] In progress
- [x] Complete
- [!] Blocked

**Next Steps**: Complete Phase 0.1 manual testing, then begin Phase 0.2 - GStreamer Integration
