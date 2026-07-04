#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>

namespace GCMidi {

/** Immutable octave snapshot used for one mapper operation. */
struct MapperOctaveState {
    int8_t base = 0;
    int8_t trigger = 0;
};

/**
 * Shared, UI/host-visible state owned by EventDispatcher.
 *
 * Mapper code may use this during an event callback, but must not store a
 * reference to it. Base octave changes initiated by mapper/controller paths set
 * a host-sync flag so Plugin::run() can notify the host parameter system.
 */
class SharedState {
public:
    MapperOctaveState octaveSnapshot() const {
        return {
            fBaseOctave.load(std::memory_order_relaxed),
            fTriggerOctave.load(std::memory_order_relaxed)
        };
    }

    int8_t baseOctave() const {
        return fBaseOctave.load(std::memory_order_relaxed);
    }

    int8_t triggerOctave() const {
        return fTriggerOctave.load(std::memory_order_relaxed);
    }

    void setBaseOctaveFromHost(int value) {
        fBaseOctave.store(clampOctave(value), std::memory_order_relaxed);
    }

    void setBaseOctaveFromPreset(int value) {
        const int8_t clamped = clampOctave(value);
        const int8_t oldValue = fBaseOctave.exchange(clamped, std::memory_order_relaxed);
        if (oldValue != clamped) {
            fBaseOctaveHostSyncPending.store(true, std::memory_order_relaxed);
        }
    }

    void adjustBaseOctaveFromMapper(int delta) {
        int8_t current = fBaseOctave.load(std::memory_order_relaxed);
        while (true) {
            const int8_t next = clampOctave(static_cast<int>(current) + delta);
            if (next == current) {
                return;
            }
            if (fBaseOctave.compare_exchange_weak(current, next, std::memory_order_relaxed, std::memory_order_relaxed)) {
                fBaseOctaveHostSyncPending.store(true, std::memory_order_relaxed);
                return;
            }
        }
    }

    void setTriggerOctave(int value) {
        fTriggerOctave.store(clampOctave(value), std::memory_order_relaxed);
    }

    void adjustTriggerOctave(int delta) {
        int8_t current = fTriggerOctave.load(std::memory_order_relaxed);
        while (true) {
            const int8_t next = clampOctave(static_cast<int>(current) + delta);
            if (next == current) {
                return;
            }
            if (fTriggerOctave.compare_exchange_weak(current, next, std::memory_order_relaxed, std::memory_order_relaxed)) {
                return;
            }
        }
    }

    void resetTriggerOctave() {
        setTriggerOctave(0);
    }

    bool consumeBaseOctaveHostSyncPending() {
        return fBaseOctaveHostSyncPending.exchange(false, std::memory_order_relaxed);
    }

private:
    static int8_t clampOctave(int value) {
        return static_cast<int8_t>(std::clamp(value, -4, 4));
    }

    std::atomic<int8_t> fBaseOctave{0};
    std::atomic<int8_t> fTriggerOctave{0};
    std::atomic<bool> fBaseOctaveHostSyncPending{false};
};

}  // namespace GCMidi
