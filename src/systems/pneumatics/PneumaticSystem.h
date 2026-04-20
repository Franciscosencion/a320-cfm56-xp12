#pragma once
#include "BleedSource.h"
#include "Pack.h"
#include "AntiIce.h"
#include "Pressurization.h"

namespace a320 {

// Cross-bleed valve selector positions
enum class XBleedMode { Auto, Open, Closed };

// A320 Pneumatic System — top-level orchestrator.
//
// Topology:
//   ENG1 bleed ──[BLEED1 pb]──┬── LEFT manifold ──[PACK1 FCV]── PACK 1
//   APU bleed  ──[APU pb]──────┤                                  │
//                               └──[X-BLEED valve]────────────────┘
//   ENG2 bleed ──[BLEED2 pb]──┬── RIGHT manifold ─[PACK2 FCV]── PACK 2
//   APU bleed  ──[APU pb]──────┤                                  │
//                               └──[X-BLEED valve]────────────────┘
//
// With X-bleed OPEN, both manifolds are connected — one source feeds both packs.
// APU bleed feeds the left manifold and (via X-bleed) the right as well.
class PneumaticSystem {
public:
    PneumaticSystem();

    void init();
    void update(float dt);

    // ── Engine / APU inputs (fed each tick) ──────────────────────────────────
    void setEng1N2(float n2Pct);
    void setEng2N2(float n2Pct);
    // FCOM DSC-36-10-30: APU bleed available when APU N > 95%
    void setApuNPct(float nPct);
    void setApuBleedAvail(bool avail);  // legacy: treated as N=100% when true, N=0% when false
    void setOatDegC(float oat);
    void setAircraftAltFt(float alt);
    void setAmbientPressHpa(float hpa);
    void setOnGround(bool onGround);

    // ── Overhead panel controls ───────────────────────────────────────────────
    void setBleed1Switch(bool on);
    void setBleed2Switch(bool on);
    void setApuBleedSwitch(bool on);
    void setXBleedSelector(XBleedMode mode);
    void setPack1Switch(bool norm);
    void setPack2Switch(bool norm);
    void setPackHiFlow(bool hi);
    void setLandingElevationFt(float elev);
    void setWingAntiIce(bool on);
    void setEng1AntiIce(bool on);
    void setEng2AntiIce(bool on);

    // ── State queries ─────────────────────────────────────────────────────────
    float manifold1PressurePsi() const { return m_manifold1Psi; }
    float manifold2PressurePsi() const { return m_manifold2Psi; }
    bool  isApuBleedActive()     const { return m_apuBleedActive; }
    bool  isXBleedOpen()         const { return m_xBleedOpen; }

    const BleedSource&   bleed1()        const { return m_bleed1; }
    const BleedSource&   bleed2()        const { return m_bleed2; }
    const Pack&          pack1()         const { return m_pack1; }
    const Pack&          pack2()         const { return m_pack2; }
    const AntiIce&       antiIce()       const { return m_antiIce; }
    const Pressurization& pressurization() const { return m_press; }

private:
    void updateManifolds();
    void registerDataRefs();

    BleedSource   m_bleed1 { 0 };
    BleedSource   m_bleed2 { 1 };
    Pack          m_pack1  { 0 };
    Pack          m_pack2  { 1 };
    AntiIce       m_antiIce;
    Pressurization m_press;

    // Inputs
    float       m_oatDegC        = 15.f;
    float       m_altFt          = 0.f;
    float       m_ambientPressHpa = 1013.25f;
    bool        m_onGround       = true;
    float       m_apuNPct        = 0.f;   // APU speed %N; bleed available when > 95%
    bool        m_apuBleedSwitch = true;

    // FCOM DSC-36-10-30: APU bleed available when N > 95%
    static constexpr float APU_BLEED_MIN_N = 95.f;
    XBleedMode  m_xBleedMode     = XBleedMode::Auto;

    // Resolved each tick
    float m_manifold1Psi  = 0.f;
    float m_manifold2Psi  = 0.f;
    bool  m_apuBleedActive = false;
    bool  m_xBleedOpen    = false;
};

} // namespace a320
