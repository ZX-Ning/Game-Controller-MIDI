#ifndef MAJOR_SCALE_MAPPER_HPP_INCLUDED
#define MAJOR_SCALE_MAPPER_HPP_INCLUDED

#include <SDL.h>

#include "IMidiMapper.hpp"

namespace GCMidi {

class MajorScaleMapper : public IMidiMapper {
public:
    MajorScaleMapper();
    ~MajorScaleMapper() override = default;

    const char* getName() const override {
        return "C Major Scale (X/Y/B/A)";
    }

    void onButton(uint8_t button, bool pressed, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) override;

    void onAxis(uint8_t axis, int16_t value, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) override;

    int getOctaveOffset() const override {
        return fOctaveOffset;
    }

    void setOctaveOffset(int offset) override {
        if (offset < -4) {
            offset = -4;
        }
        if (offset > 4) {
            offset = 4;
        }
        fOctaveOffset = offset;
    }

private:
    void pushMidiEvent(boost::lockfree::queue<RawMidi>& queue, uint8_t status, uint8_t data1, uint8_t data2);

    // Track active notes per button to send correct NoteOff
    uint8_t fActiveNotes[SDL_CONTROLLER_BUTTON_MAX];

    int fOctaveOffset;
};

}  // namespace GCMidi

#endif  // MAJOR_SCALE_MAPPER_HPP_INCLUDED
