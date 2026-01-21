#include <gtest/gtest.h>
#include <umppi/umppi.hpp>

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
    Ump ump(0x20906040);

    EXPECT_EQ(ump.getMessageType(), MidiMessageType::MIDI1);
    EXPECT_EQ(ump.getGroup(), 0);
    EXPECT_EQ(ump.getStatusByte(), 0x90);
    EXPECT_EQ(ump.getStatusCode(), MidiChannelStatus::NOTE_ON);
    EXPECT_EQ(ump.getChannelInGroup(), 0);
    EXPECT_EQ(ump.getMidi1Note(), 0x60);
    EXPECT_EQ(ump.getMidi1Velocity(), 0x40);
}

TEST(UmppiBasicTest, UmpSizeCalculation) {
    Ump ump32(0x20906040);
    EXPECT_EQ(ump32.getSizeInInts(), 1);
    EXPECT_EQ(ump32.getSizeInBytes(), 4);

    Ump ump64(0x40906040, 0x12345678);
    EXPECT_EQ(ump64.getSizeInInts(), 2);
    EXPECT_EQ(ump64.getSizeInBytes(), 8);

    Ump ump128(0x50906040, 0x12345678, 0xABCDEF00, 0x11223344);
    EXPECT_EQ(ump128.getSizeInInts(), 4);
    EXPECT_EQ(ump128.getSizeInBytes(), 16);
}

TEST(UmppiBasicTest, Midi2Track) {
    Midi2Track track;

    track.messages.push_back(Ump(0x00100000));
    track.messages.push_back(Ump(0x20906040));

    EXPECT_EQ(track.messages.size(), 2);
}

TEST(UmppiBasicTest, Midi2Music) {
    Midi2Music music;
    music.deltaTimeSpec = 480;

    Midi2Track track;
    track.messages.push_back(Ump(0x00100000));

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
    Ump ump(0x20906040);
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
