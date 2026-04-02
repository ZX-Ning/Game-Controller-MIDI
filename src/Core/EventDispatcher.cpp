#include "EventDispatcher.hpp"

namespace GCMidi {

EventDispatcher::EventDispatcher()
    : fMidiQueue(256),
      fControllerConnected(false),
      fOctaveDirty(false) {
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
    std::lock_guard<std::mutex> lock(fStateMutex);
    fControllerConnected.store(false, std::memory_order_relaxed);
    fControllerName.clear();
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        fButtonStates[i].store(false, std::memory_order_relaxed);
    }
}

void EventDispatcher::onControllerButton(uint8_t button, bool pressed, bool shiftState) {
    if (button < SDL_CONTROLLER_BUTTON_MAX) {
        fButtonStates[button].store(pressed, std::memory_order_relaxed);
    }

    if (fMapper) {
        int oldOctave = fMapper->getOctaveOffset();
        fMapper->onButton(button, pressed, shiftState, fMidiQueue);
        if (oldOctave != fMapper->getOctaveOffset()) {
            fOctaveDirty.store(true, std::memory_order_relaxed);
        }
    }
}

void EventDispatcher::onControllerAxis(uint8_t axis, int16_t value, bool shiftState) {
    if (fMapper) {
        fMapper->onAxis(axis, value, shiftState, fMidiQueue);
    }
}

uint8_t EventDispatcher::getShiftButton() const {
    if (fMapper) {
        return fMapper->getShiftButton();
    }
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
}

bool EventDispatcher::popMidi(RawMidi& outEv) {
    return fMidiQueue.pop(outEv);
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

IMidiMapper* EventDispatcher::getMapper() {
    return fMapper.get();
}

void EventDispatcher::setMapper(std::unique_ptr<IMidiMapper> mapper) {
    std::lock_guard<std::mutex> lock(fStateMutex);
    fMapper = std::move(mapper);
}

bool EventDispatcher::getAndResetOctaveDirty() {
    return fOctaveDirty.exchange(false);
}

}  // namespace GCMidi
