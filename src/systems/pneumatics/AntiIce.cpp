#include "AntiIce.h"

namespace a320 {

void AntiIce::update(float /*dt*/)
{
    // WAI active when switch on AND at least one manifold has pressure
    m_waiActive  = m_waiSwitch  && (m_manifold1Avail || m_manifold2Avail);

    // EAI active when switch on AND respective manifold has pressure
    m_eai1Active = m_eai1Switch && m_manifold1Avail;
    m_eai2Active = m_eai2Switch && m_manifold2Avail;
}

void AntiIce::setWingAntiIce(bool on)      { m_waiSwitch  = on; }
void AntiIce::setEng1AntiIce(bool on)      { m_eai1Switch = on; }
void AntiIce::setEng2AntiIce(bool on)      { m_eai2Switch = on; }
void AntiIce::setManifold1Avail(bool avail){ m_manifold1Avail = avail; }
void AntiIce::setManifold2Avail(bool avail){ m_manifold2Avail = avail; }

} // namespace a320
