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

#include "Plugin.hpp"

START_NAMESPACE_DISTRHO

ImGuiMinimalPlugin::ImGuiMinimalPlugin()
    : Plugin(kParamCount, 0, 1),  // parameters, programs, states
      fMidiHistoryIndex(0) {
    fTestState = "hello";
    fMidiHistory.resize(kMidiHistorySize);
}

void ImGuiMinimalPlugin::initState(uint32_t index, State& state) {
    if (index != 0) {
        return;
    }

    state.hints = kStateIsHostReadable | kStateIsHostWritable;
    state.key = "test-state";
    state.defaultValue = "hello";
}

String ImGuiMinimalPlugin::getState(const char* key) const {
    if (std::strcmp(key, "test-state") == 0) {
        return fTestState;
    }

    return Plugin::getState(key);
}

void ImGuiMinimalPlugin::setState(const char* key, const char* value) {
    if (std::strcmp(key, "test-state") == 0) {
        fTestState = value;
    }
}

void ImGuiMinimalPlugin::run(const float**, float**, uint32_t, const MidiEvent* midiEvents, uint32_t midiEventCount) {
    for (uint32_t i = 0; i < midiEventCount; ++i) {
        if (midiEvents[i].size <= 4) {
            fMidiHistoryIndex = (fMidiHistoryIndex + 1) % kMidiHistorySize;
            fMidiHistory[fMidiHistoryIndex].size = midiEvents[i].size;
            std::memcpy(fMidiHistory[fMidiHistoryIndex].data, midiEvents[i].data, midiEvents[i].size);
        }
        writeMidiEvent(midiEvents[i]);
    }
}

Plugin* createPlugin() {
    return new ImGuiMinimalPlugin();
}

END_NAMESPACE_DISTRHO
