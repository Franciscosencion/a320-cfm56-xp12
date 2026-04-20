#pragma once

namespace a320 {

// Models one A320 ECS pack (Air Conditioning Pack).
// Each pack receives bleed air, cools it through ram air heat exchangers
// and a turbine expansion cycle, then supplies conditioned air to the cabin.
class Pack {
public:
    explicit Pack(int index);

    void update(float dt);

    // Controls
    void setSwitch(bool norm);           // PACK pb: norm=operating, off=closed FCV
    void setHiFlow(bool hi);             // PACK FLOW HIGH/NORM selector

    // Inputs from pneumatic system each tick
    void setInletPressurePsi(float psi);
    void setInletTempC(float tempC);
    void setRamAirTempC(float tempC);    // OAT used as ram air approximation
    void setTargetTempC(float tempC);    // zone temperature demand

    // State
    bool  isOperating()     const { return m_operating; }
    float outletTempC()     const { return m_outletTempC; }
    float flowKgS()         const { return m_flowKgS; }
    bool  isFaulted()       const { return m_fault; }
    int   index()           const { return m_index; }

private:
    int   m_index;
    bool  m_switchNorm   = true;
    bool  m_hiFlow       = false;
    float m_inletPress   = 0.f;
    float m_inletTempC   = 180.f;
    float m_ramAirTempC  = -55.f;
    float m_targetTempC  = 24.f;
    bool  m_fault        = false;

    bool  m_operating    = false;
    float m_outletTempC  = 20.f;
    float m_flowKgS      = 0.f;

    static constexpr float MIN_INLET_PSI   = 15.f;  // minimum for FCV to open
    static constexpr float NORM_FLOW_KGS   = 0.35f; // kg/s per pack at NORM
    static constexpr float HI_FLOW_KGS     = 0.70f; // kg/s per pack at HIGH
    static constexpr float TEMP_RATE_C_S   = 2.f;   // °C/s outlet temp slew rate
};

} // namespace a320
