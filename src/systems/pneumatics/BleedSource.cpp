#include "BleedSource.h"

namespace a320 {

BleedSource::BleedSource(int index) : m_index(index) {}

void BleedSource::update(float /*dt*/)
{
    if (!m_switchOn || m_fault) {
        m_flowAvail   = false;
        m_pressurePsi = 0.f;
        m_tempC       = 20.f;
        return;
    }

    // Bleed valve opens when upstream pressure is sufficient (N2 >= ~15%).
    // BMC selects IP or HP stage internally; manifold always sees regulated 45 PSI.
    if (m_n2Pct >= MIN_N2) {
        m_flowAvail   = true;
        m_pressurePsi = DELIVERY_PRESS_PSI;
        m_tempC       = BLEED_TEMP_C;
    } else {
        m_flowAvail   = false;
        m_pressurePsi = 0.f;
        m_tempC       = 20.f;
    }
}

void BleedSource::setSwitch(bool on)    { m_switchOn = on; }
void BleedSource::setN2(float n2)       { m_n2Pct = n2; }
void BleedSource::setFault(bool fault)  { m_fault = fault; }

} // namespace a320
