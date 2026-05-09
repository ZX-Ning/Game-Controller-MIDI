#ifndef IMIDI_MAPPER_HPP_INCLUDED
#define IMIDI_MAPPER_HPP_INCLUDED

#include <SDL.h>

#include <cstdint>

#include "Common/MidiTypes.hpp"

namespace GCMidi {

/**
 * Interface for translating controller input into MIDI.
 *
 * `EventDispatcher` owns and serializes mapper access with its mapper mutex.
 * Event methods are called from the SDL polling thread and should avoid
 * allocation, blocking, and internal locks.
 */
class IMidiMapper {
public:
    virtual ~IMidiMapper() = default;

    /** Return a null-terminated mapper or preset name. */
    virtual const char* getName() const = 0;

    /** Process a button event and push generated MIDI to `out`. */
    virtual void onButton(uint8_t button, bool pressed, bool shiftState, IMidiOutputSink& out) = 0;

    /** Process an axis event from analog sticks or triggers. */
    virtual void onAxis(uint8_t axis, int16_t value, bool shiftState, IMidiOutputSink& out) = 0;

    /** Emit Note Off for held notes; failed sends must remain tracked. */
    virtual bool flushActiveNotes(IMidiOutputSink& out) = 0;

    /** Return the base octave offset shown in UI and plugin state. */
    virtual int getOctaveOffset() const {
        return 0;
    }

    /** Set the base octave offset. */
    virtual void setOctaveOffset(int offset) = 0;

    /** Return the transient LT/RT trigger octave offset. */
    virtual int8_t getTriggerOctaveOffset() const {
        return 0;
    }

    /** Set the transient LT/RT trigger octave offset. */
    virtual void setTriggerOctaveOffset(int8_t offset) {
        (void)offset;
    }

    /** Return the global SDL button used as shift modifier. */
    virtual uint8_t getShiftButton() const {
        return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    }

    /** Return a per-button shift override, or the global shift button. */
    virtual uint8_t getShiftButtonForButton(uint8_t button) const {
        (void)button;  // Suppress unused parameter warning
        return getShiftButton();
    }
};

}  // namespace GCMidi

#endif  // IMIDI_MAPPER_HPP_INCLUDED
