#pragma once

namespace a320 {

// A320 Cabin Pressure Control System (CPCS).
//
// Auto mode schedule:
//   Ground:  cabin alt = field elevation
//   Climb:   cabin alt climbs at ≤300 FPM, capped at 8.06 PSI diff press
//   Cruise:  cabin alt held at equilibrium (~6000–8000 ft at FL390)
//   Descent: cabin alt descends to landing elevation, arriving at touchdown
//
// The outflow valve position is computed by a P-controller on cabin pressure.
// Safety relief valve opens at 8.35 PSI diff.
class Pressurization {
public:
    Pressurization() = default;

    void init();
    void update(float dt);

    // Inputs (set each tick from DataRefManager / other systems)
    void setAircraftAltFt(float alt);
    void setAmbientPressHpa(float hpa);
    void setOnGround(bool onGround);
    void setLandingElevationFt(float elev);  // from FMGC or manual entry
    void setAutoMode(bool autoMode);          // SYS 1/2 or MAN

    // Manual outflow valve command (only used when autoMode = false)
    void setManualOutflowPct(float pct);

    // State
    float cabinAltFt()        const { return m_cabinAltFt; }
    float diffPressPsi()      const { return m_diffPressPsi; }
    float cabinVsFpm()        const { return m_cabinVsFpm; }
    float outflowValvePct()   const { return m_outflowValvePct; }
    bool  isSafetyValveOpen() const { return m_safetyValveOpen; }
    bool  isExcessCabinAlt()  const { return m_cabinAltFt > EXCESS_CABIN_ALT_FT; }
    bool  isExcessDiffPress() const { return m_diffPressPsi > EXCESS_DIFF_PRESS_PSI; }

private:
    float pressHpaFromAltFt(float altFt) const;
    float altFtFromPressHpa(float pressHpa) const;

    float m_aircraftAltFt   = 0.f;
    float m_ambientPressHpa = 1013.25f;
    bool  m_onGround        = true;
    float m_landingElevFt   = 0.f;
    bool  m_autoMode        = true;
    float m_manualOutflowPct = 50.f;

    // State
    float m_cabinAltFt       = 0.f;
    float m_cabinPressHpa    = 1013.25f;
    float m_prevCabinAltFt   = 0.f;
    float m_cabinVsFpm       = 0.f;
    float m_diffPressPsi     = 0.f;
    float m_outflowValvePct  = 100.f;  // fully open on ground
    bool  m_safetyValveOpen  = false;

    // CPCS constants
    static constexpr float MAX_DIFF_PRESS_PSI    = 8.06f;
    static constexpr float SAFETY_RELIEF_PSI     = 8.35f;
    static constexpr float EXCESS_CABIN_ALT_FT   = 9550.f;   // ECAM warning level
    static constexpr float EXCESS_DIFF_PRESS_PSI = 8.35f;
    static constexpr float MAX_CABIN_VS_FPM      = 300.f;    // max cabin climb rate
    static constexpr float MAX_CABIN_DESC_FPM    = 750.f;    // max cabin descent rate
    static constexpr float HPA_PER_PSI           = 68.9476f;
};

} // namespace a320
