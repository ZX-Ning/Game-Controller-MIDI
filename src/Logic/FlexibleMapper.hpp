#pragma once

#include <SDL.h>

#include <array>
#include <string_view>

#include "IMidiMapper.hpp"
#include "MapperConfig.hpp"

namespace GCMidi {

using MidiQueue = boost::lockfree::queue<RawMidi>;

class FlexibleMapper : public IMidiMapper {
public:
    FlexibleMapper();
    ~FlexibleMapper() override = default;

    const char* getName() const override {
        return fPreset.name.data();
    }

    // Load preset from embedded JSON data
    bool loadPreset(const uint8_t* jsonData, size_t jsonSize);

    // IMidiMapper interface
    void onButton(uint8_t button, bool pressed, bool shift, MidiQueue& queue) override;
    void onAxis(uint8_t axis, int16_t value, bool shift, MidiQueue& queue) override;
    int getOctaveOffset() const override;
    void setOctaveOffset(int offset) override;
    uint8_t getShiftButton() const override;
    uint8_t getShiftButtonForButton(uint8_t button) const override;

private:
    MapperConfig::MapperPreset fPreset{};
    int8_t fOctaveOffset = 0;

    // Track active notes per button for correct NoteOff
    struct ActiveNotes {
        uint8_t count = 0;
        std::array<uint8_t, MapperConfig::MAX_ACTIVE_NOTES> notes{};
    };
    std::array<ActiveNotes, SDL_CONTROLLER_BUTTON_MAX> fActiveNotes{};

    // Track CC toggle states
    std::array<bool, 128> fCCToggleStates{};

    // Track last sent values per axis to avoid redundant messages
    std::array<uint8_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisCCValues{};
    std::array<uint16_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisPitchBendValues{};
    std::array<uint8_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisAftertouchValues{};

    // Internal helpers
    void handleNoteMode(const MapperConfig::ButtonConfig& config, bool pressed, uint8_t button, MidiQueue& queue);
    void handleChordMode(const MapperConfig::ButtonConfig& config, bool pressed, uint8_t button, MidiQueue& queue);
    void handleCCMomentary(const MapperConfig::ButtonConfig& config, bool pressed, MidiQueue& queue);
    void handleCCToggle(const MapperConfig::ButtonConfig& config, bool pressed, MidiQueue& queue);

    void handleAxisCC(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, MidiQueue& queue);
    void handleAxisPitchBend(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, MidiQueue& queue);

    uint8_t clampNote(int note) const;
    float normalizeAxis(int16_t value, const MapperConfig::AxisConfig& config) const;
};

}  // namespace GCMidi
