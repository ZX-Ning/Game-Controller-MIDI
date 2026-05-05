#ifndef SDL_MANAGER_HPP_INCLUDED
#define SDL_MANAGER_HPP_INCLUDED

#include <SDL.h>

#include <array>
#include <atomic>
#include <mutex>
#include <thread>

#include "IControllerEventHandler.hpp"

namespace GCMidi {

inline constexpr size_t MAX_SDL_EVENT_HANDLERS = 8;

// Global SDL Manager to handle lifecycle and event polling
class SdlManager {
public:
    static SdlManager& getInstance();

    // Set the event handler. Pass nullptr to unregister.
    void setEventHandler(IControllerEventHandler* handler);
    void clearEventHandler(IControllerEventHandler* handler);

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
    bool hasHandlersLocked() const;
    std::array<IControllerEventHandler*, MAX_SDL_EVENT_HANDLERS> getHandlerSnapshotLocked() const;

    std::mutex fMutex;
    std::thread fThread;
    std::atomic<bool> fRunning;
    SDL_GameController* fController;
    std::array<IControllerEventHandler*, MAX_SDL_EVENT_HANDLERS> fHandlers{};
    bool fSdlInitialized = false;
};

}  // namespace GCMidi

#endif  // SDL_MANAGER_HPP_INCLUDED
