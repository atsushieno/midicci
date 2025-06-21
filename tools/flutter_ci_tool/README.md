# MIDI-CI Flutter Tool

A Flutter desktop application for MIDI-CI (Capability Inquiry) communication, built on top of the midicci C++ library.

## Features

- **Initiator Mode**: Send discovery requests and communicate with remote MIDI-CI devices
- **Responder Mode**: Respond to incoming MIDI-CI requests (TODO)
- **Real-time Logging**: View MIDI-CI message traffic
- **Settings**: Configure devices and application preferences

## Prerequisites

- Flutter SDK (>=3.10.0) 
- Dart SDK (>=3.0.0)
- CMake (>=3.18)
- C++ compiler with C++20 support
- macOS: Xcode 16+ (for macOS builds)
- Linux: Build tools and development libraries
- Windows: Visual Studio 2019+ (for Windows builds)

## Quick Start

### 1. Get Dependencies

```bash
flutter pub get
```

### 2. Run on macOS

```bash
flutter run -d macos
```

### 3. Build for Release

```bash
flutter build macos --release
```

## Architecture

The application uses Flutter for the UI with FFI (Foreign Function Interface) to communicate with the underlying midicci C++ library:

```
┌─────────────────┐    FFI    ┌─────────────────┐
│   Flutter UI    │ ◄─────────► │  midicci C++    │
│   (Dart)        │           │  Library        │
└─────────────────┘           └─────────────────┘
```

### Key Components

- **Providers**: State management using the Provider package
  - `CIToolProvider`: Main application state and native bridge
  - `CIDeviceProvider`: MIDI device and connection management

- **FFI Bridge**: Direct communication with C++ library (`midicci::tooling` namespace)
  - `MidiCCIBindings`: Native function bindings
  - `MidiCIBridge`: High-level interface
  - `ci_tool_wrapper.cpp`: C++ wrapper for FFI

- **Screens**: Tab-based interface
  - Initiator: Device discovery and communication
  - Responder: Local profile and property management  
  - Log: Message logging and debugging
  - Settings: Configuration and device selection

### Latest Updates (Post-Merge)

- ✅ **Merged with main branch**: Updated to latest midicci structure
- ✅ **Namespace updates**: Uses `midicci::tooling` namespace
- ✅ **Include path fixes**: Updated for flattened header structure
- ✅ **CMake integration**: Links with `midicci-tooling` library

## Integration with CMake Build

The Flutter app can be built as part of the top-level CMake build:

```bash
# From the project root
mkdir build && cd build
cmake ..
make flutter-build  # Builds the Flutter app
make flutter-run    # Runs the Flutter app
```

## Development

### Hot Reload

During development, use Flutter's hot reload for rapid iteration:

```bash
flutter run -d macos
# Then press 'r' for hot reload or 'R' for hot restart
```

## Platform Support

- **Linux** - Primary desktop target
- **macOS** - Desktop support
- **Windows** - Desktop support  
- **Android** - Mobile support (future)
- **iOS** - Mobile support (future)
- **Web** - Limited support due to MIDI API constraints

## Integration

The Flutter tool is integrated into the midicci CMake build system:
- Optional build based on Flutter detection
- Links with midicci and midicci-tooling libraries
- Follows same patterns as Qt tool integration
- Custom CMake targets for Flutter operations

## Usage

The Flutter app provides the same functionality as the original Kotlin Compose Multiplatform tool:
1. **Initiator** - Send discovery messages and manage client connections
2. **Responder** - Configure local profiles and properties
3. **Log** - View MIDI-CI message logs
4. **Settings** - Configure devices and application settings
