#ifndef EVENT_DISPATCHER_HPP_INCLUDED
#define EVENT_DISPATCHER_HPP_INCLUDED

#include <SDL.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "Common/MidiTypes.hpp"
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

    /** Atomically report and clear a mapper-driven base-octave change. */
    bool getAndResetBaseOctaveDirty();

    /** Atomic mirror of the mapper base octave, for UI/host display. */
    int getBaseOctaveOffset() const;

    /** Request a base-octave change; mapper update is deferred to SDL thread. */
    void requestBaseOctaveOffset(int offset);

    /** Atomic read of the transient LT/RT trigger octave offset. */
    int8_t getTriggerOctaveOffset() const;

    /** Set trigger octave and update the mapper under `fMapperMutex`. */
    void setTriggerOctaveOffset(int8_t offset);

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
     * mapper-owned state such as active notes, CC toggles, last axis values,
     * preset reads, and mapper octave fields. It does not protect queues,
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

    /** Public trigger-octave mirror; mapper copy is changed under `fMapperMutex`. */
    std::atomic<int8_t> fTriggerOctaveOffset{0};

    /** Public base-octave mirror; mapper copy is changed under `fMapperMutex`. */
    std::atomic<int8_t> fBaseOctaveOffset{0};

    /** Pending base-octave value requested by UI/host and consumed by SDL thread. */
    std::atomic<int8_t> fRequestedBaseOctaveOffset{0};

    /** True when `fRequestedBaseOctaveOffset` still needs applying to mapper. */
    std::atomic<bool> fBaseOctaveRequestPending{false};

    /** SDL-thread-only edge detector for LT octave-down. */
    bool fLeftTriggerPressed = false;

    /** SDL-thread-only edge detector for RT octave-up. */
    bool fRightTriggerPressed = false;

    /** Mapper-driven base-octave changes set this for host/UI synchronization. */
    std::atomic<bool> fBaseOctaveDirty;

    /** Protects only `fControllerName`; never lock from the audio thread. */
    mutable std::mutex fStateMutex;

    /** Apply a pending base-octave request. Requires `fMapperMutex`. */
    void applyPendingBaseOctaveOffsetLocked();

    /** Flush active mapper notes. Requires `fMapperMutex`. */
    void clearMapperRuntimeStateLocked();

};

}  // namespace GCMidi

#endif  // EVENT_DISPATCHER_HPP_INCLUDED
