#pragma once

namespace a320 {

// Models one engine HP/IP bleed extraction point.
// HP bleed: N2 > 68% → ~35 PSI (high pressure stage)
// IP bleed: N2 > 15% → ~18 PSI (intermediate stage, lower pressure)
// The overhead BLEED pb controls the high-pressure shut-off valve.
class BleedSource {
public:
    explicit BleedSource(int index);

    void update(float dt);

    // Controls
    void setSwitch(bool on);      // BLEED pb: on = AUTO (valve enabled)
    void setN2(float n2Pct);
    void setFault(bool fault);

    // State
    bool  isFlowAvailable()  const { return m_flowAvail; }
    float pressurePsi()      const { return m_pressurePsi; }
    float bleedTempC()       const { return m_tempC; }
    bool  isFaulted()        const { return m_fault; }
    bool  isSwitchOn()       const { return m_switchOn; }
    int   index()            const { return m_index; }

private:
    int   m_index;
    bool  m_switchOn    = true;
    float m_n2Pct       = 0.f;
    bool  m_fault       = false;

    bool  m_flowAvail   = false;
    float m_pressurePsi = 0.f;
    float m_tempC       = 20.f;

    static constexpr float HP_MIN_N2 = 68.f;
    static constexpr float IP_MIN_N2 = 15.f;
    static constexpr float HP_PRESS  = 35.f;   // PSI at HP stage
    static constexpr float IP_PRESS  = 18.f;   // PSI at IP stage
    static constexpr float HP_TEMP_C = 180.f;  // approx HP bleed temp
    static constexpr float IP_TEMP_C = 120.f;  // approx IP bleed temp
};

} // namespace a320
