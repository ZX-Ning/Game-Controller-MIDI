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

#include "DistrhoUI.hpp"
// DearImGui.hpp defines ImGuiWidget and provides ImGui headers
#include "../third_party/DPF-Widgets/opengl/DearImGui.hpp"

START_NAMESPACE_DISTRHO

class ImGuiMinimalUI : public UI,
                       public DGL_NAMESPACE::ImGuiTopLevelWidget {
public:
    ImGuiMinimalUI()
        : UI(400, 300),
          DGL_NAMESPACE::ImGuiTopLevelWidget(UI::getWindow()) {
        fGain = 1.0f;
    }

protected:
    /* -------------------------------------------------------------------------------------------------------
     * DSP/UI Callbacks */

    void parameterChanged(uint32_t index, float value) override {
        if (index == 0) {  // kParamGain
            fGain = value;
            UI::repaint();
        }
    }

    /* -------------------------------------------------------------------------------------------------------
     * ImGui Callbacks */

    void onImGuiDisplay() override {
        const float width = UI::getWidth();
        const float height = UI::getHeight();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("DPF ImGui Minimal");
        ImGui::Separator();

        if (ImGui::SliderFloat("Gain", &fGain, 0.0f, 2.0f)) {
            setParameterValue(0, fGain);
        }

        ImGui::End();
    }

private:
    float fGain;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiMinimalUI)
};

UI* createUI() {
    return new ImGuiMinimalUI();
}

END_NAMESPACE_DISTRHO
