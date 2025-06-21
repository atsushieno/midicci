# Flutter MIDI-CI Tool

A Flutter GUI for the MIDI-CI Tool using the midicci C++ library.

## Overview

This Flutter application provides a cross-platform GUI for the MIDI-CI protocol implementation in the midicci library. It integrates with the existing C++ business logic through platform channels and FFI.

## Architecture

The Flutter app connects to the C++ midicci library through:
- **CIToolRepository** - Main repository for configuration and logging
- **CIDeviceModel** - Device state management and MIDI-CI operations  
- **CIDeviceManager** - MIDI device enumeration and connection management

## Building

### Prerequisites
- Flutter SDK (3.10.0 or later)
- CMake 3.18+
- C++ compiler
- midicci library built

### Build Commands

From the midicci repository root:

```bash
# Configure CMake (Flutter will be detected automatically)
cmake -B build

# Build everything including Flutter app
cmake --build build

# Or build just the Flutter app
cmake --build build --target flutter-build
```

### Development

```bash
# Install Flutter dependencies
flutter pub get

# Run the app in development mode
flutter run -d linux

# Or use CMake target
cmake --build build --target flutter-run
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
