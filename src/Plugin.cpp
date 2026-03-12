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

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class ImGuiMinimalPlugin : public Plugin {
public:
    enum Parameters {
        kParamGain = 0,
        kParamCount
    };

    ImGuiMinimalPlugin()
        : Plugin(kParamCount, 0, 0),  // parameters, programs, states
          fGain(1.0f) {
    }

protected:
    /* -------------------------------------------------------------------------------------------------------
     * Information */

    const char* getLabel() const override {
        return "ImGuiMinimal";
    }

    const char* getDescription() const override {
        return "Minimal DPF plugin with ImGui UI";
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
     * Init */

    void initParameter(uint32_t index, Parameter& parameter) override {
        if (index != kParamGain) {
            return;
        }

        parameter.hints = kParameterIsAutomable;
        parameter.name = "Gain";
        parameter.symbol = "gain";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 2.0f;
    }

    /* -------------------------------------------------------------------------------------------------------
     * Internal data */

    float getParameterValue(uint32_t index) const override {
        if (index != kParamGain) {
            return 0.0f;
        }

        return fGain;
    }

    void setParameterValue(uint32_t index, float value) override {
        if (index != kParamGain) {
            return;
        }

        fGain = value;
    }

    /* -------------------------------------------------------------------------------------------------------
     * Process */

    void activate() override {}

    void run(const float** inputs, float** outputs, uint32_t frames) override {
        const float* const inL = inputs[0];
        const float* const inR = inputs[1];
        float* const outL = outputs[0];
        float* const outR = outputs[1];

        for (uint32_t i = 0; i < frames; ++i) {
            outL[i] = inL[i] * fGain;
            outR[i] = inR[i] * fGain;
        }
    }

    /* -------------------------------------------------------------------------------------------------------
     * Callbacks (optional) */

    // void sampleRateChanged(double newSampleRate) override {}

private:
    float fGain;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiMinimalPlugin)
};

Plugin* createPlugin() {
    return new ImGuiMinimalPlugin();
}

END_NAMESPACE_DISTRHO
