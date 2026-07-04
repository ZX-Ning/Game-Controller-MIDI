#ifndef PLUGIN_HPP_INCLUDED
#define PLUGIN_HPP_INCLUDED

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

#include "Core/EventDispatcher.hpp"
#include "DistrhoPlugin.hpp"
#include "Logic/MapperConfig.hpp"

START_NAMESPACE_DISTRHO

/**
 * DPF plugin instance.
 *
 * Owns the dispatcher, active preset, host parameters/states, and audio-thread
 * MIDI output loop. `run()` only drains dispatcher queues and writes MIDI to the
 * host; JSON parsing, mapper replacement, SDL lifecycle, and config locking all
 * stay outside the audio callback.
 */
class GameControllerMIDIPlugin : public Plugin {
public:
    /** Host-visible automatable parameters. */
    enum Parameters {
        kParamOctave = 0,
        kParamCount
    };

    /** Host-persisted state keys; never serialized/deserialized from `run()`. */
    enum States {
        kStateConfig = 0,     // JSON serialized MapperPreset
        kStateTriggerOctave,  // Current trigger octave offset
        kStateEditMode,       // "true" (editing) or "false" (playing)
        kStateWidth,          // Window width
        kStateHeight,         // Window height
        kStateCount
    };

    /** Create dispatcher, load default preset, and register for SDL events. */
    GameControllerMIDIPlugin();

    /** Unregister from SDL and release plugin resources. */
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

    /** Describe one host-visible parameter. */
    void initParameter(uint32_t index, Parameter& parameter) override;

    /** Return current parameter value from atomics/dispatcher state. */
    float getParameterValue(uint32_t index) const override;

    /** Apply parameter changes to dispatcher-owned atomic state. */
    void setParameterValue(uint32_t index, float value) override;

    /* ------------------------------------------------------------------
     * State */

    /** Describe one host-persisted state entry. */
    void initState(uint32_t index, State& state) override;

    /** Apply persisted/UI state. Config JSON uses `fConfigMutex` and is not real-time safe. */
    void setState(const char* key, const char* value) override;

    /** Serialize current state; may allocate, so never call from audio thread. */
    String getState(const char* key) const override;

    /* -----------------------------------------------------------------
     * Process */

    void activate() override {}

    /** Flush active mapper notes from the host lifecycle thread. */
    void deactivate() override;

    /** Audio callback: drain lock-free MIDI queues and write host MIDI output. */
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

public:
    /** UI-safe access to controller state and MIDI diagnostics. */
    GCMidi::EventDispatcher* dispatcher();

    /** Copy the active preset under the plugin config mutex. */
    MapperConfig::MapperPreset activeConfigCopy() const;

    /** Replace the active preset and rebuild the mapper outside the config lock. */
    void setActiveConfig(const MapperConfig::MapperPreset& config);

    /** Mirror the current UI mode for host state serialization. */
    void setPlayMode(bool playMode);

private:
    // Core logic hub; owns the thread-safe bridge to SDL and audio output.
    std::unique_ptr<GCMidi::EventDispatcher> fDispatcher;

    // Active preset used to build mapper instances. Guard with `fConfigMutex`.
    MapperConfig::MapperPreset fActiveConfig{};

    /**
     * Protects only `fActiveConfig`.
     *
     * Accessed by the UI/host lifecycle thread through `activeConfigCopy()`,
     * `setActiveConfig()`, `setState("config")`, `getState("config")`, and
     * `reloadMapper()`. The audio callback thread must never lock this;
     * The SDL polling thread does not access plugin config directly.
     */
    mutable std::mutex fConfigMutex;

    // UI/host state mirrors. Atomics avoid locking for simple reads/writes.
    std::atomic<bool> fPlayMode{true};  // true = Play, false = Edit
    std::atomic<uint32_t> fWidth{1000};
    std::atomic<uint32_t> fHeight{500};

    /** Rebuild mapper from `fActiveConfig`; may allocate and flush notes. */
    void reloadMapper();

    /** Process-wide instance count used around global SDL lifecycle assumptions. */
    static std::atomic<int> sInstanceCount;

    /** Audio-thread-owned MIDI event retried on the next callback. */
    std::optional<GCMidi::RawMidi> fPendingMidiOutput;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameControllerMIDIPlugin)
};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_HPP_INCLUDED
