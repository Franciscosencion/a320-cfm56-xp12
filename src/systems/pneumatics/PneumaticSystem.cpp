#include "PneumaticSystem.h"
#include "core/DataRefManager.h"
#include "core/Logger.h"

namespace a320 {

// ── Custom dataref names ─────────────────────────────────────────────────────
static constexpr const char* DR_MAN1_PSI    = "a320/systems/pneumatics/manifold1_psi";
static constexpr const char* DR_MAN2_PSI    = "a320/systems/pneumatics/manifold2_psi";
static constexpr const char* DR_PACK1_ON    = "a320/systems/pneumatics/pack1_on";
static constexpr const char* DR_PACK2_ON    = "a320/systems/pneumatics/pack2_on";
static constexpr const char* DR_PACK1_TEMP  = "a320/systems/pneumatics/pack1_temp_c";
static constexpr const char* DR_PACK2_TEMP  = "a320/systems/pneumatics/pack2_temp_c";
static constexpr const char* DR_XBLEED_OPEN = "a320/systems/pneumatics/xbleed_open";
static constexpr const char* DR_APU_BLEED   = "a320/systems/pneumatics/apu_bleed_on";
static constexpr const char* DR_WAI         = "a320/systems/pneumatics/wing_anti_ice";
static constexpr const char* DR_EAI1        = "a320/systems/pneumatics/eng1_anti_ice";
static constexpr const char* DR_EAI2        = "a320/systems/pneumatics/eng2_anti_ice";
static constexpr const char* DR_CABIN_ALT   = "a320/systems/pressurization/cabin_alt_ft";
static constexpr const char* DR_DIFF_PRESS  = "a320/systems/pressurization/diff_press_psi";
static constexpr const char* DR_CABIN_VS    = "a320/systems/pressurization/cabin_vs_fpm";
static constexpr const char* DR_OUTFLOW_PCT = "a320/systems/pressurization/outflow_pct";

PneumaticSystem::PneumaticSystem() = default;

void PneumaticSystem::init()
{
    m_press.init();
    registerDataRefs();
    LOG_INFO("PneumaticSystem initialised");
}

void PneumaticSystem::registerDataRefs()
{
    auto& dm = DataRefManager::instance();
    dm.registerFloat(DR_MAN1_PSI,   0.f);
    dm.registerFloat(DR_MAN2_PSI,   0.f);
    dm.registerInt  (DR_PACK1_ON,   0);
    dm.registerInt  (DR_PACK2_ON,   0);
    dm.registerFloat(DR_PACK1_TEMP, 20.f);
    dm.registerFloat(DR_PACK2_TEMP, 20.f);
    dm.registerInt  (DR_XBLEED_OPEN, 0);
    dm.registerInt  (DR_APU_BLEED,   0);
    dm.registerInt  (DR_WAI,         0);
    dm.registerInt  (DR_EAI1,        0);
    dm.registerInt  (DR_EAI2,        0);
    dm.registerFloat(DR_CABIN_ALT,   0.f);
    dm.registerFloat(DR_DIFF_PRESS,  0.f);
    dm.registerFloat(DR_CABIN_VS,    0.f);
    dm.registerFloat(DR_OUTFLOW_PCT, 100.f);
}

void PneumaticSystem::update(float dt)
{
    // 1. Update bleed sources
    m_bleed1.update(dt);
    m_bleed2.update(dt);

    // 2. Resolve manifold pressures and X-bleed
    updateManifolds();

    // 3. Update packs
    float bleedTemp1 = m_bleed1.isFlowAvailable() ? m_bleed1.bleedTempC() : (m_apuBleedActive ? 160.f : 20.f);
    float bleedTemp2 = m_bleed2.isFlowAvailable() ? m_bleed2.bleedTempC() : (m_apuBleedActive ? 160.f : 20.f);

    m_pack1.setInletPressurePsi(m_manifold1Psi);
    m_pack1.setInletTempC(bleedTemp1);
    m_pack1.setRamAirTempC(m_oatDegC);
    m_pack1.update(dt);

    m_pack2.setInletPressurePsi(m_manifold2Psi);
    m_pack2.setInletTempC(bleedTemp2);
    m_pack2.setRamAirTempC(m_oatDegC);
    m_pack2.update(dt);

    // 4. Update anti-ice
    m_antiIce.setManifold1Avail(m_manifold1Psi > 15.f);
    m_antiIce.setManifold2Avail(m_manifold2Psi > 15.f);
    m_antiIce.update(dt);

    // 5. Update pressurization
    m_press.setAircraftAltFt(m_altFt);
    m_press.setAmbientPressHpa(m_ambientPressHpa);
    m_press.setOnGround(m_onGround);
    m_press.update(dt);

    // 6. Publish custom datarefs
    auto& dm = DataRefManager::instance();
    dm.setCustomFloat(DR_MAN1_PSI,    m_manifold1Psi);
    dm.setCustomFloat(DR_MAN2_PSI,    m_manifold2Psi);
    dm.setCustomInt  (DR_PACK1_ON,    m_pack1.isOperating() ? 1 : 0);
    dm.setCustomInt  (DR_PACK2_ON,    m_pack2.isOperating() ? 1 : 0);
    dm.setCustomFloat(DR_PACK1_TEMP,  m_pack1.outletTempC());
    dm.setCustomFloat(DR_PACK2_TEMP,  m_pack2.outletTempC());
    dm.setCustomInt  (DR_XBLEED_OPEN, m_xBleedOpen ? 1 : 0);
    dm.setCustomInt  (DR_APU_BLEED,   m_apuBleedActive ? 1 : 0);
    dm.setCustomInt  (DR_WAI,         m_antiIce.isWaiActive() ? 1 : 0);
    dm.setCustomInt  (DR_EAI1,        m_antiIce.isEai1Active() ? 1 : 0);
    dm.setCustomInt  (DR_EAI2,        m_antiIce.isEai2Active() ? 1 : 0);
    dm.setCustomFloat(DR_CABIN_ALT,   m_press.cabinAltFt());
    dm.setCustomFloat(DR_DIFF_PRESS,  m_press.diffPressPsi());
    dm.setCustomFloat(DR_CABIN_VS,    m_press.cabinVsFpm());
    dm.setCustomFloat(DR_OUTFLOW_PCT, m_press.outflowValvePct());
}

void PneumaticSystem::updateManifolds()
{
    bool eng1Flow = m_bleed1.isFlowAvailable();
    bool eng2Flow = m_bleed2.isFlowAvailable();

    m_apuBleedActive = m_apuBleedAvail && m_apuBleedSwitch;

    // X-bleed valve logic (FCOM DSC-36-20):
    //   AUTO: open ONLY when APU bleed valve is open; closed otherwise
    //   OPEN: always open
    //   SHUT: always closed
    bool xBleedOpen = false;
    switch (m_xBleedMode) {
        case XBleedMode::Open:
            xBleedOpen = true;
            break;
        case XBleedMode::Closed:
            xBleedOpen = false;
            break;
        case XBleedMode::Auto:
            // AUTO opens only when APU bleed is active (DSC-36-20 item 3)
            xBleedOpen = m_apuBleedActive;
            break;
    }
    m_xBleedOpen = xBleedOpen;

    // Left manifold (PACK 1 side): ENG1 bleed or APU or cross-fed from right
    if (eng1Flow) {
        m_manifold1Psi = m_bleed1.pressurePsi();
    } else if (m_apuBleedActive) {
        m_manifold1Psi = 45.f;  // APU bleed pressure (lower than HP)
    } else if (xBleedOpen && eng2Flow) {
        m_manifold1Psi = m_bleed2.pressurePsi();
    } else {
        m_manifold1Psi = 0.f;
    }

    // Right manifold (PACK 2 side): ENG2 bleed or APU (via x-bleed) or cross-fed from left
    if (eng2Flow) {
        m_manifold2Psi = m_bleed2.pressurePsi();
    } else if (m_apuBleedActive && xBleedOpen) {
        m_manifold2Psi = 45.f;
    } else if (xBleedOpen && eng1Flow) {
        m_manifold2Psi = m_bleed1.pressurePsi();
    } else {
        m_manifold2Psi = 0.f;
    }
}

// ── Setters ─────────────────────────────────────────────────────────────────

void PneumaticSystem::setEng1N2(float n2)            { m_bleed1.setN2(n2); }
void PneumaticSystem::setEng2N2(float n2)            { m_bleed2.setN2(n2); }
void PneumaticSystem::setApuBleedAvail(bool avail)   { m_apuBleedAvail  = avail; }
void PneumaticSystem::setOatDegC(float oat)          { m_oatDegC        = oat; }
void PneumaticSystem::setAircraftAltFt(float alt)    { m_altFt          = alt; }
void PneumaticSystem::setAmbientPressHpa(float hpa)  { m_ambientPressHpa = hpa; }
void PneumaticSystem::setOnGround(bool onGround)     { m_onGround       = onGround; }

void PneumaticSystem::setBleed1Switch(bool on)       { m_bleed1.setSwitch(on); }
void PneumaticSystem::setBleed2Switch(bool on)       { m_bleed2.setSwitch(on); }
void PneumaticSystem::setApuBleedSwitch(bool on)     { m_apuBleedSwitch = on; }
void PneumaticSystem::setXBleedSelector(XBleedMode m){ m_xBleedMode     = m; }
void PneumaticSystem::setPack1Switch(bool norm)      { m_pack1.setSwitch(norm); }
void PneumaticSystem::setPack2Switch(bool norm)      { m_pack2.setSwitch(norm); }
void PneumaticSystem::setPackHiFlow(bool hi)         { m_pack1.setHiFlow(hi); m_pack2.setHiFlow(hi); }
void PneumaticSystem::setLandingElevationFt(float e) { m_press.setLandingElevationFt(e); }
void PneumaticSystem::setWingAntiIce(bool on)        { m_antiIce.setWingAntiIce(on); }
void PneumaticSystem::setEng1AntiIce(bool on)        { m_antiIce.setEng1AntiIce(on); }
void PneumaticSystem::setEng2AntiIce(bool on)        { m_antiIce.setEng2AntiIce(on); }

} // namespace a320
