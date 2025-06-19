It is an agentic coding experiment to port ktmidi-ci and ktmidi-ci-tool to C++ so that I can build a fully featured MICI-CI library bassed on existing implementation.

So far it is not successful. It's not really working for various parts. Though the GUI application (based on Qt) can communicate with ktmidi-ci-tool
to some extent.

## dependencies

We use [libremidi](https://github.com/jcelerier/libremidi) for MIDI device access.

We use Qt for the GUI.

## management

It is so far not much more than an agentic coding experiment. Code quality is as low as generated. But also I make changes by myself too to avoid silly requests to them.

So far only Devin is used for this tool, but can be hybrid of others sooner or later.
