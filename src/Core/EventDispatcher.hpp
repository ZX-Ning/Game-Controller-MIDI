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

    // Interface IControllerEventHandler implement
    // Called by SDL Thread (via SdlManager)
    void onControllerConnected(const char* name) override;
    void onControllerDisconnected() override;
    void onControllerButton(uint8_t button, bool pressed, bool shiftState) override;
    void onControllerAxis(uint8_t axis, int16_t value, bool shiftState) override;

    uint8_t getShiftButton() const override;
    uint8_t getShiftButtonForButton(uint8_t button) const override;

    // Called by DSP Thread
    bool popMidi(RawMidi& outEv);
    bool isConnected() const;
    std::string getControllerName() const;
    bool getButtonState(uint8_t button) const;
    IMidiMapper* getMapper();

    /**
     * Set the MIDI mapper.
     *
     * IMPORTANT: This method is NOT fully thread-safe for dynamic switching.
     * It should only be called during initialization (before SDL events start)
     * or from the same thread that calls onControllerButton/onControllerAxis.
     *
     * For dynamic mapper switching during playback, a lock-free mechanism
     * or atomic shared_ptr would be required.
     */
    void setMapper(std::unique_ptr<IMidiMapper> mapper);

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
