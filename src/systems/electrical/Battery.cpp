#include "Battery.h"
#include <algorithm>

namespace a320 {

Battery::Battery(int index) : m_index(index) {}

void Battery::update(float dt)
{
    if (!m_switchOn) {
        m_contactorClosed = false;
        m_chargingActive  = false;
        updateVoltage();
        return;
    }

    if (m_chargingActive && m_busVoltage > NOMINAL_V) {
        // Charge at ~0.1C rate (roughly 2.3 A for a 23 Ah battery)
        float chargeRateAh = (CAPACITY_AH * 0.1f) * dt / 3600.f;
        float chargeGain   = chargeRateAh / CAPACITY_AH * 100.f;
        m_chargePct = std::min(m_chargePct + chargeGain, 100.f);
    } else if (!m_chargingActive && m_contactorClosed) {
        // Discharging while powering bus
        float dischargeAh = EMERGENCY_LOAD_A * dt / 3600.f;
        float dischargeLoss = dischargeAh / CAPACITY_AH * 100.f;
        m_chargePct = std::max(m_chargePct - dischargeLoss, 0.f);
    }

    // Contactor: AUTO mode — close when switch on
    // Bus tie logic in ElectricalSystem handles priority; Battery just reports state.
    m_contactorClosed = m_switchOn;
    updateVoltage();
}

void Battery::setSwitch(bool on) { m_switchOn = on; }

void Battery::syncContactorState()
{
    m_contactorClosed = m_switchOn;
    if (!m_switchOn) m_chargingActive = false;
}

void Battery::setCharging(bool charging, float busVoltage)
{
    m_chargingActive = charging;
    m_busVoltage     = busVoltage;
}

void Battery::updateVoltage()
{
    // Linear interpolation between discharged and full-charge voltage
    float t = std::max(0.f, std::min(m_chargePct / 100.f, 1.f));
    m_voltage = DISCHARGED_V + t * (FULL_CHARGE_V - DISCHARGED_V);
}

} // namespace a320
