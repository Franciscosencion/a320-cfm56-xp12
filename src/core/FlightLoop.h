#pragma once
#include <XPLM/XPLMProcessing.h>
#include <functional>
#include <vector>
#include <chrono>

namespace a320 {

// Manages two flight loop callbacks:
//   - Pre-flight-model (50 Hz fixed): all systems logic
//   - Post-flight-model (per-frame):  FBW control surface writes
class FlightLoop {
public:
    static constexpr float SYSTEMS_HZ = 50.f;

    using SystemTick = std::function<void(float dt)>;

    void registerCallbacks();
    void unregisterCallbacks();

    // Systems register their tick function here during init.
    // Called in registration order every 1/50 s on the pre-FM callback.
    void addSystem(SystemTick tick);

    // Post-FM tick (FBW, engine overrides) — called every rendered frame.
    void addPostFMTick(SystemTick tick);

private:
    static float cbPreFM (float elapsedSinceLast, float elapsedFlightLoop,
                          int counter, void* refcon);
    static float cbPostFM(float elapsedSinceLast, float elapsedFlightLoop,
                          int counter, void* refcon);

    XPLMFlightLoopID m_preFMLoop  = nullptr;
    XPLMFlightLoopID m_postFMLoop = nullptr;

    std::vector<SystemTick> m_systems;
    std::vector<SystemTick> m_postFMSystems;
};

} // namespace a320
