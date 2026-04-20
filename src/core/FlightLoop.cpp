#include "FlightLoop.h"
#include "Logger.h"

namespace a320 {

void FlightLoop::registerCallbacks()
{
    XPLMCreateFlightLoop_t params{};
    params.structSize = sizeof(params);
    params.refcon     = this;

    // Pre-flight-model at fixed 50 Hz — all systems simulation
    params.phase        = xplm_FlightLoop_Phase_BeforeFlightModel;
    params.callbackFunc = cbPreFM;
    m_preFMLoop = XPLMCreateFlightLoop(&params);
    XPLMScheduleFlightLoop(m_preFMLoop, -SYSTEMS_HZ, 1);

    // Post-flight-model every rendered frame — FBW + engine overrides
    params.phase        = xplm_FlightLoop_Phase_AfterFlightModel;
    params.callbackFunc = cbPostFM;
    m_postFMLoop = XPLMCreateFlightLoop(&params);
    XPLMScheduleFlightLoop(m_postFMLoop, -1.f, 1);  // -1 = every frame

    LOG_INFO("Flight loops registered ({} Hz systems, per-frame post-FM)", SYSTEMS_HZ);
}

void FlightLoop::unregisterCallbacks()
{
    if (m_preFMLoop)  { XPLMDestroyFlightLoop(m_preFMLoop);  m_preFMLoop  = nullptr; }
    if (m_postFMLoop) { XPLMDestroyFlightLoop(m_postFMLoop); m_postFMLoop = nullptr; }
}

void FlightLoop::addSystem(SystemTick tick)     { m_systems.push_back(std::move(tick)); }
void FlightLoop::addPostFMTick(SystemTick tick) { m_postFMSystems.push_back(std::move(tick)); }

float FlightLoop::cbPreFM(float elapsedSinceLast, float /*loopElapsed*/,
                           int /*counter*/, void* refcon)
{
    auto* self = static_cast<FlightLoop*>(refcon);
    const float dt = elapsedSinceLast;
    for (auto& fn : self->m_systems) fn(dt);
    return 1.f / SYSTEMS_HZ;  // reschedule at fixed rate
}

float FlightLoop::cbPostFM(float elapsedSinceLast, float /*loopElapsed*/,
                            int /*counter*/, void* refcon)
{
    auto* self = static_cast<FlightLoop*>(refcon);
    const float dt = elapsedSinceLast;
    for (auto& fn : self->m_postFMSystems) fn(dt);
    return -1.f;  // reschedule every frame
}

} // namespace a320
