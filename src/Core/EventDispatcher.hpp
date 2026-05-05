#ifndef EVENT_DISPATCHER_HPP_INCLUDED
#define EVENT_DISPATCHER_HPP_INCLUDED

#include <SDL.h>

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "Common/MidiTypes.hpp"
#include "Core/IControllerEventHandler.hpp"
#include "Logic/IMidiMapper.hpp"

namespace GCMidi {

// Trigger threshold for octave control (50% of max trigger value)
inline constexpr int16_t TRIGGER_THRESHOLD = 16384;

class EventDispatcher : public IControllerEventHandler, public IMidiOutputSink {
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
    bool pushMidi(const RawMidi& midi) override;

    void setMapper(std::unique_ptr<IMidiMapper> mapper);

    bool getAndResetBaseOctaveDirty();

    // Trigger octave offset for melody transposition
    int getBaseOctaveOffset() const;
    void requestBaseOctaveOffset(int offset);
    int8_t getTriggerOctaveOffset() const;
    void setTriggerOctaveOffset(int8_t offset);
    void deactivate();

    uint64_t getDroppedMidiEventCount() const;
    uint64_t getDroppedNoteOffCount() const;

private:
    std::unique_ptr<IMidiMapper> fMapper;
    mutable std::mutex fMapperMutex;
    boost::lockfree::queue<RawMidi> fMidiQueue;
    boost::lockfree::queue<RawMidi> fPriorityNoteOffQueue;

    // UI State variables
    std::atomic<bool> fControllerConnected;
    std::string fControllerName;
    std::atomic<bool> fButtonStates[SDL_CONTROLLER_BUTTON_MAX];

    std::atomic<int8_t> fTriggerOctaveOffset{0};
    std::atomic<int8_t> fBaseOctaveOffset{0};
    std::atomic<int8_t> fRequestedBaseOctaveOffset{0};
    std::atomic<bool> fBaseOctaveRequestPending{false};
    bool fLeftTriggerPressed = false;
    bool fRightTriggerPressed = false;

    std::atomic<bool> fBaseOctaveDirty;
    std::atomic<uint64_t> fDroppedMidiEvents{0};
    std::atomic<uint64_t> fDroppedNoteOffEvents{0};

    // Mutex to protect UI state and mapper access across threads if needed
    // However, for single-instance button states and name, we can use simple atomics or just assume UI thread reads are fine
    mutable std::mutex fStateMutex;

    void applyPendingBaseOctaveOffsetLocked();
    void clearMapperRuntimeStateLocked();
    static bool isNoteOff(const RawMidi& midi);
};

}  // namespace GCMidi

#endif  // EVENT_DISPATCHER_HPP_INCLUDED
