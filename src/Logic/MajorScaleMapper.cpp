#include "MajorScaleMapper.hpp"

#include <cstring>

namespace DISTRHO {

MajorScaleMapper::MajorScaleMapper() {
    std::memset(fActiveNotes, 0, sizeof(fActiveNotes));
}

void MajorScaleMapper::onButton(uint8_t button, bool pressed, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) {
    if (button >= SDL_CONTROLLER_BUTTON_MAX) {
        return;
    }

    uint8_t note = 0;
    switch (button) {
        case SDL_CONTROLLER_BUTTON_X:
            note = shiftState ? 67 : 60;
            break;  // G4 / C4
        case SDL_CONTROLLER_BUTTON_Y:
            note = shiftState ? 69 : 62;
            break;  // A4 / D4
        case SDL_CONTROLLER_BUTTON_B:
            note = shiftState ? 71 : 64;
            break;  // B4 / E4
        case SDL_CONTROLLER_BUTTON_A:
            note = shiftState ? 72 : 65;
            break;  // C5 / F4
    }

    if (note > 0) {
        if (pressed) {
            fActiveNotes[button] = note;
            pushMidiEvent(outQueue, 0x90, note, 100);  // Note On
        }
        else {
            if (fActiveNotes[button] > 0) {
                pushMidiEvent(outQueue, 0x80, fActiveNotes[button], 0);  // Note Off
                fActiveNotes[button] = 0;
            }
        }
    }
}

void MajorScaleMapper::pushMidiEvent(boost::lockfree::queue<RawMidi>& queue, uint8_t status, uint8_t data1, uint8_t data2) {
    RawMidi ev;
    ev.data[0] = status;
    ev.data[1] = data1;
    ev.data[2] = data2;
    ev.size = 3;
    ev.frame = 0;
    queue.push(ev);
}

}  // namespace DISTRHO
