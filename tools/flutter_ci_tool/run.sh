#!/bin/bash

# Flutter MIDI-CI Tool Runner Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}🎵 MIDI-CI Flutter Tool${NC}"
echo "=========================="

# Check if Flutter is installed
if ! command -v flutter &> /dev/null; then
    echo -e "${RED}❌ Flutter is not installed or not in PATH${NC}"
    exit 1
fi

# Check Flutter version
echo -e "${YELLOW}📋 Checking Flutter...${NC}"
flutter --version

# Build native library
echo -e "${YELLOW}🔨 Building native library...${NC}"
cd ../../
if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake ..
make midicci-flutter-wrapper

# Copy library to Flutter directory
echo -e "${YELLOW}📋 Setting up native library...${NC}"
cp tools/flutter_ci_tool/libmidicci-flutter-wrapper.dylib ../tools/flutter_ci_tool/

# Return to Flutter directory
cd ../tools/flutter_ci_tool

# Get dependencies
echo -e "${YELLOW}📦 Getting dependencies...${NC}"
flutter pub get

# Build Flutter app first to create bundle structure
echo -e "${YELLOW}🔨 Building Flutter app...${NC}"
flutter build macos --debug

# Copy native dependencies to app bundle
echo -e "${YELLOW}📋 Copying native dependencies...${NC}"
./copy_native_deps.sh

# Run analysis
echo -e "${YELLOW}🔍 Running analysis...${NC}"
flutter analyze

# Run tests (skip integration tests to avoid widget disposal issues)
echo -e "${YELLOW}🧪 Running unit tests...${NC}"
flutter test test/logging_verification_test.dart

# Run the app
echo -e "${GREEN}🚀 Launching MIDI-CI Tool...${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop the application${NC}"
flutter run -d macos --debug