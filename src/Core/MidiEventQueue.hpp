#ifndef MIDI_EVENT_QUEUE_HPP_INCLUDED
#define MIDI_EVENT_QUEUE_HPP_INCLUDED

#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <cstdint>

#include "Common/MidiTypes.hpp"

namespace GCMidi {

/**
 * Lock-free MIDI transport between controller/mapping code and the audio
 * callback. Producers may run on SDL or lifecycle threads; the audio thread is
 * the only consumer and drains with `popMidi()`. Note Off messages use a small
 * priority queue first so stuck notes are favored when the host accepts fewer
 * events than were generated.
 */
class MidiEventQueue : public IMidiOutputSink {
public:
    /** Preallocate lock-free queues; no audio-thread allocation is expected. */
    MidiEventQueue();

    /** Producer-side enqueue. Returns false and increments counters if full. */
    bool pushMidi(const RawMidi& midi) override;

    /** Audio-thread safe: pop one event, preferring queued Note Off messages. */
    bool popMidi(RawMidi& outEv);

    /** Atomic count of all MIDI events dropped because both queues were full. */
    uint64_t getDroppedMidiEventCount() const;

    /** Atomic count of dropped Note Off events, included in total drops. */
    uint64_t getDroppedNoteOffCount() const;

private:
    /** Lock-free queue for normal MIDI events. */
    boost::lockfree::queue<RawMidi> fMidiQueue;

    /** Lock-free priority queue for Note Off messages. */
    boost::lockfree::queue<RawMidi> fPriorityNoteOffQueue;

    /** Drop counters updated by producers and read by UI. */
    std::atomic<uint64_t> fDroppedMidiEvents{0};
    std::atomic<uint64_t> fDroppedNoteOffEvents{0};

    /** True for Note Off, including Note On with zero velocity. */
    static bool isNoteOff(const RawMidi& midi);
};

}  // namespace GCMidi

#endif  // MIDI_EVENT_QUEUE_HPP_INCLUDED
