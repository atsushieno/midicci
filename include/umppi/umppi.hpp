#pragma once

// umppi - MIDI file I/O and playback library
// Headers ordered by dependency (least dependent first)

#include "details/Common.hpp"
#include "details/Utility.hpp"
#include "details/DeltaTimeComputer.hpp"
#include "details/PlayerCommon.hpp"
#include "details/MidiPlayerTimer.hpp"
#include "details/Ump.hpp"
#include "details/UmpFactory.hpp"
#include "details/UmpRetriever.hpp"
#include "details/UmpTranslator.hpp"

#include "details/Midi1Message.hpp"
#include "details/Midi1Event.hpp"
#include "details/Midi1Track.hpp"
#include "details/Midi1Music.hpp"
#include "details/Midi1Reader.hpp"
#include "details/Midi1Writer.hpp"
#include "details/Midi1Machine.hpp"

#include "details/Midi2Track.hpp"
