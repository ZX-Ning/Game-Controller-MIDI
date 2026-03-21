#ifndef IMIDI_MAPPER_HPP_INCLUDED
#define IMIDI_MAPPER_HPP_INCLUDED

#include <boost/lockfree/queue.hpp>
#include <cstdint>

#include "Common/MidiTypes.hpp"

namespace DISTRHO {

// Interface for translating controller events into MIDI
class IMidiMapper {
public:
    virtual ~IMidiMapper() = default;

    // Returns the name of the mapping
    virtual const char* getName() const = 0;

    // Process a button event and push generated MIDI to the queue
    virtual void onButton(uint8_t button, bool pressed, bool shiftState, boost::lockfree::queue<RawMidi>& outQueue) = 0;
};

}  // namespace DISTRHO

#endif  // IMIDI_MAPPER_HPP_INCLUDED
