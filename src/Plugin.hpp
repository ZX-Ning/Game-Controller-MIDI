#ifndef PLUGIN_HPP_INCLUDED
#define PLUGIN_HPP_INCLUDED

#include <SDL.h>

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <memory>
#include <vector>

#include "Common/MidiTypes.hpp"
#include "Core/IControllerEventHandler.hpp"
#include "DistrhoPlugin.hpp"
#include "Logic/IMidiMapper.hpp"

START_NAMESPACE_DISTRHO

class GameControllerMIDIPlugin : public Plugin, public GCMidi::IControllerEventHandler {
public:
    enum Parameters {
        kParamCount = 0
    };

    GameControllerMIDIPlugin();
    ~GameControllerMIDIPlugin() override;

protected:
    /* -------------------------------------------------------------------------------------------------------
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

    /* -------------------------------------------------------------------------------------------------------
     * Process */

    void activate() override {}
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

public:
    /* -------------------------------------------------------------------------------------------------------
     * IControllerEventHandler implementation */
    void onControllerConnected(const char* name) override;
    void onControllerDisconnected() override;
    void onControllerButton(uint8_t button, bool pressed, bool shiftState) override;

    // For UI feedback
    static const uint32_t kMidiHistorySize = 128;
    std::vector<GCMidi::RawMidi> fMidiHistory;
    std::atomic<uint32_t> fMidiHistoryIndex;

    char fControllerName[256];
    std::atomic<bool> fControllerConnected;
    bool fButtonStates[SDL_CONTROLLER_BUTTON_MAX];

private:
    // Thread-safe MIDI event queue for internal communication
    boost::lockfree::queue<GCMidi::RawMidi> fMidiQueue;

    // Active MIDI Mapping Strategy
    std::unique_ptr<GCMidi::IMidiMapper> fMapper;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameControllerMIDIPlugin)
};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_HPP_INCLUDED
