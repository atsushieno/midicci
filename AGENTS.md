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
- rg
- sed
- gh run view ...
- gh run download ...
- cmake --build cmake-build-debug

When you run `cmake`, you must not use any other build directory than `cmake-build-debug` .

You must not perform CMake builds using `make` or `ninja`. Always use `cmake` instead.

Always ensure that this CMake build passes.

# Coding conventions

- Use `#pragma once` in the headers. We expect this works in all modern C++ compilers.
- Use shorthand namespace notation `namespace Foo::Bar::Baz { ... }` (NOTE: `Foo::Bar::Baz` is not THE NAMESPACE you use).

# Related specifications

MIDI-CI: https://drive.google.com/file/d/1PDhak0-sWUEVscsz_4SSRRRQo94h1lX4/view
UMP: https://drive.google.com/file/d/1l2L5ALHj4K9hw_LalQ2jJZBMXDxc9Uel/view

# Code comments

Claude tends to leave too much comments. It must be kept minimum.

In particular, it must never ever leave comments like "following Kotlin implementation" which is TOO OBVIOUS.

Claude leaves another kind of silly comments like "following ktmidi commit xxxxx" which is wrong for a handful of reasons. The commit hashes can change when the ktmidi development occurs in dev/feature branches. They can vanish and then the reference becomes a dangling pointer. It is prohibited at all.
