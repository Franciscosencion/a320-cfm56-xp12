#pragma once

namespace a320 {

// A320 anti-ice systems.
// WAI (Wing Anti-Ice): uses bleed air from manifold; powered from either side.
// EAI (Engine Anti-Ice): dedicated bleed path per engine, independent of BLEED pb.
class AntiIce {
public:
    AntiIce() = default;

    void update(float dt);

    // Overhead panel controls
    void setWingAntiIce(bool on);
    void setEng1AntiIce(bool on);
    void setEng2AntiIce(bool on);

    // Bleed availability from PneumaticSystem (manifold pressure > threshold)
    void setManifold1Avail(bool avail);
    void setManifold2Avail(bool avail);

    // State
    bool  isWaiActive()        const { return m_waiActive; }
    bool  isEai1Active()       const { return m_eai1Active; }
    bool  isEai2Active()       const { return m_eai2Active; }

    // Approximate bleed consumption (kg/h) — used by PneumaticSystem for load model
    float waiBleedKgH()        const { return m_waiActive ? WAI_FLOW_KGH : 0.f; }
    float eai1BleedKgH()       const { return m_eai1Active ? EAI_FLOW_KGH : 0.f; }
    float eai2BleedKgH()       const { return m_eai2Active ? EAI_FLOW_KGH : 0.f; }

private:
    bool m_waiSwitch    = false;
    bool m_eai1Switch   = false;
    bool m_eai2Switch   = false;

    bool m_manifold1Avail = false;
    bool m_manifold2Avail = false;

    bool m_waiActive    = false;
    bool m_eai1Active   = false;
    bool m_eai2Active   = false;

    // WAI draws from both manifolds; EAI from respective engine bleed
    static constexpr float WAI_FLOW_KGH = 1500.f;  // total WAI bleed consumption
    static constexpr float EAI_FLOW_KGH =  800.f;  // per engine
};

} // namespace a320
