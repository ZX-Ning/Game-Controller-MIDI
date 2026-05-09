#ifndef MIDI_TYPES_HPP_INCLUDED
#define MIDI_TYPES_HPP_INCLUDED

#include <cstdint>

namespace GCMidi {

/**
 * Fixed-size representation of one MIDI message queued between controller,
 * mapper, and plugin processing code. Trivially copyable and allocation-free.
 */
struct RawMidi {
    uint8_t data[4];
    uint8_t size;
    uint32_t frame;
};

/**
 * Output target used by mappers to emit MIDI without knowing queue or host
 * details. Implementations may be called from SDL and lifecycle threads and
 * must document their own synchronization.
 */
class IMidiOutputSink {
public:
    virtual ~IMidiOutputSink() = default;

    /** Queue one MIDI event; returns false when it cannot be accepted. */
    virtual bool pushMidi(const RawMidi& midi) = 0;
};

}  // namespace GCMidi

#endif  // MIDI_TYPES_HPP_INCLUDED
