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
#include "preset_c_major_scale_chords.hpp"

START_NAMESPACE_DISTRHO

class GameControllerMIDIUI : public UI {
public:
    GameControllerMIDIUI()
        : UI(1000, 500) {
        setGeometryConstraints(1000, 500);  // Set minimum size
    }

protected:
    void onImGuiDisplay() override {
        const float width = getWidth();
        const float height = getHeight();

        // Remove window padding to fill entire plugin window
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // Add padding back for content area
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

        auto* plugin = static_cast<GameControllerMIDIPlugin*>(getPluginInstancePointer());
        if (!plugin || !plugin->fDispatcher) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Plugin or Dispatcher not found");
            ImGui::End();
            ImGui::PopStyleVar(2);
            return;
        }

        if (fIsEditMode) {
            renderEditModeUI(plugin);
        }
        else {
            renderPlayModeUI(plugin);
        }

        ImGui::PopStyleVar();
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void parameterChanged(uint32_t, float) override {
        repaint();
    }

    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "editMode") == 0) {
            fIsEditMode = (std::strcmp(value, "true") == 0);
        }
        else if (std::strcmp(key, "width") == 0) {
            uint32_t w = static_cast<uint32_t>(std::atoi(value));
            if (w != getWidth()) {
                setSize(w, getHeight());
            }
        }
        else if (std::strcmp(key, "height") == 0) {
            uint32_t h = static_cast<uint32_t>(std::atoi(value));
            if (h != getHeight()) {
                setSize(getWidth(), h);
            }
        }
    }

    void onResize(const ResizeEvent& ev) override {
        UI::onResize(ev);  // Update ImGui DisplaySize

        // Notify host of new size
        setState("width", std::to_string(ev.size.getWidth()).c_str());
        setState("height", std::to_string(ev.size.getHeight()).c_str());

        repaint();  // Force redraw
    }

private:
    // Edit mode state
    bool fIsEditMode = false;
    MapperConfig::MapperPreset fDraftConfig{};  // Draft copy

    // Selected item for editing
    int fSelectedButton = -1;
    int fSelectedAxis = -1;
    bool fEditingShiftButton = false;

    // Helper methods
    void renderPlayModeUI(GameControllerMIDIPlugin* plugin);
    void renderEditModeUI(GameControllerMIDIPlugin* plugin);

    void renderButtonListEditor(std::array<MapperConfig::ButtonConfig, SDL_CONTROLLER_BUTTON_MAX>& buttons, bool isShift);
    void renderButtonEditor(int buttonIndex, MapperConfig::ButtonConfig& config);

    void renderAxisListEditor(std::array<MapperConfig::AxisConfig, SDL_CONTROLLER_AXIS_MAX>& axes, bool isShift);
    void renderAxisEditor(int axisIndex, MapperConfig::AxisConfig& config);

    void renderChordIntervalEditor(MapperConfig::ButtonConfig& config);

    void loadDraftFromActive(GameControllerMIDIPlugin* plugin);
    void applyDraftToActive(GameControllerMIDIPlugin* plugin);

    const char* getButtonName(int button) {
        if (button < 0 || button >= SDL_CONTROLLER_BUTTON_MAX) {
            return "None";
        }
        const char* name = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)button);
        return name ? name : "Unknown";
    }

    const char* getAxisName(int axis) {
        if (axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX) {
            return "None";
        }
        const char* name = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)axis);
        return name ? name : "Unknown";
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameControllerMIDIUI)
};

UI* createUI() {
    return new GameControllerMIDIUI();
}

void GameControllerMIDIUI::renderPlayModeUI(GameControllerMIDIPlugin* plugin) {
    auto& dispatcher = *plugin->fDispatcher;

    ImGui::Text("Game Controller MIDI - Play Mode");
    ImGui::Separator();

    // Connection status
    if (dispatcher.isConnected()) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected: %s", dispatcher.getControllerName().c_str());

        // Octave display (base + trigger)
        if (auto mapper = dispatcher.getMapper()) {
            int baseOctave = mapper->getOctaveOffset();
            int triggerOctave = mapper->getTriggerOctaveOffset();
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::Text("Base Octave: %+d", baseOctave);
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::Text("Trigger Octave: %+d", triggerOctave);
            ImGui::SameLine();
            ImGui::TextDisabled("(LT/RT)");
        }
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Controller Disconnected");
    }

    ImGui::Separator();

    auto drawButton = [&](const char* label, int buttonIdx) {
        bool down = dispatcher.getButtonState(buttonIdx);
        if (down) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        }
        ImGui::Button(label, ImVec2(40, 40));
        if (down) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Button: %s", label);
            if (auto mapper = dispatcher.getMapper()) {
                uint8_t shiftBtn = mapper->getShiftButtonForButton(buttonIdx);
                ImGui::Text("Shift: %s", getButtonName(shiftBtn));
            }
            ImGui::EndTooltip();
        }
    };

    ImGui::BeginGroup();
    ImGui::Text("D-Pad / ABXY:");
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(45, 0));
    ImGui::SameLine();
    drawButton("U", SDL_CONTROLLER_BUTTON_DPAD_UP);
    drawButton("L", SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(40, 0));
    ImGui::SameLine();
    drawButton("R", SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    ImGui::Dummy(ImVec2(45, 0));
    ImGui::SameLine();
    drawButton("D", SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();

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
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(40, 0));
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Shift Status:");
    if (auto mapper = dispatcher.getMapper()) {
        bool shiftButtonsUsed[SDL_CONTROLLER_BUTTON_MAX] = {false};
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            uint8_t s = mapper->getShiftButtonForButton(i);
            if (s < SDL_CONTROLLER_BUTTON_MAX) {
                shiftButtonsUsed[s] = true;
            }
        }

        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            if (shiftButtonsUsed[i]) {
                bool pressed = dispatcher.getButtonState(i);
                ImGui::Text("- %s: %s", getButtonName(i), pressed ? "PRESSED" : "released");
            }
        }
    }
    ImGui::EndGroup();

    ImGui::Separator();

    if (ImGui::Button("Edit Config", ImVec2(-1, 40))) {
        fIsEditMode = true;
        loadDraftFromActive(plugin);
        plugin->fPlayMode = false;
        setState("editMode", "true");
    }
}

void GameControllerMIDIUI::renderEditModeUI(GameControllerMIDIPlugin* plugin) {
    const float width = getWidth();
    const float height = getHeight();
    const float footerHeight = 60.0f;  // Height for action buttons

    // Scrollable editor area
    ImGui::BeginChild("EditorMain", ImVec2(0, height - footerHeight - 30), false);

    ImGui::Text("Config Editor");
    ImGui::Separator();

    // Preset metadata
    ImGui::InputText("Preset Name", fDraftConfig.name.data(), fDraftConfig.name.size());

    int channel = fDraftConfig.channel;
    if (ImGui::SliderInt("MIDI Channel", &channel, 0, 15)) {
        fDraftConfig.channel = static_cast<uint8_t>(channel);
    }

    int baseOctave = fDraftConfig.baseOctaveOffset;
    if (ImGui::SliderInt("Base Octave Offset", &baseOctave, -4, 4)) {
        fDraftConfig.baseOctaveOffset = static_cast<int8_t>(baseOctave);
    }

    // Global shift button selector
    if (ImGui::BeginCombo("Global Shift Button", getButtonName(fDraftConfig.shiftButton))) {
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            bool selected = (i == fDraftConfig.shiftButton);
            if (ImGui::Selectable(getButtonName(i), selected)) {
                fDraftConfig.shiftButton = static_cast<uint8_t>(i);
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // Tabs
    if (ImGui::BeginTabBar("ConfigTabs")) {
        if (ImGui::BeginTabItem("Normal Buttons")) {
            renderButtonListEditor(fDraftConfig.buttons, false);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shift Buttons")) {
            renderButtonListEditor(fDraftConfig.shiftButtons, true);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Axes")) {
            renderAxisListEditor(fDraftConfig.axes, false);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shift Axes")) {
            renderAxisListEditor(fDraftConfig.shiftAxes, true);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::EndChild();

    ImGui::Separator();

    // Action buttons
    if (ImGui::Button("Apply & Play", ImVec2(width * 0.31f, 40))) {
        applyDraftToActive(plugin);
        fIsEditMode = false;
        plugin->fPlayMode = true;
        setState("editMode", "false");
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(width * 0.31f, 40))) {
        fIsEditMode = false;
        plugin->fPlayMode = true;
        setState("editMode", "false");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Default", ImVec2(width * 0.31f, 40))) {
        ImGui::OpenPopup("Confirm Reset");
    }

    if (ImGui::BeginPopupModal("Confirm Reset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Reset current configuration to default?");
        ImGui::Separator();
        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            std::string jsonStr(reinterpret_cast<const char*>(preset_c_major_scale_chords_data), preset_c_major_scale_chords_size);
            MapperConfig::deserializePreset(jsonStr, fDraftConfig);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GameControllerMIDIUI::renderButtonListEditor(std::array<MapperConfig::ButtonConfig, SDL_CONTROLLER_BUTTON_MAX>& buttons, bool isShift) {
    ImGui::BeginChild("ButtonList", ImVec2(150, 0), true);
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        bool selected = (fSelectedButton == i && fEditingShiftButton == isShift);
        if (ImGui::Selectable(getButtonName(i), selected)) {
            fSelectedButton = i;
            fEditingShiftButton = isShift;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ButtonEditor", ImVec2(0, 0), true);
    if (fSelectedButton >= 0 && fEditingShiftButton == isShift) {
        renderButtonEditor(fSelectedButton, buttons[fSelectedButton]);
    }
    else {
        ImGui::TextDisabled("Select a button to edit");
    }
    ImGui::EndChild();
}

void GameControllerMIDIUI::renderButtonEditor(int buttonIndex, MapperConfig::ButtonConfig& config) {
    ImGui::Text("Editing: %s", getButtonName(buttonIndex));
    ImGui::Separator();

    const char* modes[] = {"None", "Note", "Chord", "CC Momentary", "CC Toggle"};
    int modeIndex = static_cast<int>(config.mode);
    if (modeIndex >= 5) {
        modeIndex = 0;  // Clamp deprecated modes
    }

    if (ImGui::Combo("Mode", &modeIndex, modes, 5)) {
        config.mode = static_cast<MapperConfig::ButtonMode>(modeIndex);
    }

    switch (config.mode) {
        case MapperConfig::ButtonMode::Note: {
            int note = config.noteOrCC;
            if (ImGui::SliderInt("MIDI Note", &note, 0, 127)) {
                config.noteOrCC = static_cast<uint8_t>(note);
            }
            int velocity = config.velocity;
            if (ImGui::SliderInt("Velocity", &velocity, 0, 127)) {
                config.velocity = static_cast<uint8_t>(velocity);
            }
            break;
        }
        case MapperConfig::ButtonMode::Chord: {
            int note = config.noteOrCC;
            if (ImGui::SliderInt("Root Note", &note, 0, 127)) {
                config.noteOrCC = static_cast<uint8_t>(note);
            }
            int velocity = config.velocity;
            if (ImGui::SliderInt("Velocity", &velocity, 0, 127)) {
                config.velocity = static_cast<uint8_t>(velocity);
            }
            int chordOct = config.chordOctaveOffset;
            if (ImGui::SliderInt("Chord Octave Offset", &chordOct, -4, 4)) {
                config.chordOctaveOffset = static_cast<int8_t>(chordOct);
            }
            ImGui::Separator();
            renderChordIntervalEditor(config);
            break;
        }
        case MapperConfig::ButtonMode::CC_Momentary:
        case MapperConfig::ButtonMode::CC_Toggle: {
            int cc = config.noteOrCC;
            if (ImGui::SliderInt("CC Number", &cc, 0, 127)) {
                config.noteOrCC = static_cast<uint8_t>(cc);
            }
            break;
        }
        default:
            break;
    }

    ImGui::Separator();
    ImGui::Text("Shift Key Override:");
    bool useGlobal = (config.shiftButton < 0);
    if (ImGui::Checkbox("Use Global Shift", &useGlobal)) {
        config.shiftButton = useGlobal ? -1 : static_cast<int8_t>(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    }
    if (!useGlobal) {
        if (ImGui::BeginCombo("Shift Button", getButtonName(config.shiftButton))) {
            for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
                if (ImGui::Selectable(getButtonName(i), i == config.shiftButton)) {
                    config.shiftButton = static_cast<int8_t>(i);
                }
            }
            ImGui::EndCombo();
        }
    }
}

void GameControllerMIDIUI::renderChordIntervalEditor(MapperConfig::ButtonConfig& config) {
    ImGui::Text("Chord Intervals:");
    for (size_t i = 0; i < config.intervalCount; ++i) {
        ImGui::PushID(static_cast<int>(i));
        int interval = config.chordIntervals[i];
        ImGui::SetNextItemWidth(100);
        if (ImGui::SliderInt("##int", &interval, -24, 24)) {
            config.chordIntervals[i] = static_cast<int8_t>(interval);
        }
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            for (size_t j = i; j < config.intervalCount - 1; ++j) {
                config.chordIntervals[j] = config.chordIntervals[j + 1];
            }
            config.intervalCount--;
        }
        ImGui::PopID();
    }
    if (config.intervalCount < MapperConfig::MAX_CHORD_INTERVALS) {
        if (ImGui::Button("Add Interval")) {
            config.chordIntervals[config.intervalCount++] = 0;
        }
    }

    ImGui::Text("Quick Chord:");
    if (ImGui::Button("Maj")) {
        config.intervalCount = 3;
        config.chordIntervals = {0, 4, 7};
    }
    ImGui::SameLine();
    if (ImGui::Button("Min")) {
        config.intervalCount = 3;
        config.chordIntervals = {0, 3, 7};
    }
    ImGui::SameLine();
    if (ImGui::Button("Dim")) {
        config.intervalCount = 3;
        config.chordIntervals = {0, 3, 6};
    }
    ImGui::SameLine();
    if (ImGui::Button("7th")) {
        config.intervalCount = 4;
        config.chordIntervals = {0, 4, 7, 10};
    }
}

void GameControllerMIDIUI::renderAxisListEditor(std::array<MapperConfig::AxisConfig, SDL_CONTROLLER_AXIS_MAX>& axes, bool isShift) {
    ImGui::BeginChild("AxisList", ImVec2(150, 0), true);
    for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
        bool selected = (fSelectedAxis == i && fEditingShiftButton == isShift);
        if (ImGui::Selectable(getAxisName(i), selected)) {
            fSelectedAxis = i;
            fEditingShiftButton = isShift;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("AxisEditor", ImVec2(0, 0), true);
    if (fSelectedAxis >= 0 && fEditingShiftButton == isShift) {
        renderAxisEditor(fSelectedAxis, axes[fSelectedAxis]);
    }
    else {
        ImGui::TextDisabled("Select an axis to edit");
    }
    ImGui::EndChild();
}

void GameControllerMIDIUI::renderAxisEditor(int axisIndex, MapperConfig::AxisConfig& config) {
    ImGui::Text("Editing Axis: %s", getAxisName(axisIndex));
    ImGui::Separator();

    const char* modes[] = {"None", "CC", "PitchBend", "Aftertouch"};
    int modeIndex = static_cast<int>(config.mode);
    if (ImGui::Combo("Mode", &modeIndex, modes, 4)) {
        config.mode = static_cast<MapperConfig::AxisMode>(modeIndex);
    }

    if (config.mode == MapperConfig::AxisMode::CC) {
        int cc = config.ccNumber;
        if (ImGui::SliderInt("CC Number", &cc, 0, 127)) {
            config.ccNumber = static_cast<uint8_t>(cc);
        }
    }

    if (config.mode != MapperConfig::AxisMode::None) {
        ImGui::Checkbox("Bipolar (Stick)", &config.isBipolar);
        ImGui::Checkbox("Inverted", &config.isInverted);
        ImGui::SliderFloat("Deadzone", &config.deadzone, 0.0f, 0.5f);
    }
}

void GameControllerMIDIUI::loadDraftFromActive(GameControllerMIDIPlugin* plugin) {
    std::lock_guard<std::mutex> lock(plugin->fConfigMutex);
    fDraftConfig = plugin->fActiveConfig;
}

void GameControllerMIDIUI::applyDraftToActive(GameControllerMIDIPlugin* plugin) {
    {
        std::lock_guard<std::mutex> lock(plugin->fConfigMutex);
        plugin->fActiveConfig = fDraftConfig;
    }

    // Reload outside lock to avoid deadlock
    plugin->reloadMapper();

    // Persist to host outside lock
    std::string json = MapperConfig::serializePreset(fDraftConfig);
    setState("config", json.c_str());
}

END_NAMESPACE_DISTRHO
