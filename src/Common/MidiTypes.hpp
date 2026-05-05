#ifndef MIDI_TYPES_HPP_INCLUDED
#define MIDI_TYPES_HPP_INCLUDED

#include <cstdint>

namespace GCMidi {

struct RawMidi {
    uint8_t data[4];
    uint8_t size;
    uint32_t frame;
};

class IMidiOutputSink {
public:
    virtual ~IMidiOutputSink() = default;

    virtual bool pushMidi(const RawMidi& midi) = 0;
};

}  // namespace GCMidi

#endif  // MIDI_TYPES_HPP_INCLUDED
