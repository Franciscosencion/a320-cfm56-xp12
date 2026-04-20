#pragma once

namespace a320 {

// Models one engine bleed extraction point (BMC-controlled).
// FCOM DSC-36-10-20: IP stage is the normal source; HP stage supplements at low N2.
// The bleed valve downstream of the HP/IP junction regulates delivery to 45 ± 5 PSI.
// Bleed valve closes if upstream pressure < 8 PSI (≈ N2 below ~15%).
// The overhead BLEED pb controls the bleed shut-off valve.
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

    // N2 below this: upstream pressure < 8 PSI, bleed valve cannot open (FCOM DSC-36-10-20)
    static constexpr float MIN_N2     = 15.f;
    // Regulated delivery pressure at manifold (FCOM DSC-36-10-20: 45 ± 5 PSI)
    static constexpr float DELIVERY_PRESS_PSI = 45.f;
    static constexpr float BLEED_TEMP_C       = 200.f;  // typical precooled bleed temp
};

} // namespace a320
