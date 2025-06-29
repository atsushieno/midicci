## What is this project?

It is an agentic coding experiment to port ktmidi-ci and ktmidi-ci-tool to C++ so that I can build a fully featured MICI-CI library bassed on existing implementation.

So far it is partially successful. It's not really working at various parts. Though the GUI application (based on Qt) can communicate with ktmidi-ci-tool
to some extent.

## dependencies

We use [libremidi](https://github.com/jcelerier/libremidi) for MIDI device access.

We use Qt for the GUI, after various attempts to let Devin and Claude Code to write React+Electron code as well as Flutter code. Devin generated platform channels while it should be dart FFI. Claude did not make such a mistake, but could not handle complicated interop scenarios especially beyond isolates. I didn't feel I should try similar for React with its threading model.

## management

It is so far not much more than an agentic coding experiment. Code quality is as low as generated. But also I make changes by myself too to avoid silly requests to them.

Initially I used Devin to instruct it to generate code, but now I switched to Claude Code and fixed a lot of mistakes by Devin, and I fix various issues by myself.

