#!/bin/bash

# Copy native dependencies to Flutter app bundle
FRAMEWORKS_DIR="build/macos/Build/Products/Debug/midicci_flutter_gui.app/Contents/Frameworks"

echo "📦 Copying native dependencies to Flutter app bundle..."

# Create Frameworks directory if it doesn't exist
mkdir -p "$FRAMEWORKS_DIR"

# Copy main wrapper library
cp libmidicci-flutter-wrapper.dylib "$FRAMEWORKS_DIR/"

# Copy dependencies
cp /Users/atsushi/sources/midicci/build/src/libmidicci.1.dylib "$FRAMEWORKS_DIR/"
cp /Users/atsushi/sources/midicci/build/_deps/libremidi-build/liblibremidi.5.2.0.dylib "$FRAMEWORKS_DIR/"

echo "✅ Native dependencies copied successfully"
echo "Libraries in Frameworks directory:"
ls -la "$FRAMEWORKS_DIR/"lib*.dylib