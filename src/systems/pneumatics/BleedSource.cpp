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

    if (m_n2Pct >= HP_MIN_N2) {
        m_flowAvail   = true;
        m_pressurePsi = HP_PRESS;
        m_tempC       = HP_TEMP_C;
    } else if (m_n2Pct >= IP_MIN_N2) {
        m_flowAvail   = true;
        m_pressurePsi = IP_PRESS;
        m_tempC       = IP_TEMP_C;
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
