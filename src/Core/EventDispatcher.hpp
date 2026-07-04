#ifndef EVENT_DISPATCHER_HPP_INCLUDED
#define EVENT_DISPATCHER_HPP_INCLUDED

#include <SDL.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "Common/MidiTypes.hpp"
#include "Common/SharedState.hpp"
#include "Core/IControllerEventHandler.hpp"
#include "Core/MidiEventQueue.hpp"
#include "Logic/IMidiMapper.hpp"

namespace GCMidi {

// Trigger threshold for octave control (50% of max trigger value)
inline constexpr int16_t TRIGGER_THRESHOLD = 16384;

/**
 * Bridges SDL controller events to MIDI output and UI-visible state.
 *
 * SDL callbacks update atomics/string state and call the active mapper. Mapper
 * access is serialized by `fMapperMutex`; generated MIDI is handed to
 * `fMidiEvents`, which owns the lock-free audio-thread queue.
 */
class EventDispatcher : public IControllerEventHandler {
public:
    /** Create empty MIDI queues and reset controller/UI state. */
    EventDispatcher();

    /** Destroy after unregistering from `SdlManager`; no callbacks may be active. */
    ~EventDispatcher() override;

    /** SDL-thread callback: store connected state and controller name. */
    void onControllerConnected(const char* name) override;

    /** SDL-thread callback: clear controller state and flush held notes. */
    void onControllerDisconnected() override;

    /** SDL-thread callback: update button state and pass the edge to the mapper. */
    void onControllerButton(uint8_t button, bool pressed, bool shiftState) override;

    /** SDL-thread callback: handle trigger octave edges, then map axis motion. */
    void onControllerAxis(uint8_t axis, int16_t value, bool shiftState) override;

    /** SDL-thread query; locks `fMapperMutex` to read mapper shift settings. */
    uint8_t getShiftButton() const override;

    /** SDL-thread query; returns a per-button shift override when configured. */
    uint8_t getShiftButtonForButton(uint8_t button) const override;

    /** Audio-thread safe: pop one queued MIDI event from `fMidiEvents`. */
    bool popMidi(RawMidi& outEv);

    /** UI/host safe atomic read of controller connection state. */
    bool isConnected() const;

    /** UI helper; locks `fStateMutex` to copy the controller name. */
    std::string getControllerName() const;

    /** UI helper; atomically reads the last known state for one SDL button. */
    bool getButtonState(uint8_t button) const;

    /** Replace the mapper under `fMapperMutex`, flushing old active notes first. */
    void setMapper(std::unique_ptr<IMidiMapper> mapper);

    /** Shared, UI/host-visible mapper control state. */
    SharedState& sharedState();

    /** Shared, UI/host-visible mapper control state. */
    const SharedState& sharedState() const;

    /** Host lifecycle hook; flushes active notes, not for the audio callback. */
    void deactivate();

    /** Atomic counter of MIDI events dropped because queues were full. */
    uint64_t getDroppedMidiEventCount() const;

    /** Atomic counter of dropped Note Off events. */
    uint64_t getDroppedNoteOffCount() const;

private:
    /** Active mapper. Any read, replacement, or call requires `fMapperMutex`. */
    std::unique_ptr<IMidiMapper> fMapper;

    /**
     * Serializes `fMapper` lifetime and every mapper call. This also protects
     * mapper-owned runtime state such as active notes, CC toggles, last axis
     * values, and preset reads. It does not protect queues,
     * atomics, or `fControllerName`. Never lock this from the audio thread.
     */
    mutable std::mutex fMapperMutex;

    /** MIDI queue boundary between mapper/lifecycle producers and audio output. */
    MidiEventQueue fMidiEvents;

    /** SDL-written, UI-read connection flag. */
    std::atomic<bool> fControllerConnected;

    /** Controller name for UI display; protected by `fStateMutex`. */
    std::string fControllerName;

    /** Per-button states written by SDL thread and read by UI thread. */
    std::atomic<bool> fButtonStates[SDL_CONTROLLER_BUTTON_MAX];

    /** Shared octave and host-sync state; owned by this dispatcher. */
    SharedState fSharedState;

    /** SDL-thread-only edge detector for LT octave-down. */
    bool fLeftTriggerPressed = false;

    /** SDL-thread-only edge detector for RT octave-up. */
    bool fRightTriggerPressed = false;

    /** Protects only `fControllerName`; never lock from the audio thread. */
    mutable std::mutex fStateMutex;

    /** Flush active mapper notes. Requires `fMapperMutex`. */
    void clearMapperRuntimeStateLocked();

};

}  // namespace GCMidi

#endif  // EVENT_DISPATCHER_HPP_INCLUDED
