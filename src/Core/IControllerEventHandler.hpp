#ifndef ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED
#define ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED

#include <SDL.h>

#include <cstdint>

namespace GCMidi {

class IControllerEventHandler {
public:
    virtual ~IControllerEventHandler() = default;

    // Called by SdlManager's background thread
    virtual void onControllerConnected(const char* name) = 0;
    virtual void onControllerDisconnected() = 0;
    virtual void onControllerButton(uint8_t button, bool pressed, bool shiftState) = 0;
    virtual void onControllerAxis(uint8_t axis, int16_t value, bool shiftState) = 0;

    // Get the button index used as shift modifier
    virtual uint8_t getShiftButton() const {
        return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;  // Default
    }

    // Get the shift button for a specific button (per-button shift support)
    virtual uint8_t getShiftButtonForButton(uint8_t button) const {
        (void)button;             // Suppress unused parameter warning
        return getShiftButton();  // Default: use global shift
    }
};

}  // namespace GCMidi

#endif  // ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED
