## midicci: MIDI-CI tools and library in C++

![midicci-app v0.1 sshot](docs/images/midicci-app-v0.1-sshot.png)

It is an agentic coding experiment to port [ktmidi-ci](https://github.com/atsushieno/ktmidi/tree/main/ktmidi-ci) and [ktmidi-ci-tool](https://github.com/atsushieno/ktmidi/tree/main/ktmidi-ci-tool) to C++ so that I can build a **fully featured** MICI-CI library based on the existing implementation **under a liberal license**.

So far it is fairly successful. It is mostly working, sometimes ahead of ktmidi-ci It is the MIDI-CI integration engine in my [uapmd](https://github.com/atsushieno/uapmd) project, and the GUI application (based on ImGui or Qt) can communicate with ktmidi-ci-tool to some extent.

## Build, Install, and Use midicci

There are Linux packages available in the [Releases](https://github.com/atsushieno/midicci/releases) page as well as GitHub Actions CI artifacts.

For macOS there is a Homebrew setup available: `brew install atsushieno/oss/midicci`

To build from source, use CMake:

```
cmake -B build -G Ninja # build or whatever directory you want, Ninja or whatever generator
cmake --build build
cmake --install build --prefix /usr/local # or wherever destination directory
```

midicci comes with a MIDI 2.0 keyboard and MIDI-CI diagnostic utility `midicci-app` (or `midicci-keyboard` and `midicci-gui` in Qt).

## License and dependencies

midicci is released under the MIT license.

We use [libremidi](https://github.com/celtera/libremidi) for MIDI device access, which is released under the BSD license (its dependencies are under the MIT license and the BSD license).

We use ImGui for the GUI (and Qt for some old GUI tools before the big integration), after various attempts to let Devin and Claude Code to write React+Electron code as well as Flutter code. Devin generated platform channels while it should be dart FFI. Claude did not make such a mistake, but could not handle complicated interop scenarios especially beyond isolates. I didn't feel I should try similar for React with its threading model. But you should take these with 
a grain of salt because the situation around agentic coding is a moving target.

We use [zlib-ng](https://github.com/zlib-ng/zlib-ng) for Mcoded7 compression (also fits nicely in Windows build), which is released under the Zlib license.

## Development

It is not much more than an agentic coding experiment. Code quality is as low as generated. But also I make changes by myself too to avoid silly requests to them.

Initially I used Devin to instruct it to generate code, but now I switched to Claude Code and Codex to fix a lot of mistakes by Devin and each other, and I fix various issues by myself.
