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

#include <vector>

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class ImGuiMinimalPlugin : public Plugin {
public:
    enum Parameters {
        kParamCount = 0
    };

    struct RawMidi {
        uint8_t data[4];
        uint8_t size;
    };

    ImGuiMinimalPlugin();

protected:
    /* -------------------------------------------------------------------------------------------------------
     * Information */

    const char* getLabel() const override {
        return "ImGuiMinimal";
    }

    const char* getDescription() const override {
        return "Minimal DPF MIDI plugin with ImGui UI";
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
        return d_cconst('d', 'i', 'm', 'p');
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
    static const uint32_t kMidiHistorySize = 128;
    std::vector<RawMidi> fMidiHistory;
    uint32_t fMidiHistoryIndex;

private:
    String fTestState;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiMinimalPlugin)
};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_HPP_INCLUDED
