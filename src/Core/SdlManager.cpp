#include "SdlManager.hpp"

#include <chrono>

#include "DistrhoUtils.hpp"

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
            std::lock_guard<std::mutex> lock(fMutex);

            switch (event.type) {
                case SDL_CONTROLLERDEVICEADDED:
                    checkControllerStatus();
                    break;

                case SDL_CONTROLLERDEVICEREMOVED:
                    if (fController) {
                        SDL_JoystickID instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(fController));
                        if (instanceId == (SDL_JoystickID)event.cdevice.which) {
                            SDL_GameControllerClose(fController);
                            fController = nullptr;
                            if (fHandler) {
                                fHandler->onControllerDisconnected();
                            }
                        }
                    }
                    break;

                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP: {
                    if (fHandler && fController) {
                        bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
                        uint8_t button = event.cbutton.button;
                        bool shift = SDL_GameControllerGetButton(fController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) == 1;
                        fHandler->onControllerButton(button, down, shift);
                    }
                    break;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

}  // namespace GCMidi
