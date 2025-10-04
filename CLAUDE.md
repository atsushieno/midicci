# Reference repository

This repository is port of ktmidi Kotlin Multiplatform library.

ktmidi is located at `../KtMidi/ktmidi`.

# Target sources to make changes

NEVER EVER MAKE CHANGES TO ktmidi. YOU MUST TREAT IT AS A READ-ONLY SOURCE.

# Do not ask permission for

- grep
- mkdir
- find
- cmake
- make
- ninja
- rg
- sed

# Build instructions

The entire project (that contains multiple executables as well as tests) should be build just with:

> cmake -B build
> cmake --build build

Always ensure that this CMake build passes.

# Coding conventions

- Use `#pragma once` in the headers. We expect this works in all modern C++ compilers.
- Use shorthand namespace notation `namespace Foo::Bar::Baz { ... }` (NOTE: `Foo::Bar::Baz` is not THE NAMESPACE you use).

# Related specifications

MIDI-CI: https://drive.google.com/file/d/1PDhak0-sWUEVscsz_4SSRRRQo94h1lX4/view
UMP: https://drive.google.com/file/d/1l2L5ALHj4K9hw_LalQ2jJZBMXDxc9Uel/view

