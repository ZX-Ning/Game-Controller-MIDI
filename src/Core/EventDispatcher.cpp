#include "EventDispatcher.hpp"

namespace GCMidi {

EventDispatcher::EventDispatcher()
    : fControllerConnected(false) {
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        fButtonStates[i].store(false, std::memory_order_relaxed);
    }
}

EventDispatcher::~EventDispatcher() {
}

void EventDispatcher::onControllerConnected(const char* name) {
    std::lock_guard<std::mutex> lock(fControllerNameMutex);
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
        std::lock_guard<std::mutex> lock(fControllerNameMutex);
        fControllerConnected.store(false, std::memory_order_relaxed);
        fControllerName.clear();
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            fButtonStates[i].store(false, std::memory_order_relaxed);
        }
        fLeftTriggerPressed = false;
        fRightTriggerPressed = false;
    }

    std::lock_guard<std::mutex> mapperLock(fMapperMutex);
    if (fMapper) {
        fMapper->flushActiveNotes(fMidiEvents);
    }
    fSharedState.resetTriggerOctave();
}

void EventDispatcher::onControllerButton(uint8_t button, bool pressed, bool shiftState) {
    if (button < SDL_CONTROLLER_BUTTON_MAX) {
        fButtonStates[button].store(pressed, std::memory_order_relaxed);
    }

    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        fMapper->onButton(button, pressed, shiftState, fSharedState, fMidiEvents);
    }
}

void EventDispatcher::onControllerAxis(uint8_t axis, int16_t value, bool shiftState) {
    // Handle trigger octave control
    if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
        bool pressed = (value > TRIGGER_THRESHOLD);
        if (pressed && !fLeftTriggerPressed) {
            // LT just pressed: decrement octave
            fSharedState.adjustTriggerOctave(-1);
        }
        fLeftTriggerPressed = pressed;
        return;  // Don't forward to mapper
    }

    if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
        bool pressed = (value > TRIGGER_THRESHOLD);
        if (pressed && !fRightTriggerPressed) {
            // RT just pressed: increment octave
            fSharedState.adjustTriggerOctave(1);
        }
        fRightTriggerPressed = pressed;
        return;
    }

    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        fMapper->onAxis(axis, value, shiftState, fSharedState, fMidiEvents);
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
    return fMidiEvents.popMidi(outEv);
}

// Called by UI Thread
bool EventDispatcher::isConnected() const {
    return fControllerConnected.load(std::memory_order_relaxed);
}

std::string EventDispatcher::getControllerName() const {
    std::lock_guard<std::mutex> lock(fControllerNameMutex);
    return fControllerName;
}

bool EventDispatcher::getButtonState(uint8_t button) const {
    return button < SDL_CONTROLLER_BUTTON_MAX ? fButtonStates[button].load(std::memory_order_relaxed) : false;
}

void EventDispatcher::setMapper(std::unique_ptr<IMidiMapper> mapper) {
    std::lock_guard<std::mutex> lock(fMapperMutex);
    if (fMapper) {
        if (!fMapper->flushActiveNotes(fMidiEvents)) {
            return;
        }
    }
    fMapper = std::move(mapper);
    if (fMapper) {
        fSharedState.setBaseOctaveFromPreset(fMapper->getInitialBaseOctaveOffset());
    }
}

SharedState& EventDispatcher::sharedState() {
    return fSharedState;
}

const SharedState& EventDispatcher::sharedState() const {
    return fSharedState;
}

void EventDispatcher::deactivate() {
    std::lock_guard<std::mutex> lock(fMapperMutex);
    clearMapperRuntimeStateLocked();
}

uint64_t EventDispatcher::getDroppedMidiEventCount() const {
    return fMidiEvents.getDroppedMidiEventCount();
}

uint64_t EventDispatcher::getDroppedNoteOffCount() const {
    return fMidiEvents.getDroppedNoteOffCount();
}

void EventDispatcher::clearMapperRuntimeStateLocked() {
    if (fMapper) {
        fMapper->flushActiveNotes(fMidiEvents);
    }
}

}  // namespace GCMidi
