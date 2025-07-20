# Reference repository

This repository is port of ktmidi Kotlin Multiplatform library.

ktmidi is located at `../KtMidi/ktmidi`.

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
- Use shorthand namespace notation `namespace Foo::Bar::Baz { ... }`.
