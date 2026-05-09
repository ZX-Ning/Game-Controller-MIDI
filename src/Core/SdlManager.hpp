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

/**
 * Process-wide owner for SDL controller lifecycle and polling.
 *
 * The first registered plugin instance starts SDL and a background polling
 * thread; the last unregistering instance shuts both down. `fMutex` protects the
 * controller handle, handler registry, and SDL initialization flag. This class
 * is never used from the audio thread.
 */
class SdlManager {
public:
    /** Return the single process-wide SDL manager. */
    static SdlManager& getInstance();

    /** Register a handler and start SDL polling if this is the first one. */
    void setEventHandler(IControllerEventHandler* handler);

    /** Unregister a handler and stop SDL polling when no handlers remain. */
    void clearEventHandler(IControllerEventHandler* handler);

private:
    /** Construct an idle manager; SDL is initialized lazily. */
    SdlManager();

    /** Stop polling and release SDL resources. */
    ~SdlManager();

    /** Initialize SDL and start the polling thread. */
    void init();

    /** Stop polling, close the active controller, and quit SDL subsystems. */
    void cleanup();

    /** Poll SDL events on the background thread until `fRunning` is cleared. */
    void loop();

    /** Open the first controller or notify handlers about the current one. Requires `fMutex`. */
    void checkControllerStatus();

    /** Dispatch one SDL event. Locks `fMutex`. */
    void handleEvent(const SDL_Event& event);

    /** Close the active controller after removal. Requires `fMutex`. */
    void handleControllerRemoved(const SDL_ControllerDeviceEvent& event);

    /** Forward a button edge to handlers. Requires `fMutex`. */
    void handleControllerButton(const SDL_ControllerButtonEvent& event);

    /** Forward axis motion to handlers. Requires `fMutex`. */
    void handleControllerAxis(const SDL_ControllerAxisEvent& event);

    /** Return whether any handler slots are occupied. Requires `fMutex`. */
    bool hasHandlersLocked() const;

    /** Copy handler slots for iteration. Requires `fMutex`. */
    std::array<IControllerEventHandler*, MAX_SDL_EVENT_HANDLERS> getHandlerSnapshotLocked() const;

    /**
     * Protects `fController`, `fHandlers`, and `fSdlInitialized`. Handler
     * callbacks run while this is held, so callbacks must not call back into
     * `SdlManager`. Never lock it from the audio thread.
     */
    std::mutex fMutex;

    /** Background SDL polling thread, created by `init()` and joined by `cleanup()`. */
    std::thread fThread;

    /** Atomic run flag written by lifecycle code and read by the polling thread. */
    std::atomic<bool> fRunning;

    /** Current SDL controller, or nullptr. Protected by `fMutex`. */
    SDL_GameController* fController;

    /** Fixed-size handler registry, protected by `fMutex`. */
    std::array<IControllerEventHandler*, MAX_SDL_EVENT_HANDLERS> fHandlers{};

    /** Whether this manager initialized SDL subsystems. Protected by `fMutex`. */
    bool fSdlInitialized = false;
};

}  // namespace GCMidi

#endif  // SDL_MANAGER_HPP_INCLUDED
