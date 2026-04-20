#include "ElectricalSystem.h"
#include "core/DataRefManager.h"
#include "core/EventBus.h"
#include "core/Logger.h"

namespace a320 {

// ── Custom dataref names ────────────────────────────────────────────────────
static constexpr const char* DR_AC1_PWR   = "a320/systems/electrical/ac_bus1_powered";
static constexpr const char* DR_AC2_PWR   = "a320/systems/electrical/ac_bus2_powered";
static constexpr const char* DR_ACESS_PWR = "a320/systems/electrical/ac_ess_powered";
static constexpr const char* DR_ACSHED_PWR= "a320/systems/electrical/ac_ess_shed_powered";
static constexpr const char* DR_DC1_PWR   = "a320/systems/electrical/dc_bus1_powered";
static constexpr const char* DR_DC2_PWR   = "a320/systems/electrical/dc_bus2_powered";
static constexpr const char* DR_DCESS_PWR = "a320/systems/electrical/dc_ess_powered";
static constexpr const char* DR_DCBAT_PWR = "a320/systems/electrical/dc_bat_powered";
static constexpr const char* DR_GEN1_V    = "a320/systems/electrical/gen1_voltage";
static constexpr const char* DR_GEN2_V    = "a320/systems/electrical/gen2_voltage";
static constexpr const char* DR_GEN1_HZ   = "a320/systems/electrical/gen1_freq";
static constexpr const char* DR_GEN2_HZ   = "a320/systems/electrical/gen2_freq";
static constexpr const char* DR_GEN1_LOAD = "a320/systems/electrical/gen1_load_pct";
static constexpr const char* DR_GEN2_LOAD = "a320/systems/electrical/gen2_load_pct";
static constexpr const char* DR_APU_V     = "a320/systems/electrical/apugen_voltage";
static constexpr const char* DR_APU_HZ    = "a320/systems/electrical/apugen_freq";
static constexpr const char* DR_BAT1_V    = "a320/systems/electrical/bat1_voltage";
static constexpr const char* DR_BAT2_V    = "a320/systems/electrical/bat2_voltage";
static constexpr const char* DR_BAT1_PCT  = "a320/systems/electrical/bat1_charge_pct";
static constexpr const char* DR_BAT2_PCT  = "a320/systems/electrical/bat2_charge_pct";

// ── Init / registration ─────────────────────────────────────────────────────

ElectricalSystem::ElectricalSystem() {}

void ElectricalSystem::init()
{
    registerDataRefs();
    // Default power-up state: GEN switches AUTO, BAT AUTO, BUS TIE AUTO
    m_idg1.setSwitch(true);
    m_idg2.setSwitch(true);
    m_apuGen.setSwitch(true);
    m_extPwr.setSwitch(false);
    LOG_INFO("ElectricalSystem initialised");
}

void ElectricalSystem::registerDataRefs()
{
    auto& dm = DataRefManager::instance();
    dm.registerInt  (DR_AC1_PWR,    0);
    dm.registerInt  (DR_AC2_PWR,    0);
    dm.registerInt  (DR_ACESS_PWR,  0);
    dm.registerInt  (DR_ACSHED_PWR, 0);
    dm.registerInt  (DR_DC1_PWR,    0);
    dm.registerInt  (DR_DC2_PWR,    0);
    dm.registerInt  (DR_DCESS_PWR,  0);
    dm.registerInt  (DR_DCBAT_PWR,  0);
    dm.registerFloat(DR_GEN1_V,     0.f);
    dm.registerFloat(DR_GEN2_V,     0.f);
    dm.registerFloat(DR_GEN1_HZ,    0.f);
    dm.registerFloat(DR_GEN2_HZ,    0.f);
    dm.registerFloat(DR_GEN1_LOAD,  0.f);
    dm.registerFloat(DR_GEN2_LOAD,  0.f);
    dm.registerFloat(DR_APU_V,      0.f);
    dm.registerFloat(DR_APU_HZ,     0.f);
    dm.registerFloat(DR_BAT1_V,    25.5f);
    dm.registerFloat(DR_BAT2_V,    25.5f);
    dm.registerFloat(DR_BAT1_PCT, 100.f);
    dm.registerFloat(DR_BAT2_PCT, 100.f);
}

// ── Main tick ───────────────────────────────────────────────────────────────

void ElectricalSystem::update(float dt)
{
    // 1. Update sources
    m_idg1.update(dt);
    m_idg2.update(dt);
    m_apuGen.update(dt);
    m_extPwr.update(dt);
    // Sync battery contactor state before DC network resolution so the topology
    // sees the correct open/closed state this tick.
    m_bat1.syncContactorState();
    m_bat2.syncContactorState();

    // 2. Resolve AC network
    updateACNetwork();

    // 3. Resolve DC network (depends on AC state and battery contactors)
    updateDCNetwork();

    // 4. Update batteries: set charging from resolved DC BAT BUS, then integrate SoC
    updateBatteries(dt);

    // 5. Publish events for changed bus states
    publishEvents();

    // 6. Write custom datarefs
    auto& dm = DataRefManager::instance();
    dm.setCustomInt(DR_AC1_PWR,    m_acBus1.isPowered()    ? 1 : 0);
    dm.setCustomInt(DR_AC2_PWR,    m_acBus2.isPowered()    ? 1 : 0);
    dm.setCustomInt(DR_ACESS_PWR,  m_acEss.isPowered()     ? 1 : 0);
    dm.setCustomInt(DR_ACSHED_PWR, m_acEssShed.isPowered() ? 1 : 0);
    dm.setCustomInt(DR_DC1_PWR,    m_dcBus1.isPowered()    ? 1 : 0);
    dm.setCustomInt(DR_DC2_PWR,    m_dcBus2.isPowered()    ? 1 : 0);
    dm.setCustomInt(DR_DCESS_PWR,  m_dcEss.isPowered()     ? 1 : 0);
    dm.setCustomInt(DR_DCBAT_PWR,  m_dcBat.isPowered()     ? 1 : 0);
    dm.setCustomFloat(DR_GEN1_V,    m_idg1.voltageAC());
    dm.setCustomFloat(DR_GEN2_V,    m_idg2.voltageAC());
    dm.setCustomFloat(DR_GEN1_HZ,   m_idg1.frequencyHz());
    dm.setCustomFloat(DR_GEN2_HZ,   m_idg2.frequencyHz());
    dm.setCustomFloat(DR_GEN1_LOAD, m_idg1.loadPct());
    dm.setCustomFloat(DR_GEN2_LOAD, m_idg2.loadPct());
    dm.setCustomFloat(DR_APU_V,     m_apuGen.voltageAC());
    dm.setCustomFloat(DR_APU_HZ,    m_apuGen.frequencyHz());
    dm.setCustomFloat(DR_BAT1_V,    m_bat1.voltage());
    dm.setCustomFloat(DR_BAT2_V,    m_bat2.voltage());
    dm.setCustomFloat(DR_BAT1_PCT,  m_bat1.chargePct());
    dm.setCustomFloat(DR_BAT2_PCT,  m_bat2.chargePct());
}

// ── AC network resolution ───────────────────────────────────────────────────
// A320 AC priority (per FCOM 24-40):
//   EXT PWR > APU GEN > IDG (GEN 1 for BUS 1, GEN 2 for BUS 2)
//   Bus tie (BTB) connects bus 1 and 2 when one source is lost.

void ElectricalSystem::updateACNetwork()
{
    bool gen1On  = m_idg1.isOnline();
    bool gen2On  = m_idg2.isOnline();
    bool apuOn   = m_apuGen.isOnline();
    bool extOn   = m_extPwr.isOnline();

    // Preferred source for each bus
    // EXT PWR and APU GEN feed both buses via the same contactor in practice,
    // but only one bus at a time without bus tie. We model EXT/APU as single-bus
    // and use bus tie to extend to the other side.

    // ── AC BUS 1 ────────────────────────────────────────────────────────────
    // Primary: GEN 1. Secondary (via tie): GEN 2, APU, EXT.
    bool acBus1Powered = false;
    float v1 = 0.f, f1 = 0.f;

    if (extOn) {
        // EXT PWR feeds AC BUS 1 first
        acBus1Powered = true;
        v1 = m_extPwr.voltageAC(); f1 = m_extPwr.frequencyHz();
    } else if (apuOn) {
        acBus1Powered = true;
        v1 = m_apuGen.voltageAC(); f1 = m_apuGen.frequencyHz();
    } else if (gen1On) {
        acBus1Powered = true;
        v1 = m_idg1.voltageAC(); f1 = m_idg1.frequencyHz();
    } else if (m_busTieAuto && gen2On) {
        // Bus tie: GEN 2 feeds BUS 1
        acBus1Powered = true;
        v1 = m_idg2.voltageAC(); f1 = m_idg2.frequencyHz();
    }
    m_acBus1.setPowered(acBus1Powered, v1, f1);

    // ── AC BUS 2 ────────────────────────────────────────────────────────────
    bool acBus2Powered = false;
    float v2 = 0.f, f2 = 0.f;

    if (extOn) {
        acBus2Powered = true;
        v2 = m_extPwr.voltageAC(); f2 = m_extPwr.frequencyHz();
    } else if (apuOn) {
        acBus2Powered = true;
        v2 = m_apuGen.voltageAC(); f2 = m_apuGen.frequencyHz();
    } else if (gen2On) {
        acBus2Powered = true;
        v2 = m_idg2.voltageAC(); f2 = m_idg2.frequencyHz();
    } else if (m_busTieAuto && gen1On) {
        // Bus tie: GEN 1 feeds BUS 2
        acBus2Powered = true;
        v2 = m_idg1.voltageAC(); f2 = m_idg1.frequencyHz();
    }
    m_acBus2.setPowered(acBus2Powered, v2, f2);

    // ── AC ESS BUS ──────────────────────────────────────────────────────────
    // NORM: fed from AC BUS 1. ALTN: fed from AC BUS 2.
    bool acEssFeed = m_acEssFeedAltn ? m_acBus2.isPowered() : m_acBus1.isPowered();
    float ve = m_acEssFeedAltn ? m_acBus2.voltageAC() : m_acBus1.voltageAC();
    float fe = m_acEssFeedAltn ? m_acBus2.frequencyHz() : m_acBus1.frequencyHz();
    m_acEss.setPowered(acEssFeed, ve, fe);

    // ── AC ESS SHED ─────────────────────────────────────────────────────────
    // Shed when only one source is available (single-gen operation)
    int srcCount = onlineACSourceCount();
    bool shedPowered = acEssFeed && (srcCount >= 2 || extOn || apuOn);
    m_acEssShed.setPowered(shedPowered, ve, fe);
}

// ── DC network resolution ───────────────────────────────────────────────────

void ElectricalSystem::updateDCNetwork()
{
    // TR1 rectifies AC BUS 1 → DC BUS 1
    bool dc1 = m_acBus1.isPowered();
    m_dcBus1.setPowered(dc1, dc1 ? 28.f : 0.f);

    // TR2 rectifies AC BUS 2 → DC BUS 2
    bool dc2 = m_acBus2.isPowered();
    m_dcBus2.setPowered(dc2, dc2 ? 28.f : 0.f);

    // DC BAT BUS: powered when DC BUS 1, DC BUS 2, or any battery contactor closed
    bool dcBat = dc1 || dc2
                 || m_bat1.isContactorClosed()
                 || m_bat2.isContactorClosed();
    float batV = 0.f;
    if (dc1 || dc2) batV = 28.f;
    else if (m_bat1.isContactorClosed()) batV = m_bat1.voltage();
    else if (m_bat2.isContactorClosed()) batV = m_bat2.voltage();
    m_dcBat.setPowered(dcBat, batV);

    // DC ESS BUS: normally from DC BAT BUS, alternate from ESS TR (AC ESS → DC ESS)
    bool dcEss = m_dcBat.isPowered() || m_acEss.isPowered();
    float essV = m_dcBat.isPowered() ? m_dcBat.voltage() : (m_acEss.isPowered() ? 28.f : 0.f);
    m_dcEss.setPowered(dcEss, essV);

    // HOT BUSses: directly connected to respective battery (always live if battery charged)
    m_hotBus1.setPowered(m_bat1.chargePct() > 1.f, m_bat1.voltage());
    m_hotBus2.setPowered(m_bat2.chargePct() > 1.f, m_bat2.voltage());
}

// ── Battery charging ────────────────────────────────────────────────────────

void ElectricalSystem::updateBatteries(float dt)
{
    // Set charging state from the now-resolved DC BAT BUS voltage, then run the
    // full battery update which integrates SoC and voltage for this tick.
    bool dcBatUp = m_dcBat.isPowered() && m_dcBat.voltage() >= 28.f;
    m_bat1.setCharging(dcBatUp, m_dcBat.voltage());
    m_bat2.setCharging(dcBatUp, m_dcBat.voltage());
    m_bat1.update(dt);
    m_bat2.update(dt);
}

// ── EventBus publishing ─────────────────────────────────────────────────────

void ElectricalSystem::publishEvents()
{
    auto check = [](bool prev, bool curr, const char* name) {
        if (curr != prev)
            A320_PUBLISH(BusPoweredEvent{ name, curr });
    };
    check(m_prevAcBus1Powered,    m_acBus1.isPowered(),    "AC_BUS_1");
    check(m_prevAcBus2Powered,    m_acBus2.isPowered(),    "AC_BUS_2");
    check(m_prevAcEssPowered,     m_acEss.isPowered(),     "AC_ESS");
    check(m_prevAcEssShedPowered, m_acEssShed.isPowered(), "AC_ESS_SHED");

    m_prevAcBus1Powered    = m_acBus1.isPowered();
    m_prevAcBus2Powered    = m_acBus2.isPowered();
    m_prevAcEssPowered     = m_acEss.isPowered();
    m_prevAcEssShedPowered = m_acEssShed.isPowered();
}

// ── Control inputs ──────────────────────────────────────────────────────────

void ElectricalSystem::setGen1Switch(bool on)        { m_idg1.setSwitch(on); }
void ElectricalSystem::setGen2Switch(bool on)        { m_idg2.setSwitch(on); }
void ElectricalSystem::setApuGenSwitch(bool on)      { m_apuGen.setSwitch(on); }
void ElectricalSystem::setExtPwrSwitch(bool on)      { m_extPwr.setSwitch(on); }
void ElectricalSystem::setBat1Switch(bool on)        { m_bat1.setSwitch(on); }
void ElectricalSystem::setBat2Switch(bool on)        { m_bat2.setSwitch(on); }
void ElectricalSystem::setAcBusTieSwitch(bool on)    { m_busTieAuto = on; }
void ElectricalSystem::setAcEssFeedAltn(bool altn)   { m_acEssFeedAltn = altn; }
void ElectricalSystem::setEng1N2(float n2)           { m_idg1.setN2(n2); }
void ElectricalSystem::setEng2N2(float n2)           { m_idg2.setN2(n2); }
void ElectricalSystem::setApuAvailable(bool avail)   { m_apuGen.setAvailable(avail); }
void ElectricalSystem::setExtPwrAvailable(bool avail){ m_extPwr.setAvailable(avail); }

// ── Helpers ─────────────────────────────────────────────────────────────────

int ElectricalSystem::onlineACSourceCount() const
{
    return (m_idg1.isOnline()   ? 1 : 0)
         + (m_idg2.isOnline()   ? 1 : 0)
         + (m_apuGen.isOnline() ? 1 : 0)
         + (m_extPwr.isOnline() ? 1 : 0);
}

bool ElectricalSystem::isAnythingPowered() const
{
    return m_acBus1.isPowered() || m_acBus2.isPowered()
        || m_dcBus1.isPowered() || m_dcBus2.isPowered()
        || m_hotBus1.isPowered() || m_hotBus2.isPowered();
}

} // namespace a320
