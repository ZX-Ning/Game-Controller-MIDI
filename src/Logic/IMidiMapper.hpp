#ifndef IMIDI_MAPPER_HPP_INCLUDED
#define IMIDI_MAPPER_HPP_INCLUDED

#include <SDL.h>

#include <boost/lockfree/queue.hpp>
#include <cstdint>

#include "Common/MidiTypes.hpp"

namespace GCMidi {

// Interface for translating controller events into MIDI
class IMidiMapper {
public:
    virtual ~IMidiMapper() = default;

    // Returns the name of the mapping
    virtual const char* getName() const = 0;

    // Process a button event and push generated MIDI to the queue
    virtual void onButton(uint8_t button, bool pressed, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) = 0;

    // Process an axis event (analog sticks, triggers)
    virtual void onAxis(uint8_t axis, int16_t value, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) = 0;

    // Get current octave offset for UI display
    virtual int getOctaveOffset() const {
        return 0;
    }

    // Set octave offset (e.g. from plugin parameters)
    virtual void setOctaveOffset(int offset) = 0;

    // Get the button index used as shift modifier (default: right shoulder)
    virtual uint8_t getShiftButton() const {
        return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    }
};

}  // namespace GCMidi

#endif  // IMIDI_MAPPER_HPP_INCLUDED
