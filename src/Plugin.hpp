#ifndef PLUGIN_HPP_INCLUDED
#define PLUGIN_HPP_INCLUDED

#include <atomic>
#include <memory>
#include <mutex>

#include "Core/EventDispatcher.hpp"
#include "DistrhoPlugin.hpp"
#include "Logic/MapperConfig.hpp"

START_NAMESPACE_DISTRHO

class GameControllerMIDIPlugin : public Plugin {
public:
    enum Parameters {
        kParamOctave = 0,
        kParamCount
    };

    enum States {
        kStateConfig = 0,     // JSON serialized MapperPreset
        kStateTriggerOctave,  // Current trigger octave offset
        kStateEditMode,       // "true" (editing) or "false" (playing)
        kStateWidth,          // Window width
        kStateHeight,         // Window height
        kStateCount
    };

    GameControllerMIDIPlugin();
    ~GameControllerMIDIPlugin() override;

protected:
    /* -----------------------------------------------------
     * Information */

    const char* getLabel() const override {
        return "GameControllerMIDI";
    }
    const char* getDescription() const override {
        return "Game controller to MIDI plugin";
    }
    const char* getMaker() const override {
        return "ZX-NING";
    }
    const char* getHomePage() const override {
        return "https://github.com/zx-ning/GameControllerMIDI";
    }
    const char* getLicense() const override {
        return "MIT";
    }
    uint32_t getVersion() const override {
        return d_version(1, 0, 0);
    }
    int64_t getUniqueId() const override {
        return d_cconst('g', 'c', 'm', 'i');
    }

    /* ------------------------------------------------------------------
     * Parameters */

    void initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void setParameterValue(uint32_t index, float value) override;

    /* ------------------------------------------------------------------
     * State */

    void initState(uint32_t index, State& state) override;
    void setState(const char* key, const char* value) override;
    String getState(const char* key) const override;

    /* -----------------------------------------------------------------
     * Process */

    void activate() override {}
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

public:
    // Core logic hub
    std::unique_ptr<GCMidi::EventDispatcher> fDispatcher;

    // Active configuration (used by mapper)
    MapperConfig::MapperPreset fActiveConfig{};

    // Thread safety for config access
    mutable std::mutex fConfigMutex;

    // Runtime state
    std::atomic<int8_t> fTriggerOctaveOffset{0};  // Cumulative LT/RT offset
    std::atomic<bool> fPlayMode{true};            // true = Play, false = Edit
    std::atomic<uint32_t> fWidth{1000};
    std::atomic<uint32_t> fHeight{500};

    // Helper to reload mapper from fActiveConfig
    void reloadMapper();

private:
    static std::atomic<int> sInstanceCount;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameControllerMIDIPlugin)
};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_HPP_INCLUDED
