#include "Core/EventDispatcher.hpp"
#include "Core/MidiEventQueue.hpp"

#include <cassert>
#include <cstdint>
#include <memory>

namespace {

GCMidi::RawMidi makeMidi(uint8_t status, uint8_t data1, uint8_t data2) {
    GCMidi::RawMidi midi{};
    midi.data[0] = status;
    midi.data[1] = data1;
    midi.data[2] = data2;
    midi.size = 3;
    return midi;
}

void assertMidiEquals(const GCMidi::RawMidi& actual, const GCMidi::RawMidi& expected) {
    assert(actual.size == expected.size);
    for (uint8_t i = 0; i < actual.size; ++i) {
        assert(actual.data[i] == expected.data[i]);
    }
}

class TestMapper final : public GCMidi::IMidiMapper {
public:
    TestMapper(bool flushResult, uint8_t shiftButton, int8_t baseOctave)
        : fFlushResult(flushResult),
          fShiftButton(shiftButton),
          fBaseOctave(baseOctave) {
    }

    const char* getName() const override {
        return "TestMapper";
    }

    void onButton(uint8_t, bool, bool, GCMidi::SharedState&, GCMidi::IMidiOutputSink&) override {
    }

    void onAxis(uint8_t, int16_t, bool, const GCMidi::SharedState&, GCMidi::IMidiOutputSink&) override {
    }

    bool flushActiveNotes(GCMidi::IMidiOutputSink&) override {
        ++fFlushCalls;
        return fFlushResult;
    }

    int8_t getInitialBaseOctaveOffset() const override {
        return fBaseOctave;
    }

    uint8_t getShiftButton() const override {
        return fShiftButton;
    }

    uint32_t flushCalls() const {
        return fFlushCalls;
    }

private:
    bool fFlushResult;
    uint8_t fShiftButton;
    int8_t fBaseOctave;
    uint32_t fFlushCalls = 0;
};

void testMidiQueuePreservesNoteOrder() {
    GCMidi::MidiEventQueue queue;
    const auto noteOn = makeMidi(0x90, 60, 100);
    const auto noteOff = makeMidi(0x80, 60, 0);

    assert(queue.pushMidi(noteOn));
    assert(queue.pushMidi(noteOff));

    GCMidi::RawMidi out{};
    assert(queue.popMidi(out));
    assertMidiEquals(out, noteOn);
    assert(queue.popMidi(out));
    assertMidiEquals(out, noteOff);
    assert(!queue.popMidi(out));
}

void testMidiQueuePreservesChordOrder() {
    GCMidi::MidiEventQueue queue;
    const GCMidi::RawMidi events[] = {
        makeMidi(0x90, 60, 100),
        makeMidi(0x90, 64, 100),
        makeMidi(0x90, 67, 100),
        makeMidi(0x80, 60, 0),
        makeMidi(0x80, 64, 0),
        makeMidi(0x80, 67, 0),
    };

    for (const auto& event : events) {
        assert(queue.pushMidi(event));
    }

    GCMidi::RawMidi out{};
    for (const auto& event : events) {
        assert(queue.popMidi(out));
        assertMidiEquals(out, event);
    }
    assert(!queue.popMidi(out));
}

void testMidiQueueCountsDroppedNoteOffs() {
    GCMidi::MidiEventQueue queue;
    const auto cc = makeMidi(0xB0, 1, 64);
    const auto noteOff = makeMidi(0x80, 60, 0);

    while (queue.pushMidi(cc)) {
    }

    const uint64_t droppedBefore = queue.getDroppedMidiEventCount();
    const uint64_t droppedNoteOffBefore = queue.getDroppedNoteOffCount();

    assert(!queue.pushMidi(noteOff));
    assert(queue.getDroppedMidiEventCount() == droppedBefore + 1);
    assert(queue.getDroppedNoteOffCount() == droppedNoteOffBefore + 1);
}

void testSetMapperInstallsNewMapperAfterFlushFailure() {
    GCMidi::EventDispatcher dispatcher;
    auto oldMapper = std::make_unique<TestMapper>(false, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 0);
    auto newMapper = std::make_unique<TestMapper>(true, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 3);

    dispatcher.setMapper(std::move(oldMapper));
    assert(dispatcher.getShiftButton() == SDL_CONTROLLER_BUTTON_LEFTSHOULDER);

    dispatcher.setMapper(std::move(newMapper));
    assert(dispatcher.getShiftButton() == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    assert(dispatcher.sharedState().consumeBaseOctaveHostSyncPending());
    assert(dispatcher.sharedState().baseOctave() == 3);
}

void testSharedStateHostSyncFlag() {
    GCMidi::SharedState state;

    state.setBaseOctaveFromPreset(12);
    assert(state.consumeBaseOctaveHostSyncPending());
    assert(state.baseOctave() == 4);
    assert(!state.consumeBaseOctaveHostSyncPending());

    state.adjustBaseOctaveFromMapper(-2);
    assert(state.consumeBaseOctaveHostSyncPending());
    assert(state.baseOctave() == 2);
    assert(!state.consumeBaseOctaveHostSyncPending());
}

}  // namespace

int main() {
    testMidiQueuePreservesNoteOrder();
    testMidiQueuePreservesChordOrder();
    testMidiQueueCountsDroppedNoteOffs();
    testSetMapperInstallsNewMapperAfterFlushFailure();
    testSharedStateHostSyncFlag();
    return 0;
}
