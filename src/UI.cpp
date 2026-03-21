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

#include <cstdio>
#include <cstring>

#include "DistrhoPluginInfo.h"
#include "DistrhoUI.hpp"
#include "Plugin.hpp"

START_NAMESPACE_DISTRHO

class ImGuiMinimalUI : public UI {
public:
    ImGuiMinimalUI()
        : UI(400, 300) {
        fTestState = "hello";
        std::strncpy(fStateBuf, "hello", sizeof(fStateBuf) - 1);
        fStateBuf[sizeof(fStateBuf) - 1] = '\0';
    }

protected:
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "test-state") == 0) {
            fTestState = value;
            std::strncpy(fStateBuf, value, sizeof(fStateBuf) - 1);
            fStateBuf[sizeof(fStateBuf) - 1] = '\0';
            repaint();
        }
    }

    void onImGuiDisplay() override {
        const float width = getWidth();
        const float height = getHeight();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("DPF ImGui MIDI Minimal");
        ImGui::Separator();

        ImGui::Text("Last MIDI Messages:");
        ImGui::BeginChild("MidiList", ImVec2(0, 150), true);

        // Use direct access to get the plugin instance
        if (ImGuiMinimalPlugin* const plugin = (ImGuiMinimalPlugin*)getPluginInstancePointer()) {
            const uint32_t head = plugin->fMidiHistoryIndex;
            for (uint32_t i = 0; i < ImGuiMinimalPlugin::kMidiHistorySize; ++i) {
                // Show from newest to oldest
                const uint32_t idx = (head + ImGuiMinimalPlugin::kMidiHistorySize - i) % ImGuiMinimalPlugin::kMidiHistorySize;
                const auto& msg = plugin->fMidiHistory[idx];

                if (msg.size > 0) {
                    if (msg.size == 1) {
                        ImGui::Text("[%02X]", msg.data[0]);
                    }
                    else if (msg.size == 2) {
                        ImGui::Text("[%02X %02X]", msg.data[0], msg.data[1]);
                    }
                    else if (msg.size == 3) {
                        ImGui::Text("[%02X %02X %02X]", msg.data[0], msg.data[1], msg.data[2]);
                    }
                    else if (msg.size == 4) {
                        ImGui::Text("[%02X %02X %02X %02X]", msg.data[0], msg.data[1], msg.data[2], msg.data[3]);
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Text("State: %s", fTestState.buffer());

        ImGui::InputText("New State", fStateBuf, sizeof(fStateBuf));
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            fTestState = fStateBuf;
            setState("test-state", fStateBuf);
        }

        ImGui::End();
    }

    void parameterChanged(uint32_t, float) override {
        repaint();
    }

private:
    String fTestState;
    char fStateBuf[64];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiMinimalUI)
};

UI* createUI() {
    return new ImGuiMinimalUI();
}

END_NAMESPACE_DISTRHO
