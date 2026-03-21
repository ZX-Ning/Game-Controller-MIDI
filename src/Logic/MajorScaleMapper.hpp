#ifndef MAJOR_SCALE_MAPPER_HPP_INCLUDED
#define MAJOR_SCALE_MAPPER_HPP_INCLUDED

#include <SDL.h>

#include "IMidiMapper.hpp"

namespace DISTRHO {

class MajorScaleMapper : public IMidiMapper {
public:
    MajorScaleMapper();
    ~MajorScaleMapper() override = default;

    const char* getName() const override {
        return "C Major Scale (X/Y/B/A)";
    }

    void onButton(uint8_t button, bool pressed, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) override;

private:
    void pushMidiEvent(boost::lockfree::queue<RawMidi>& queue, uint8_t status, uint8_t data1, uint8_t data2);

    // Track active notes per button to send correct NoteOff
    uint8_t fActiveNotes[SDL_CONTROLLER_BUTTON_MAX];
};

}  // namespace DISTRHO

#endif  // MAJOR_SCALE_MAPPER_HPP_INCLUDED
