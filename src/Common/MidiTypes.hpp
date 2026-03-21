#ifndef MIDI_TYPES_HPP_INCLUDED
#define MIDI_TYPES_HPP_INCLUDED

#include <cstdint>

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

struct RawMidi {
    uint8_t data[4];
    uint8_t size;
    uint32_t frame;
};

END_NAMESPACE_DISTRHO

#endif  // MIDI_TYPES_HPP_INCLUDED
