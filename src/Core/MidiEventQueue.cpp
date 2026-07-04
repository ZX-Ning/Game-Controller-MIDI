#include "MidiEventQueue.hpp"

namespace GCMidi {

MidiEventQueue::MidiEventQueue()
    : fMidiQueue(1024) {  // Sized for high-frequency axis events.
}

bool MidiEventQueue::pushMidi(const RawMidi& midi) {
    if (fMidiQueue.bounded_push(midi)) {
        return true;
    }

    fDroppedMidiEvents.fetch_add(1, std::memory_order_relaxed);
    if (isNoteOff(midi)) {
        fDroppedNoteOffEvents.fetch_add(1, std::memory_order_relaxed);
    }
    return false;
}

bool MidiEventQueue::popMidi(RawMidi& outEv) {
    return fMidiQueue.pop(outEv);
}

uint64_t MidiEventQueue::getDroppedMidiEventCount() const {
    return fDroppedMidiEvents.load(std::memory_order_relaxed);
}

uint64_t MidiEventQueue::getDroppedNoteOffCount() const {
    return fDroppedNoteOffEvents.load(std::memory_order_relaxed);
}

bool MidiEventQueue::isNoteOff(const RawMidi& midi) {
    if (midi.size < 3) {
        return false;
    }

    const uint8_t status = midi.data[0] & 0xF0;
    return status == 0x80 || (status == 0x90 && midi.data[2] == 0);
}

}  // namespace GCMidi
