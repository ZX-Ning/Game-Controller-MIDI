#include "SdlManager.hpp"

#include <chrono>

namespace GCMidi {

SdlManager& SdlManager::getInstance() {
    static SdlManager instance;
    return instance;
}

SdlManager::SdlManager()
    : fRunning(false), fController(nullptr), fHandler(nullptr) {
}

SdlManager::~SdlManager() {
    cleanup();
}

void SdlManager::setEventHandler(IControllerEventHandler* handler) {
    bool doInit = false;
    bool doCleanup = false;

    {
        std::lock_guard<std::mutex> lock(fMutex);
        fHandler = handler;

        if (handler && !fRunning) {
            doInit = true;
        }
        else if (!handler && fRunning) {
            doCleanup = true;
        }
    }

    if (doInit) {
        init();
        std::lock_guard<std::mutex> lock(fMutex);
        if (fHandler) {
            checkControllerStatus();
        }
    }
    else if (doCleanup) {
        cleanup();
    }
    else if (handler) {
        std::lock_guard<std::mutex> lock(fMutex);
        checkControllerStatus();
    }
}

void SdlManager::init() {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        // Fallback to stderr if DistrhoUtils is not available or has different namespace
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
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
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_Quit();
    }
}

void SdlManager::checkControllerStatus() {
    if (fController) {
        if (fHandler) {
            fHandler->onControllerConnected(SDL_GameControllerName(fController));
        }
        return;
    }

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* ctrl = SDL_GameControllerOpen(i);
            if (ctrl) {
                fController = ctrl;
                const char* name = SDL_GameControllerName(ctrl);
                if (fHandler && name) {
                    fHandler->onControllerConnected(name);
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
        if (fHandler) {
            fHandler->onControllerDisconnected();
        }
    }
}

void SdlManager::handleControllerButton(const SDL_ControllerButtonEvent& event) {
    if (!fHandler || !fController) {
        return;
    }

    bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
    uint8_t button = event.button;
    uint8_t shiftBtn = fHandler->getShiftButtonForButton(button);
    bool shift = SDL_GameControllerGetButton(fController, static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
    fHandler->onControllerButton(button, down, shift);
}

void SdlManager::handleControllerAxis(const SDL_ControllerAxisEvent& event) {
    if (!fHandler || !fController) {
        return;
    }

    uint8_t axis = event.axis;
    int16_t value = event.value;
    uint8_t shiftBtn = fHandler->getShiftButton();
    bool shift = SDL_GameControllerGetButton(fController, static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
    fHandler->onControllerAxis(axis, value, shift);
}

}  // namespace GCMidi
