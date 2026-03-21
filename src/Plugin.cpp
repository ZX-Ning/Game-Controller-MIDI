#include "Plugin.hpp"

#include <algorithm>
#include <cstring>
#include <mutex>

START_NAMESPACE_DISTRHO

// -------------------------------------------------------------------------------------------------------
// Global SDL Manager to handle multi-instance event polling

class SdlManager {
public:
    static SdlManager& getInstance() {
        static SdlManager instance;
        return instance;
    }

    void registerPlugin(GameControllerMIDIPlugin* plugin) {
        std::lock_guard<std::mutex> lock(fMutex);
        if (std::find(fPlugins.begin(), fPlugins.end(), plugin) == fPlugins.end()) {
            fPlugins.push_back(plugin);
            if (!fRunning) {
                init();
            }
        }
    }

    void unregisterPlugin(GameControllerMIDIPlugin* plugin) {
        std::lock_guard<std::mutex> lock(fMutex);
        auto it = std::find(fPlugins.begin(), fPlugins.end(), plugin);
        if (it != fPlugins.end()) {
            fPlugins.erase(it);
            if (fPlugins.empty()) {
                cleanup();
            }
        }
    }

private:
    SdlManager() : fRunning(false), fController(nullptr) {}
    ~SdlManager() {
        cleanup();
    }

    void init() {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
            d_stderr2("SDL_Init failed: %s", SDL_GetError());
            return;
        }
        fRunning = true;
        fThread = std::thread(&SdlManager::loop, this);
    }

    void cleanup() {
        fRunning = false;
        if (fThread.joinable()) {
            fThread.join();
        }
        if (fController) {
            SDL_GameControllerClose(fController);
            fController = nullptr;
        }
        if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
            SDL_Quit();
        }
    }

    void loop() {
        while (fRunning) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                std::lock_guard<std::mutex> lock(fMutex);

                // Handle global controller management
                if (event.type == SDL_CONTROLLERDEVICEADDED) {
                    if (!fController) {
                        fController = SDL_GameControllerOpen(event.cdevice.which);
                    }
                }
                else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                    if (fController) {
                        SDL_JoystickID instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(fController));
                        if (instanceId == event.cdevice.which) {
                            SDL_GameControllerClose(fController);
                            fController = nullptr;
                        }
                    }
                }

                // Broadcast events to all registered plugins
                for (auto* p : fPlugins) {
                    p->handleSdlEvent(event);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::vector<GameControllerMIDIPlugin*> fPlugins;
    std::mutex fMutex;
    std::thread fThread;
    std::atomic<bool> fRunning;
    SDL_GameController* fController;
};

// -------------------------------------------------------------------------------------------------------

GameControllerMIDIPlugin::GameControllerMIDIPlugin()
    : Plugin(kParamCount, 0, 1),
      fMidiHistoryIndex(0),
      fControllerConnected(false),
      fController(nullptr),
      fMidiQueue(256) {
    std::memset(fControllerName, 0, sizeof(fControllerName));
    std::memset(fButtonStates, 0, sizeof(fButtonStates));
    std::memset(fActiveNotes, 0, sizeof(fActiveNotes));

    fTestState = "hello";
    fMidiHistory.resize(kMidiHistorySize);

    SdlManager::getInstance().registerPlugin(this);
    checkControllerStatus();
}

GameControllerMIDIPlugin::~GameControllerMIDIPlugin() {
    SdlManager::getInstance().unregisterPlugin(this);
}

void GameControllerMIDIPlugin::checkControllerStatus() {
    if (fControllerConnected) {
        return;
    }

    // Find any open controller
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* ctrl = SDL_GameControllerOpen(i);
            if (ctrl) {
                const char* name = SDL_GameControllerName(ctrl);
                if (name != nullptr) {
                    fControllerConnected = true;
                    std::strncpy(fControllerName, name, sizeof(fControllerName) - 1);
                    fControllerName[sizeof(fControllerName) - 1] = '\0';
                }
                SDL_GameControllerClose(ctrl);  // Just to get info
                if (fControllerConnected) {
                    break;
                }
            }
        }
    }
}

void GameControllerMIDIPlugin::handleSdlEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            checkControllerStatus();
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            // This is a bit tricky since we don't know WHICH one was removed easily without tracking handles
            // Let's just re-verify if any controller is still connected
            {
                bool any = false;
                for (int i = 0; i < SDL_NumJoysticks(); ++i) {
                    if (SDL_IsGameController(i)) {
                        any = true;
                        break;
                    }
                }
                if (!any) {
                    fControllerConnected = false;
                    std::memset(fControllerName, 0, sizeof(fControllerName));
                }
            }
            break;

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
            uint8_t button = event.cbutton.button;

            if (button < SDL_CONTROLLER_BUTTON_MAX) {
                fButtonStates[button] = down;

                // Shift is RB
                bool shift = fButtonStates[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER];

                uint8_t note = 0;
                switch (button) {
                    case SDL_CONTROLLER_BUTTON_X:
                        note = shift ? 67 : 60;
                        break;  // G4 / C4
                    case SDL_CONTROLLER_BUTTON_Y:
                        note = shift ? 69 : 62;
                        break;  // A4 / D4
                    case SDL_CONTROLLER_BUTTON_B:
                        note = shift ? 71 : 64;
                        break;  // B4 / E4
                    case SDL_CONTROLLER_BUTTON_A:
                        note = shift ? 72 : 65;
                        break;  // C5 / F4
                }

                if (note > 0) {
                    if (down) {
                        fActiveNotes[button] = note;
                        pushMidiEvent(0x90, note, 100);  // Note On
                    }
                    else {
                        if (fActiveNotes[button] > 0) {
                            pushMidiEvent(0x80, fActiveNotes[button], 0);  // Note Off
                            fActiveNotes[button] = 0;
                        }
                    }
                }
            }
            break;
        }
    }
}

void GameControllerMIDIPlugin::pushMidiEvent(uint8_t status, uint8_t data1, uint8_t data2) {
    RawMidi ev;
    ev.data[0] = status;
    ev.data[1] = data1;
    ev.data[2] = data2;
    ev.size = 3;
    ev.frame = 0;
    fMidiQueue.push(ev);
}

void GameControllerMIDIPlugin::initState(uint32_t index, State& state) {
    if (index != 0) {
        return;
    }
    state.hints = kStateIsHostReadable | kStateIsHostWritable;
    state.key = "test-state";
    state.defaultValue = "hello";
}

String GameControllerMIDIPlugin::getState(const char* key) const {
    if (std::strcmp(key, "test-state") == 0) {
        return fTestState;
    }
    return Plugin::getState(key);
}

void GameControllerMIDIPlugin::setState(const char* key, const char* value) {
    if (std::strcmp(key, "test-state") == 0) {
        fTestState = value;
    }
}

void GameControllerMIDIPlugin::run(const float**, float**, uint32_t, const MidiEvent* midiEvents, uint32_t midiEventCount) {
    for (uint32_t i = 0; i < midiEventCount; ++i) {
        if (midiEvents[i].size <= 4) {
            uint32_t historyIdx = fMidiHistoryIndex.load(std::memory_order_relaxed);
            historyIdx = (historyIdx + 1) % kMidiHistorySize;
            fMidiHistory[historyIdx].size = midiEvents[i].size;
            std::memcpy(fMidiHistory[historyIdx].data, midiEvents[i].data, midiEvents[i].size);
            fMidiHistoryIndex.store(historyIdx, std::memory_order_relaxed);
        }
        writeMidiEvent(midiEvents[i]);
    }

    // Drain our internal queue
    RawMidi rawEv;
    while (fMidiQueue.pop(rawEv)) {
        MidiEvent ev;
        ev.frame = 0;
        ev.size = rawEv.size;
        ev.dataExt = nullptr;
        std::memcpy(ev.data, rawEv.data, ev.size);

        uint32_t historyIdx = fMidiHistoryIndex.load(std::memory_order_relaxed);
        historyIdx = (historyIdx + 1) % kMidiHistorySize;
        fMidiHistory[historyIdx].size = ev.size;
        std::memcpy(fMidiHistory[historyIdx].data, ev.data, ev.size);
        fMidiHistoryIndex.store(historyIdx, std::memory_order_relaxed);

        writeMidiEvent(ev);
    }
}

Plugin* createPlugin() {
    return new GameControllerMIDIPlugin();
}

END_NAMESPACE_DISTRHO
