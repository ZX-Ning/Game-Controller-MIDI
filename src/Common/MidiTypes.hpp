#ifndef MIDI_TYPES_HPP_INCLUDED
#define MIDI_TYPES_HPP_INCLUDED

#include <cstdint>

namespace GCMidi {

struct RawMidi {
    uint8_t data[4];
    uint8_t size;
    uint32_t frame;
};

}  // namespace GCMidi

#endif  // MIDI_TYPES_HPP_INCLUDED
