#include "EventDispatcher.hpp"

#include <algorithm>

namespace GCMidi {

EventDispatcher::EventDispatcher()
    : fMidiQueue(1024),  // Increased from 256 to handle high-frequency axis events
      fPriorityNoteOffQueue(128),
      fControllerConnected(false),
      fTriggerOctaveOffset(0),
      fBaseOctaveOffset(0),
      fRequestedBaseOctaveOffset(0),
      fBaseOctaveRequestPending(false),
      fBaseOctaveDirty(false) {
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        fButtonStates[i].store(false, std::memory_order_relaxed);
    }
}

EventDispatcher::~EventDispatcher() {
}

void EventDispatcher::onControllerConnected(const char* name) {
    std::lock_guard<std::mutex> lock(fStateMutex);
    fControllerConnected.store(true, std::memory_order_relaxed);
    if (name) {
        fControllerName = name;
    }
    else {
        fControllerName = "Unknown Controller";
    }
}

void EventDispatcher::onControllerDisconnected() {
    {
        std::lock_guard<std::mutex> lock(fStateMutex);
        fControllerConnected.store(false, std::memory_order_relaxed);
        fControllerName.clear();
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            fButtonStates[i].store(false, std::memory_order_relaxed);
        }
        fTriggerOctaveOffset.store(0, std::memory_order_relaxed);
        fLeftTriggerPressed = false;
        fRightTriggerPressed = false;
    }

    std::lock_guard<std::mutex> mapperLock(fMapperMutex);
    if (fMapper) {
        fMapper->flushActiveNotes(*this);
        fMapper->setTriggerOctaveOffset(0);
    }
}

void EventDispatcher::onControllerButton(uint8_t button, bool pressed, bool shiftState) {
    if (button < SDL_CONTROLLER_BUTTON_MAX) {
        fButtonStates[button].store(pressed, std::memory_order_relaxed);
    }

    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        applyPendingBaseOctaveOffsetLocked();
        int oldOctave = fMapper->getOctaveOffset();
        fMapper->onButton(button, pressed, shiftState, *this);
        int newOctave = fMapper->getOctaveOffset();
        if (oldOctave != newOctave) {
            fBaseOctaveOffset.store(static_cast<int8_t>(newOctave), std::memory_order_relaxed);
            fRequestedBaseOctaveOffset.store(static_cast<int8_t>(newOctave), std::memory_order_relaxed);
            fBaseOctaveDirty.store(true, std::memory_order_relaxed);
        }
    }
}

void EventDispatcher::onControllerAxis(uint8_t axis, int16_t value, bool shiftState) {
    // Handle trigger octave control
    if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
        bool pressed = (value > TRIGGER_THRESHOLD);
        if (pressed && !fLeftTriggerPressed) {
            // LT just pressed: decrement octave
            int8_t current = fTriggerOctaveOffset.load(std::memory_order_relaxed);
            int8_t next = static_cast<int8_t>(std::max(-4, current - 1));
            fTriggerOctaveOffset.store(next, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(fMapperMutex);
            if (fMapper) {
                fMapper->setTriggerOctaveOffset(next);
            }
        }
        fLeftTriggerPressed = pressed;
        return;  // Don't forward to mapper
    }

    if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
        bool pressed = (value > TRIGGER_THRESHOLD);
        if (pressed && !fRightTriggerPressed) {
            // RT just pressed: increment octave
            int8_t current = fTriggerOctaveOffset.load(std::memory_order_relaxed);
            int8_t next = static_cast<int8_t>(std::min(4, current + 1));
            fTriggerOctaveOffset.store(next, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(fMapperMutex);
            if (fMapper) {
                fMapper->setTriggerOctaveOffset(next);
            }
        }
        fRightTriggerPressed = pressed;
        return;
    }

    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        applyPendingBaseOctaveOffsetLocked();
        fMapper->onAxis(axis, value, shiftState, *this);
    }
}

uint8_t EventDispatcher::getShiftButton() const {
    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        return fMapper->getShiftButton();
    }
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
}

uint8_t EventDispatcher::getShiftButtonForButton(uint8_t button) const {
    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        return fMapper->getShiftButtonForButton(button);
    }
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
}

bool EventDispatcher::popMidi(RawMidi& outEv) {
    if (fPriorityNoteOffQueue.pop(outEv)) {
        return true;
    }
    return fMidiQueue.pop(outEv);
}

bool EventDispatcher::pushMidi(const RawMidi& midi) {
    if (isNoteOff(midi)) {
        if (fPriorityNoteOffQueue.push(midi) || fMidiQueue.push(midi)) {
            return true;
        }

        fDroppedMidiEvents.fetch_add(1, std::memory_order_relaxed);
        fDroppedNoteOffEvents.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (fMidiQueue.push(midi)) {
        return true;
    }

    fDroppedMidiEvents.fetch_add(1, std::memory_order_relaxed);
    return false;
}

// Called by UI Thread
bool EventDispatcher::isConnected() const {
    return fControllerConnected.load(std::memory_order_relaxed);
}

std::string EventDispatcher::getControllerName() const {
    std::lock_guard<std::mutex> lock(fStateMutex);
    return fControllerName;
}

bool EventDispatcher::getButtonState(uint8_t button) const {
    return button < SDL_CONTROLLER_BUTTON_MAX ? fButtonStates[button].load(std::memory_order_relaxed) : false;
}

void EventDispatcher::setMapper(std::unique_ptr<IMidiMapper> mapper) {
    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        if (!fMapper->flushActiveNotes(*this)) {
            return;
        }
    }
    const int oldBaseOctave = fBaseOctaveOffset.load(std::memory_order_relaxed);
    fMapper = std::move(mapper);
    if (fMapper) {
        fMapper->setTriggerOctaveOffset(fTriggerOctaveOffset.load(std::memory_order_relaxed));
        const int newBaseOctave = fMapper->getOctaveOffset();
        fBaseOctaveOffset.store(static_cast<int8_t>(newBaseOctave), std::memory_order_relaxed);
        fRequestedBaseOctaveOffset.store(static_cast<int8_t>(newBaseOctave), std::memory_order_relaxed);
        if (oldBaseOctave != newBaseOctave) {
            fBaseOctaveDirty.store(true, std::memory_order_relaxed);
        }
    }
}

bool EventDispatcher::getAndResetBaseOctaveDirty() {
    return fBaseOctaveDirty.exchange(false);
}

int EventDispatcher::getBaseOctaveOffset() const {
    return fBaseOctaveOffset.load(std::memory_order_relaxed);
}

void EventDispatcher::requestBaseOctaveOffset(int offset) {
    const int8_t clamped = static_cast<int8_t>(std::clamp(offset, -4, 4));
    fBaseOctaveOffset.store(clamped, std::memory_order_relaxed);
    fRequestedBaseOctaveOffset.store(clamped, std::memory_order_relaxed);
    fBaseOctaveRequestPending.store(true, std::memory_order_release);
}

int8_t EventDispatcher::getTriggerOctaveOffset() const {
    return fTriggerOctaveOffset.load(std::memory_order_relaxed);
}

void EventDispatcher::setTriggerOctaveOffset(int8_t offset) {
    const int8_t clamped = std::clamp(offset, int8_t(-4), int8_t(4));
    fTriggerOctaveOffset.store(clamped, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        fMapper->setTriggerOctaveOffset(clamped);
    }
}

void EventDispatcher::deactivate() {
    std::lock_guard<std::mutex> lock(fMapperMutex);
    clearMapperRuntimeStateLocked();
}

uint64_t EventDispatcher::getDroppedMidiEventCount() const {
    return fDroppedMidiEvents.load(std::memory_order_relaxed);
}

uint64_t EventDispatcher::getDroppedNoteOffCount() const {
    return fDroppedNoteOffEvents.load(std::memory_order_relaxed);
}

void EventDispatcher::applyPendingBaseOctaveOffsetLocked() {
    if (!fBaseOctaveRequestPending.exchange(false, std::memory_order_acquire)) {
        return;
    }

    const int8_t requested = fRequestedBaseOctaveOffset.load(std::memory_order_relaxed);
    if (fMapper) {
        fMapper->setOctaveOffset(requested);
    }
}

void EventDispatcher::clearMapperRuntimeStateLocked() {
    if (fMapper) {
        fMapper->flushActiveNotes(*this);
    }
}

bool EventDispatcher::isNoteOff(const RawMidi& midi) {
    if (midi.size < 3) {
        return false;
    }

    const uint8_t status = midi.data[0] & 0xF0;
    return status == 0x80 || (status == 0x90 && midi.data[2] == 0);
}

}  // namespace GCMidi
