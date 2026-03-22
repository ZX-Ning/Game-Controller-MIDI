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

class GameControllerMIDIUI : public UI {
public:
    GameControllerMIDIUI()
        : UI(400, 350) {
    }

protected:
    void onImGuiDisplay() override {
        const float width = getWidth();
        const float height = getHeight();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("Game Controller MIDI");
        ImGui::Separator();

        GameControllerMIDIPlugin* const plugin = (GameControllerMIDIPlugin*)getPluginInstancePointer();
        if (!plugin || !plugin->fDispatcher) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Plugin or Dispatcher not found");
            ImGui::End();
            return;
        }

        auto& dispatcher = *plugin->fDispatcher;

        // Controller Info
        if (dispatcher.isConnected()) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected: %s", dispatcher.getControllerName().c_str());

            // Show current octave
            if (auto mapper = dispatcher.getMapper()) {
                ImGui::SameLine();
                ImGui::TextDisabled("|");
                ImGui::SameLine();
                ImGui::Text("Octave: %+d", mapper->getOctaveOffset());
                ImGui::SameLine();
                ImGui::TextDisabled("(D-Pad L/R)");
            }
        }
        else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Controller Disconnected");
        }

        ImGui::Separator();
        ImGui::Text("Buttons:");

        auto drawButton = [&](const char* label, int buttonIdx) {
            bool down = dispatcher.getButtonState(buttonIdx);
            if (down) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            }
            ImGui::Button(label, ImVec2(40, 40));
            if (down) {
                ImGui::PopStyleColor();
            }
        };

        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(45, 0));
        ImGui::SameLine();
        drawButton("Y", SDL_CONTROLLER_BUTTON_Y);
        drawButton("X", SDL_CONTROLLER_BUTTON_X);
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(40, 0));
        ImGui::SameLine();
        drawButton("B", SDL_CONTROLLER_BUTTON_B);
        ImGui::Dummy(ImVec2(45, 0));
        ImGui::SameLine();
        drawButton("A", SDL_CONTROLLER_BUTTON_A);
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Text("RB (Shift): %s", dispatcher.getButtonState(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ? "PRESSED" : "released");
        ImGui::EndGroup();

        ImGui::Separator();
        ImGui::Text("Last MIDI Messages:");
        ImGui::BeginChild("MidiList", ImVec2(0, 150), true);

        const uint32_t head = plugin->fMidiHistoryIndex.load();
        for (uint32_t i = 0; i < GameControllerMIDIPlugin::kMidiHistorySize; ++i) {
            const uint32_t idx = (head + GameControllerMIDIPlugin::kMidiHistorySize - i) % GameControllerMIDIPlugin::kMidiHistorySize;
            const uint64_t packed = plugin->fMidiHistory[idx].load(std::memory_order_relaxed);

            if (packed > 0) {
                uint8_t size = packed & 0xFF;
                uint8_t data[4];
                data[0] = (packed >> 8) & 0xFF;
                data[1] = (packed >> 16) & 0xFF;
                data[2] = (packed >> 24) & 0xFF;
                data[3] = (packed >> 32) & 0xFF;

                uint8_t status = data[0] & 0xF0;

                if (status == 0x90 && size >= 3 && data[2] > 0) {
                    int note = data[1];
                    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Note On:  %3d (%s%d) Vel: %d", note, noteNames[note % 12], (note / 12) - 1, data[2]);
                }
                else if ((status == 0x80 && size >= 3) || (status == 0x90 && size >= 3 && data[2] == 0)) {
                    int note = data[1];
                    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "Note Off: %3d (%s%d)", note, noteNames[note % 12], (note / 12) - 1);
                }
                else {
                    if (size == 1) {
                        ImGui::Text("[%02X]", data[0]);
                    }
                    else if (size == 2) {
                        ImGui::Text("[%02X %02X]", data[0], data[1]);
                    }
                    else if (size == 3) {
                        ImGui::Text("[%02X %02X %02X]", data[0], data[1], data[2]);
                    }
                    else if (size == 4) {
                        ImGui::Text("[%02X %02X %02X %02X]", data[0], data[1], data[2], data[3]);
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::End();
        repaint();  // Force repaint to see changes
    }

    void parameterChanged(uint32_t, float) override {
        repaint();
    }

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameControllerMIDIUI)
};

UI* createUI() {
    return new GameControllerMIDIUI();
}

END_NAMESPACE_DISTRHO
