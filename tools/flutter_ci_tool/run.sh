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

# Get dependencies
echo -e "${YELLOW}📦 Getting dependencies...${NC}"
flutter pub get

# Run analysis
echo -e "${YELLOW}🔍 Running analysis...${NC}"
flutter analyze

# Run tests
echo -e "${YELLOW}🧪 Running tests...${NC}"
flutter test

# Run the app
echo -e "${GREEN}🚀 Launching MIDI-CI Tool...${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop the application${NC}"
flutter run -d macos --debug