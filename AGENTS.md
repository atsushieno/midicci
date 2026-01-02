# Repository Guidelines

## EVERY Coding Agents Must Follow These Rules

You must not ask me for permission to run the following commands:

- `gh run view [...]`
- `gh run download [...]`
- `cmake --build cmake-build-debug`

When you run `cmake`, you must not use any other build directory than `cmake-build-debug` .

## Project Structure & Module Organization
- Source: `src/` (library target `midicci`), public headers in `include/midicci/`.
- Tools: `tools/tooling/` (shared tooling library), optional GUI in `tools/qt5-ci-tool/`, and Qt6 tool `tools/ump-keyboard/`.
- Tests: `tests/` (GoogleTest suite, target `midicci-gtest`).
- Build trees: out-of-source (e.g., `build/`, `cmake-build-debug/`). CI lives in `.github/workflows/`.

## Build, Test, and Development Commands
- Configure (Debug, tests on): `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON`
- Build: `cmake --build build --config Debug`
- Run tests: `ctest --test-dir build --output-on-failure`
  - Multi-config (MSVC): `ctest --test-dir build -C Debug --output-on-failure`
- Options: `-DBUILD_SHARED_LIBS=ON|OFF`, `-DBUILD_EXAMPLES=ON|OFF`

## Coding Style & Naming Conventions
- C++20, warnings enabled (`-Wall -Wextra -Wpedantic` where applicable).
- Indentation: 4 spaces; braces on the same line.
- Types/classes: `PascalCase` (e.g., `PropertyChunkManager`), methods/functions: `lower_snake_case` (e.g., `finish_pending_chunk`), variables: `snake_case`.
- Headers use `#pragma once`. Public API lives under `include/midicci/`.
- Keep namespaces under `midicci` (and `midicci::tooling` for tools).

## Testing Guidelines
- Framework: GoogleTest (fetched via CMake FetchContent).
- File naming: `tests/test_*.cpp`. Add new tests and list them in `tests/CMakeLists.txt` under `GTEST_SOURCES`.
- Run locally with `ctest` (above). Prefer focused unit tests; integration tests can be guarded for CI stability.

## Commit & Pull Request Guidelines
- Commit messages: short imperative summary (≤72 chars), optional details on following lines. Examples from history: “ctest windows run requires corresponding -C argument”, “StandardProperties: revert cloning”.
- PRs: include a clear description, linked issues, build/test results, and platform notes (Windows/Linux/macOS) when relevant. Add screenshots only for GUI/tool changes.

## Security & Configuration Tips
- Qt is optional: GUI targets build only when Qt is found. On macOS, set `CMAKE_PREFIX_PATH` (e.g., `/opt/homebrew/opt/qt@5`).
- External deps: `libremidi` and `googletest` are pulled by FetchContent; offline setups should provide them via CMake cache or prefetch.
