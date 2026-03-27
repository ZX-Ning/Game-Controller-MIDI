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

#include "DistrhoPluginInfo.h"
#include "DistrhoUI.hpp"
#include "Plugin.hpp"

START_NAMESPACE_DISTRHO

class GameControllerMIDIUI : public UI {
public:
    GameControllerMIDIUI()
        : UI(400, 200) {
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
