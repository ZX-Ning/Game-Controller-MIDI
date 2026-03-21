#ifndef ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED
#define ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED

#include <cstdint>

namespace GCMidi {

class IControllerEventHandler {
public:
    virtual ~IControllerEventHandler() = default;

    // Called by SdlManager's background thread
    virtual void onControllerConnected(const char* name) = 0;
    virtual void onControllerDisconnected() = 0;
    virtual void onControllerButton(uint8_t button, bool pressed, bool shiftState) = 0;
};

}  // namespace GCMidi

#endif  // ICONTROLLER_EVENT_HANDLER_HPP_INCLUDED
