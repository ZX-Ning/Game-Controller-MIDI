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

    std::mutex fMutex;
    std::thread fThread;
    std::atomic<bool> fRunning;
    SDL_GameController* fController;
    IControllerEventHandler* fHandler;
};

}  // namespace GCMidi

#endif  // SDL_MANAGER_HPP_INCLUDED
