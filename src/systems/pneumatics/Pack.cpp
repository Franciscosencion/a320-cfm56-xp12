#include "Pack.h"
#include <algorithm>
#include <cmath>

namespace a320 {

Pack::Pack(int index) : m_index(index) {}

void Pack::update(float dt)
{
    m_operating = m_switchNorm && !m_fault && (m_inletPress >= MIN_INLET_PSI);

    if (!m_operating) {
        m_flowKgS = 0.f;
        return;
    }

    m_flowKgS = m_hiFlow ? HI_FLOW_KGS : NORM_FLOW_KGS;

    // Pack turbine expansion cools bleed air by ~155°C from inlet.
    // Mix valve then blends turbine exit air (~25°C below target) with hot bypass
    // to achieve the zone demand temperature.
    // Simplified: outlet converges to targetTempC at TEMP_RATE_C_S.
    float target = std::max(m_ramAirTempC + 5.f, m_targetTempC);
    target = std::min(target, m_inletTempC - 5.f);  // can't be hotter than inlet

    float diff = target - m_outletTempC;
    float step = TEMP_RATE_C_S * dt;
    if (std::abs(diff) <= step)
        m_outletTempC = target;
    else
        m_outletTempC += (diff > 0.f ? step : -step);
}

void Pack::setSwitch(bool norm)            { m_switchNorm  = norm; }
void Pack::setHiFlow(bool hi)              { m_hiFlow      = hi; }
void Pack::setInletPressurePsi(float psi)  { m_inletPress  = psi; }
void Pack::setInletTempC(float tempC)      { m_inletTempC  = tempC; }
void Pack::setRamAirTempC(float tempC)     { m_ramAirTempC = tempC; }
void Pack::setTargetTempC(float tempC)     { m_targetTempC = tempC; }

} // namespace a320
