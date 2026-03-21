#include "MajorScaleMapper.hpp"

#include <cstring>

namespace GCMidi {

MajorScaleMapper::MajorScaleMapper() : fOctaveOffset(0) {
    std::memset(fActiveNotes, 0, sizeof(fActiveNotes));
}

void MajorScaleMapper::onButton(uint8_t button, bool pressed, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) {
    if (button >= SDL_CONTROLLER_BUTTON_MAX) {
        return;
    }

    // Handle Octave Control
    if (pressed) {
        if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
            if (fOctaveOffset > -4) {
                fOctaveOffset--;
            }
            return;
        }
        if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
            if (fOctaveOffset < 4) {
                fOctaveOffset++;
            }
            return;
        }
    }

    uint8_t note = 0;
    switch (button) {
        case SDL_CONTROLLER_BUTTON_X:
            note = 60;
            break;  // C4
        case SDL_CONTROLLER_BUTTON_Y:
            note = 62;
            break;  // D4
        case SDL_CONTROLLER_BUTTON_B:
            note = 64;
            break;  // E4
        case SDL_CONTROLLER_BUTTON_A:
            note = 65;
            break;  // F4
    }

    if (note > 0) {
        // Apply octave and shift
        note += (fOctaveOffset * 12);
        if (shiftState) {
            note += 7;
        }

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

void MajorScaleMapper::onAxis(uint8_t, int16_t, bool, boost::lockfree::queue<RawMidi>&) {
    // Axis logic (pitch bend, etc.) will be added here later
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

}  // namespace GCMidi
