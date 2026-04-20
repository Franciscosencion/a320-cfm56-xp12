#pragma once

namespace a320 {

// Models one 24 V NiCd battery (BAT 1 or BAT 2).
// Capacity: 23 Ah. Emergency supply ~30 min at minimum load.
class Battery {
public:
    explicit Battery(int index);

    void update(float dt);

    // Controls
    void setSwitch(bool on);  // BAT pb AUTO(on) / OFF

    // Synchronise contactor to switch state without running charge integration.
    // Call this before DC network resolution so the contactor is current.
    void syncContactorState();

    // Charging: called by ElectricalSystem when DC BUS is powered
    void setCharging(bool charging, float busVoltage = 28.f);

    // State
    bool  isContactorClosed() const { return m_contactorClosed; }
    bool  isCharging()        const { return m_chargingActive; }
    float voltage()           const { return m_voltage; }
    float chargePct()         const { return m_chargePct; }
    int   index()             const { return m_index; }

private:
    int   m_index;
    bool  m_switchOn         = true;  // AUTO by default
    bool  m_chargingActive   = false;
    float m_busVoltage        = 0.f;

    float m_chargePct         = 100.f; // 0–100
    float m_voltage           = 25.5f; // fully charged NiCd open-circuit

    bool  m_contactorClosed   = true;   // matches default m_switchOn=true

    // Discharge: ~23 Ah at ~5 A average emergency load → ~4.6 h; 30 min at ~46 A
    static constexpr float CAPACITY_AH     = 23.f;
    static constexpr float NOMINAL_V       = 24.f;
    static constexpr float FULL_CHARGE_V   = 25.5f;
    static constexpr float DISCHARGED_V    = 20.f;
    static constexpr float EMERGENCY_LOAD_A = 46.f;  // approximate heavy load

    void updateVoltage();
};

} // namespace a320
