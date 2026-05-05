#include "SdlManager.hpp"

#include <chrono>
#include <cstdio>

namespace GCMidi {

SdlManager& SdlManager::getInstance() {
    static SdlManager instance;
    return instance;
}

SdlManager::SdlManager()
    : fRunning(false), fController(nullptr) {
}

SdlManager::~SdlManager() {
    cleanup();
}

void SdlManager::setEventHandler(IControllerEventHandler* handler) {
    bool doInit = false;
    bool registered = false;

    {
        std::lock_guard<std::mutex> lock(fMutex);
        if (!handler) {
            return;
        }

        for (auto* existing : fHandlers) {
            if (existing == handler) {
                registered = true;
                break;
            }
        }

        if (!registered) {
            for (auto& slot : fHandlers) {
                if (!slot) {
                    slot = handler;
                    registered = true;
                    break;
                }
            }
        }

        if (!registered) {
            fprintf(stderr, "SdlManager handler registry full; ignoring plugin instance\n");
            return;
        }

        if (!fRunning) {
            doInit = true;
        }
    }

    if (doInit) {
        init();
        std::lock_guard<std::mutex> lock(fMutex);
        if (hasHandlersLocked()) {
            checkControllerStatus();
        }
    }
    else {
        std::lock_guard<std::mutex> lock(fMutex);
        checkControllerStatus();
    }
}

void SdlManager::clearEventHandler(IControllerEventHandler* handler) {
    bool doCleanup = false;

    {
        std::lock_guard<std::mutex> lock(fMutex);
        for (auto& slot : fHandlers) {
            if (slot == handler) {
                slot = nullptr;
                break;
            }
        }

        doCleanup = fRunning && !hasHandlersLocked();
    }

    if (doCleanup) {
        cleanup();
    }
}

void SdlManager::init() {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        // Fallback to stderr if DistrhoUtils is not available or has different namespace
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    fSdlInitialized = true;
    fRunning = true;
    fThread = std::thread(&SdlManager::loop, this);
}

void SdlManager::cleanup() {
    fRunning = false;
    if (fThread.joinable() && std::this_thread::get_id() != fThread.get_id()) {
        fThread.join();
    }
    if (fController) {
        SDL_GameControllerClose(fController);
        fController = nullptr;
    }
    if (fSdlInitialized && SDL_WasInit(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK)) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
        fSdlInitialized = false;
    }
}

void SdlManager::checkControllerStatus() {
    if (fController) {
        auto handlers = getHandlerSnapshotLocked();
        for (auto* handler : handlers) {
            if (handler) {
                handler->onControllerConnected(SDL_GameControllerName(fController));
            }
        }
        return;
    }

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* ctrl = SDL_GameControllerOpen(i);
            if (ctrl) {
                fController = ctrl;
                const char* name = SDL_GameControllerName(ctrl);
                auto handlers = getHandlerSnapshotLocked();
                for (auto* handler : handlers) {
                    if (handler && name) {
                        handler->onControllerConnected(name);
                    }
                }
                break;  // Only open the first one
            }
        }
    }
}

void SdlManager::loop() {
    while (fRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void SdlManager::handleEvent(const SDL_Event& event) {
    std::lock_guard<std::mutex> lock(fMutex);

    switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            checkControllerStatus();
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            handleControllerRemoved(event.cdevice);
            break;

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            handleControllerButton(event.cbutton);
            break;

        case SDL_CONTROLLERAXISMOTION:
            handleControllerAxis(event.caxis);
            break;
    }
}

void SdlManager::handleControllerRemoved(const SDL_ControllerDeviceEvent& event) {
    if (!fController) {
        return;
    }

    SDL_JoystickID instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(fController));
    if (instanceId == (SDL_JoystickID)event.which) {
        SDL_GameControllerClose(fController);
        fController = nullptr;
        auto handlers = getHandlerSnapshotLocked();
        for (auto* handler : handlers) {
            if (handler) {
                handler->onControllerDisconnected();
            }
        }
    }
}

void SdlManager::handleControllerButton(const SDL_ControllerButtonEvent& event) {
    if (!fController) {
        return;
    }

    bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
    uint8_t button = event.button;
    auto handlers = getHandlerSnapshotLocked();
    for (auto* handler : handlers) {
        if (!handler) {
            continue;
        }

        uint8_t shiftBtn = handler->getShiftButtonForButton(button);
        bool shift = SDL_GameControllerGetButton(fController, static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
        handler->onControllerButton(button, down, shift);
    }
}

void SdlManager::handleControllerAxis(const SDL_ControllerAxisEvent& event) {
    if (!fController) {
        return;
    }

    uint8_t axis = event.axis;
    int16_t value = event.value;
    auto handlers = getHandlerSnapshotLocked();
    for (auto* handler : handlers) {
        if (!handler) {
            continue;
        }

        uint8_t shiftBtn = handler->getShiftButton();
        bool shift = SDL_GameControllerGetButton(fController, static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
        handler->onControllerAxis(axis, value, shift);
    }
}

bool SdlManager::hasHandlersLocked() const {
    for (auto* handler : fHandlers) {
        if (handler) {
            return true;
        }
    }
    return false;
}

std::array<IControllerEventHandler*, MAX_SDL_EVENT_HANDLERS> SdlManager::getHandlerSnapshotLocked() const {
    return fHandlers;
}

}  // namespace GCMidi
