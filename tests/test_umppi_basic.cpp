#include <gtest/gtest.h>
#include <umppi/umppi.hpp>
#include <sstream>

using namespace umppi;

TEST(UmppiBasicTest, Midi1MessageCreation) {
    auto msg = std::make_shared<Midi1SimpleMessage>(
        MidiChannelStatus::NOTE_ON, 60, 100
    );

    EXPECT_EQ(msg->getStatusCode(), MidiChannelStatus::NOTE_ON);
    EXPECT_EQ(msg->getMsb(), 60);
    EXPECT_EQ(msg->getLsb(), 100);
}

TEST(UmppiBasicTest, Midi1TrackAndMusic) {
    Midi1Music music;
    music.format = 1;
    music.deltaTimeSpec = 480;

    Midi1Track track;
    track.events.push_back(Midi1Event{
        0,
        std::make_shared<Midi1SimpleMessage>(MidiChannelStatus::NOTE_ON, 60, 100)
    });
    track.events.push_back(Midi1Event{
        480,
        std::make_shared<Midi1SimpleMessage>(MidiChannelStatus::NOTE_OFF, 60, 0)
    });

    music.addTrack(std::move(track));

    EXPECT_EQ(music.tracks.size(), 1);
    EXPECT_EQ(music.tracks[0].events.size(), 2);
}

TEST(UmppiBasicTest, UmpCreationAndAccessors) {
    Ump ump(uint32_t(0x20906040));

    EXPECT_EQ(ump.getMessageType(), MessageType::MIDI1);
    EXPECT_EQ(ump.getGroup(), 0);
    EXPECT_EQ(ump.getStatusByte(), 0x90);
    EXPECT_EQ(ump.getStatusCode(), MidiChannelStatus::NOTE_ON);
    EXPECT_EQ(ump.getChannelInGroup(), 0);
    EXPECT_EQ(ump.getMidi1Note(), 0x60);
    EXPECT_EQ(ump.getMidi1Velocity(), 0x40);
}

TEST(UmppiBasicTest, UmpSizeCalculation) {
    Ump ump32(uint32_t(0x20906040));
    EXPECT_EQ(ump32.getSizeInInts(), 1);
    EXPECT_EQ(ump32.getSizeInBytes(), 4);

    Ump ump64(uint32_t(0x40906040), uint32_t(0x12345678));
    EXPECT_EQ(ump64.getSizeInInts(), 2);
    EXPECT_EQ(ump64.getSizeInBytes(), 8);

    Ump ump128(uint32_t(0x50906040), uint32_t(0x12345678), uint32_t(0xABCDEF00), uint32_t(0x11223344));
    EXPECT_EQ(ump128.getSizeInInts(), 4);
    EXPECT_EQ(ump128.getSizeInBytes(), 16);
}

TEST(UmppiBasicTest, Midi2Track) {
    Midi2Track track;

    track.messages.push_back(Ump(uint32_t(0x00100000)));
    track.messages.push_back(Ump(uint32_t(0x20906040)));

    EXPECT_EQ(track.messages.size(), 2);
}

TEST(UmppiBasicTest, Midi2Music) {
    Midi2Music music;
    music.deltaTimeSpec = 480;

    Midi2Track track;
    track.messages.push_back(Ump(uint32_t(0x00100000)));

    music.addTrack(std::move(track));

    EXPECT_TRUE(music.isSingleTrack());
    EXPECT_EQ(music.tracks.size(), 1);
}

TEST(UmppiBasicTest, Midi1MachineStateTracking) {
    Midi1Machine machine;

    auto noteOn = Midi1SimpleMessage(MidiChannelStatus::NOTE_ON, 60, 100);
    machine.processMessage(noteOn);

    EXPECT_TRUE(machine.channels[0].noteOnStatus[60]);
    EXPECT_EQ(machine.channels[0].noteVelocity[60], 100);

    auto noteOff = Midi1SimpleMessage(MidiChannelStatus::NOTE_OFF, 60, 0);
    machine.processMessage(noteOff);

    EXPECT_FALSE(machine.channels[0].noteOnStatus[60]);
}

TEST(UmppiBasicTest, UmpToBytes) {
    Ump ump(uint32_t(0x20906040));
    auto bytes = ump.toBytes();

    EXPECT_EQ(bytes.size(), 4);
    EXPECT_EQ(bytes[0], 0x20);
    EXPECT_EQ(bytes[1], 0x90);
    EXPECT_EQ(bytes[2], 0x60);
    EXPECT_EQ(bytes[3], 0x40);
}

TEST(UmppiBasicTest, UmpFromBytes) {
    std::vector<uint8_t> bytes = {0x20, 0x90, 0x60, 0x40};
    auto umps = Ump::fromBytes(bytes);

    EXPECT_EQ(umps.size(), 1);
    EXPECT_EQ(umps[0].int1, 0x20906040);
}

TEST(UmppiBasicTest, WriteMetaTextWithEndOfTrack) {
    Midi1Music music;
    music.deltaTimeSpec = 0x30;

    Midi1Track track;
    std::vector<uint8_t> textData = {0x41, 0x41, 0x41, 0x41};
    track.events.push_back(Midi1Event{
        0,
        std::make_shared<Midi1CompoundMessage>(0xFF, 3, 0, textData)
    });
    track.events.push_back(Midi1Event{
        0,
        std::make_shared<Midi1CompoundMessage>(0xFF, MidiMetaType::END_OF_TRACK, 0, std::vector<uint8_t>())
    });

    music.addTrack(std::move(track));

    std::stringstream ss;
    Midi1Writer writer(ss);
    writer.write(music);

    std::vector<uint8_t> expected = {
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 0x30,
        'M', 'T', 'r', 'k', 0, 0, 0, 0x0C,
        0, 0xFF, 3, 4, 0x41, 0x41, 0x41, 0x41,
        0, 0xFF, 0x2F, 0
    };

    std::string result = ss.str();
    std::vector<uint8_t> actual(result.begin(), result.end());

    EXPECT_EQ(actual, expected);
}

TEST(UmppiBasicTest, Midi1MusicMergeTracks) {
    Midi1Music music;
    music.format = 1;
    music.deltaTimeSpec = 480;

    Midi1Track track1;
    track1.events.push_back(Midi1Event{
        0,
        std::make_shared<Midi1SimpleMessage>(MidiChannelStatus::NOTE_ON, 60, 100)
    });
    track1.events.push_back(Midi1Event{
        480,
        std::make_shared<Midi1SimpleMessage>(MidiChannelStatus::NOTE_OFF, 60, 0)
    });

    Midi1Track track2;
    track2.events.push_back(Midi1Event{
        240,
        std::make_shared<Midi1SimpleMessage>(MidiChannelStatus::NOTE_ON, 64, 100)
    });
    track2.events.push_back(Midi1Event{
        240,
        std::make_shared<Midi1SimpleMessage>(MidiChannelStatus::NOTE_OFF, 64, 0)
    });

    music.addTrack(std::move(track1));
    music.addTrack(std::move(track2));

    auto merged = music.mergeTracks();

    EXPECT_EQ(merged.format, 0);
    EXPECT_EQ(merged.tracks.size(), 1);
    EXPECT_EQ(merged.deltaTimeSpec, 480);
    EXPECT_EQ(merged.tracks[0].events.size(), 4);

    EXPECT_EQ(merged.tracks[0].events[0].deltaTime, 0);
    EXPECT_EQ(merged.tracks[0].events[1].deltaTime, 240);
    EXPECT_EQ(merged.tracks[0].events[2].deltaTime, 240);
    EXPECT_EQ(merged.tracks[0].events[3].deltaTime, 0);
}

TEST(UmppiBasicTest, Midi2MusicMergeTracks) {
    Midi2Music music;
    music.deltaTimeSpec = 480;

    Midi2Track track1;
    track1.messages.push_back(Ump(UmpFactory::deltaClockstamp(0)));
    track1.messages.push_back(Ump(uint32_t(0x40906040), uint32_t(0x80000000)));
    track1.messages.push_back(Ump(UmpFactory::deltaClockstamp(480)));
    track1.messages.push_back(Ump(uint32_t(0x40806040), uint32_t(0)));

    Midi2Track track2;
    track2.messages.push_back(Ump(UmpFactory::deltaClockstamp(240)));
    track2.messages.push_back(Ump(uint32_t(0x40906440), uint32_t(0x80000000)));
    track2.messages.push_back(Ump(UmpFactory::deltaClockstamp(240)));
    track2.messages.push_back(Ump(uint32_t(0x40806440), uint32_t(0)));

    music.addTrack(std::move(track1));
    music.addTrack(std::move(track2));

    auto merged = music.mergeTracks();

    EXPECT_EQ(merged.tracks.size(), 1);
    EXPECT_EQ(merged.deltaTimeSpec, 480);

    int deltaCount = 0;
    int noteCount = 0;
    for (const auto& msg : merged.tracks[0].messages) {
        if (msg.isDeltaClockstamp()) {
            deltaCount++;
        } else if (msg.getMessageType() == MessageType::MIDI2) {
            noteCount++;
        }
    }

    EXPECT_EQ(deltaCount, 2);
    EXPECT_EQ(noteCount, 4);
}
