#ifndef ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED
#define ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED

#include <SDL.h>

#include <cstdint>

namespace GCMidi {

/**
 * Consumer of SDL controller events. `SdlManager` calls this from its polling
 * thread, so implementations must synchronize any state also read by UI/host
 * code. These callbacks are not made from the audio thread.
 */
class IControllerEventHandler {
public:
    virtual ~IControllerEventHandler() = default;

    /** Record that SDL opened a controller and expose its display name. */
    virtual void onControllerConnected(const char* name) = 0;

    /** Record that the active controller was removed; flush held notes if needed. */
    virtual void onControllerDisconnected() = 0;

    /** Handle a digital controller button edge. */
    virtual void onControllerButton(uint8_t button, bool pressed, bool shiftState) = 0;

    /** Handle an analog controller axis movement. */
    virtual void onControllerAxis(uint8_t axis, int16_t value, bool shiftState) = 0;

    /** Return the global SDL button used as shift modifier. */
    virtual uint8_t getShiftButton() const {
        return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;  // Default
    }

    /** Return a per-button shift override, or the global shift button. */
    virtual uint8_t getShiftButtonForButton(uint8_t button) const {
        (void)button;             // Suppress unused parameter warning
        return getShiftButton();  // Default: use global shift
    }
};

}  // namespace GCMidi

#endif  // ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED
