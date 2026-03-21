/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PLUGIN_HPP_INCLUDED
#define PLUGIN_HPP_INCLUDED

#include <SDL.h>

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class GameControllerMIDIPlugin : public Plugin {
public:
    enum Parameters {
        kParamCount = 0
    };

    struct RawMidi {
        uint8_t data[4];
        uint8_t size;
        uint32_t frame;
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
     * State */

    void initState(uint32_t index, State& state) override;
    String getState(const char* key) const override;
    void setState(const char* key, const char* value) override;

    /* -------------------------------------------------------------------------------------------------------
     * Process */

    void activate() override {}
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

public:
    // For UI feedback
    static const uint32_t kMidiHistorySize = 128;
    std::vector<RawMidi> fMidiHistory;
    std::atomic<uint32_t> fMidiHistoryIndex;

    char fControllerName[256];
    bool fControllerConnected;
    bool fButtonStates[SDL_CONTROLLER_BUTTON_MAX];

    void handleSdlEvent(const SDL_Event& event);
    void pushMidiEvent(uint8_t status, uint8_t data1, uint8_t data2);

private:
    void checkControllerStatus();

    // SDL management handled by SdlManager now
    SDL_GameController* fController;

    // Thread-safe MIDI event queue for internal communication
    boost::lockfree::queue<RawMidi> fMidiQueue;

    // Track active notes per button to send correct NoteOff
    uint8_t fActiveNotes[SDL_CONTROLLER_BUTTON_MAX];

    String fTestState;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameControllerMIDIPlugin)
};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_HPP_INCLUDED
