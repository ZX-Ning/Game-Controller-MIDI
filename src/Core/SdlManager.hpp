#ifndef SDL_MANAGER_HPP_INCLUDED
#define SDL_MANAGER_HPP_INCLUDED

#include <SDL.h>

#include <atomic>
#include <mutex>
#include <thread>

#include "IControllerEventHandler.hpp"

namespace GCMidi {

// Global SDL Manager to handle lifecycle and event polling
class SdlManager {
public:
    static SdlManager& getInstance();

    // Set the event handler. Pass nullptr to unregister.
    void setEventHandler(IControllerEventHandler* handler);

private:
    SdlManager();
    ~SdlManager();

    void init();
    void cleanup();
    void loop();
    void checkControllerStatus();

    void handleEvent(const SDL_Event& event);
    void handleControllerRemoved(const SDL_ControllerDeviceEvent& event);
    void handleControllerButton(const SDL_ControllerButtonEvent& event);
    void handleControllerAxis(const SDL_ControllerAxisEvent& event);

    std::mutex fMutex;
    std::thread fThread;
    std::atomic<bool> fRunning;
    SDL_GameController* fController;
    IControllerEventHandler* fHandler;
};

}  // namespace GCMidi

#endif  // SDL_MANAGER_HPP_INCLUDED
