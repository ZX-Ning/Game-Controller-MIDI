#ifndef PLUGIN_HPP_INCLUDED
#define PLUGIN_HPP_INCLUDED

#include <atomic>
#include <memory>

#include "Core/EventDispatcher.hpp"
#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class GameControllerMIDIPlugin : public Plugin {
public:
    enum Parameters {
        kParamOctave = 0,
        kParamCount
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
        return "DISTRHO";
    }
    const char* getHomePage() const override {
        return "https://github.com/DISTRHO/DPF";
    }
    const char* getLicense() const override {
        return "ISC";
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

    /* -----------------------------------------------------------------
     * Process */

    void activate() override {}
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

public:
    // Core logic hub
    std::unique_ptr<GCMidi::EventDispatcher> fDispatcher;

private:
    static std::atomic<int> sInstanceCount;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameControllerMIDIPlugin)
};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_HPP_INCLUDED
