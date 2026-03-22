#ifndef EVENT_DISPATCHER_HPP_INCLUDED
#define EVENT_DISPATCHER_HPP_INCLUDED

#include <SDL.h>

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <memory>
#include <mutex>
#include <string>

#include "Common/MidiTypes.hpp"
#include "Core/IControllerEventHandler.hpp"
#include "Logic/IMidiMapper.hpp"

namespace GCMidi {

class EventDispatcher : public IControllerEventHandler {
public:
    EventDispatcher();
    ~EventDispatcher() override;

    // Called by SDL Thread (via SdlManager)
    void onControllerConnected(const char* name) override;
    void onControllerDisconnected() override;
    void onControllerButton(uint8_t button, bool pressed, bool shiftState) override;
    void onControllerAxis(uint8_t axis, int16_t value, bool shiftState) override;

    // Called by DSP Thread
    bool popMidi(RawMidi& outEv);
    bool isConnected() const;
    std::string getControllerName() const;
    bool getButtonState(uint8_t button) const;
    IMidiMapper* getMapper();
    bool getAndResetOctaveDirty();

private:
    std::unique_ptr<IMidiMapper> fMapper;
    boost::lockfree::queue<RawMidi> fMidiQueue;

    // UxeI State variables
    std::atomic<bool> fControllerConnected;
    std::string fControllerName;
    std::atomic<bool> fButtonStates[SDL_CONTROLLER_BUTTON_MAX];

    std::atomic<bool> fOctaveDirty;

    // Mutex to protect UI state and mapper access across threads if needed
    // However, for single-instance button states and name, we can use simple atomics or just assume UI thread reads are fine
    mutable std::mutex fStateMutex;
};

}  // namespace GCMidi

#endif  // EVENT_DISPATCHER_HPP_INCLUDED
