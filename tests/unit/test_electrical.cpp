#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "systems/electrical/ElectricalSystem.h"
#include "core/DataRefManager.h"

using namespace a320;
using Catch::Matchers::WithinAbs;

// Helper: run one tick with both engines at cruise N2 (80%)
static void tickBothEngines(ElectricalSystem& elec, float dt = 0.02f)
{
    elec.setEng1N2(80.f);
    elec.setEng2N2(80.f);
    elec.update(dt);
}

// Helper: run one tick with no engines
static void tickNoEngines(ElectricalSystem& elec, float dt = 0.02f)
{
    elec.setEng1N2(0.f);
    elec.setEng2N2(0.f);
    elec.update(dt);
}

// Fresh system for each test (DataRefManager singleton may accumulate keys; that's fine)
static ElectricalSystem makeSystem()
{
    ElectricalSystem elec;
    elec.init();
    return elec;
}

// ── Normal dual-generator operation ─────────────────────────────────────────

TEST_CASE("Both IDGs online: all main buses powered", "[electrical]")
{
    auto elec = makeSystem();
    tickBothEngines(elec);

    CHECK(elec.acBus1().isPowered());
    CHECK(elec.acBus2().isPowered());
    CHECK(elec.acEss().isPowered());
    CHECK(elec.acEssShed().isPowered());  // 2 sources → shed stays live

    CHECK(elec.dcBus1().isPowered());
    CHECK(elec.dcBus2().isPowered());
    CHECK(elec.dcBat().isPowered());
    CHECK(elec.dcEss().isPowered());
    CHECK(elec.hotBus1().isPowered());
    CHECK(elec.hotBus2().isPowered());
}

TEST_CASE("Both IDGs online: voltages and frequencies within limits", "[electrical]")
{
    auto elec = makeSystem();
    tickBothEngines(elec);

    CHECK_THAT(elec.acBus1().voltageAC(),   WithinAbs(115.f, 1.f));
    CHECK_THAT(elec.acBus1().frequencyHz(), WithinAbs(400.f, 1.f));
    CHECK_THAT(elec.dcBus1().voltage(),     WithinAbs(28.f, 1.f));
}

// ── Single-generator operation (bus tie) ────────────────────────────────────

TEST_CASE("GEN 1 only: bus tie feeds AC BUS 2, ESS SHED drops", "[electrical]")
{
    auto elec = makeSystem();
    elec.setEng1N2(80.f);
    elec.setEng2N2(0.f);   // GEN 2 offline
    elec.update(0.02f);

    CHECK(elec.acBus1().isPowered());
    CHECK(elec.acBus2().isPowered());     // fed via bus tie from GEN 1
    CHECK(elec.acEss().isPowered());
    CHECK_FALSE(elec.acEssShed().isPowered()); // only 1 source → shed
}

TEST_CASE("GEN 2 only: bus tie feeds AC BUS 1", "[electrical]")
{
    auto elec = makeSystem();
    elec.setEng1N2(0.f);
    elec.setEng2N2(80.f);
    elec.update(0.02f);

    CHECK(elec.acBus1().isPowered());     // fed via bus tie from GEN 2
    CHECK(elec.acBus2().isPowered());
    CHECK(elec.acEss().isPowered());
    CHECK_FALSE(elec.acEssShed().isPowered());
}

TEST_CASE("Bus tie OFF + GEN 1 only: AC BUS 2 unpowered", "[electrical]")
{
    auto elec = makeSystem();
    elec.setAcBusTieSwitch(false);
    elec.setEng1N2(80.f);
    elec.setEng2N2(0.f);
    elec.update(0.02f);

    CHECK(elec.acBus1().isPowered());
    CHECK_FALSE(elec.acBus2().isPowered());
}

// ── APU generator ───────────────────────────────────────────────────────────

TEST_CASE("APU GEN: powers both buses when no IDGs", "[electrical]")
{
    auto elec = makeSystem();
    elec.setApuAvailable(true);
    tickNoEngines(elec);

    CHECK(elec.acBus1().isPowered());
    CHECK(elec.acBus2().isPowered());
    CHECK(elec.acEss().isPowered());
}

TEST_CASE("APU GEN switch OFF: APU does not power buses", "[electrical]")
{
    auto elec = makeSystem();
    elec.setApuGenSwitch(false);
    elec.setApuAvailable(true);
    tickNoEngines(elec);

    CHECK_FALSE(elec.acBus1().isPowered());
}

// ── External power ───────────────────────────────────────────────────────────

TEST_CASE("EXT PWR: powers both buses when available and switch ON", "[electrical]")
{
    auto elec = makeSystem();
    elec.setExtPwrSwitch(true);
    elec.setExtPwrAvailable(true);
    tickNoEngines(elec);

    CHECK(elec.acBus1().isPowered());
    CHECK(elec.acBus2().isPowered());
}

TEST_CASE("EXT PWR priority over APU and IDG", "[electrical]")
{
    auto elec = makeSystem();
    elec.setExtPwrSwitch(true);
    elec.setExtPwrAvailable(true);
    elec.setApuAvailable(true);
    elec.setEng1N2(80.f);
    elec.update(0.02f);

    // EXT PWR is highest priority — generator should still be online but EXT takes bus
    CHECK(elec.acBus1().isPowered());
    // Voltage comes from EXT PWR (115V 400Hz)
    CHECK_THAT(elec.acBus1().voltageAC(),   WithinAbs(115.f, 1.f));
    CHECK_THAT(elec.acBus1().frequencyHz(), WithinAbs(400.f, 1.f));
}

// ── AC ESS feed selection ────────────────────────────────────────────────────

TEST_CASE("AC ESS FEED NORM: powered from AC BUS 1", "[electrical]")
{
    auto elec = makeSystem();
    elec.setAcEssFeedAltn(false);  // NORM
    elec.setEng1N2(80.f);
    elec.setEng2N2(0.f);
    elec.update(0.02f);

    // BUS 1 is powered (from IDG 1), ESS should be fed from it
    CHECK(elec.acEss().isPowered());
}

TEST_CASE("AC ESS FEED ALTN: powered from AC BUS 2", "[electrical]")
{
    auto elec = makeSystem();
    elec.setAcEssFeedAltn(true);   // ALTN
    elec.setEng1N2(0.f);
    elec.setEng2N2(80.f);
    elec.update(0.02f);

    CHECK(elec.acEss().isPowered());
}

TEST_CASE("AC ESS FEED NORM: AC BUS 1 lost + no bus tie -> AC ESS unpowered", "[electrical]")
{
    auto elec = makeSystem();
    elec.setAcEssFeedAltn(false);
    elec.setAcBusTieSwitch(false);  // no tie
    elec.setEng1N2(0.f);            // GEN 1 offline
    elec.setEng2N2(80.f);           // only GEN 2
    elec.update(0.02f);

    // AC BUS 1 unpowered (no tie, no GEN 1), ESS NORM feed fails
    CHECK_FALSE(elec.acBus1().isPowered());
    CHECK_FALSE(elec.acEss().isPowered());
}

// ── DC network ───────────────────────────────────────────────────────────────

TEST_CASE("No AC: DC buses unpowered (HOT buses stay live from batteries)", "[electrical]")
{
    auto elec = makeSystem();
    tickNoEngines(elec);

    CHECK_FALSE(elec.dcBus1().isPowered());
    CHECK_FALSE(elec.dcBus2().isPowered());
    // HOT buses remain powered (batteries charged from factory)
    CHECK(elec.hotBus1().isPowered());
    CHECK(elec.hotBus2().isPowered());
}

TEST_CASE("Battery-only: HOT buses live, DC BAT BUS powered via battery contactor", "[electrical]")
{
    auto elec = makeSystem();
    // Batteries default to switch=AUTO (on), charged at 100%
    tickNoEngines(elec);

    CHECK(elec.hotBus1().isPowered());
    CHECK(elec.hotBus2().isPowered());
    CHECK(elec.dcBat().isPowered());   // fed by battery contactor
    CHECK(elec.dcEss().isPowered());   // fed from DC BAT BUS
}

TEST_CASE("Battery switches OFF: DC BAT BUS unpowered; HOT buses remain live (hardwired to batteries)", "[electrical]")
{
    // On the real A320 the HOT bus is hardwired to the battery regardless of the
    // battery switch. The switch only opens the contactor to the DC BAT BUS.
    auto elec = makeSystem();
    elec.setBat1Switch(false);
    elec.setBat2Switch(false);
    tickNoEngines(elec);

    CHECK_FALSE(elec.dcBat().isPowered());  // battery cut off from main bus
    CHECK(elec.hotBus1().isPowered());       // still live — hardwired to battery
    CHECK(elec.hotBus2().isPowered());
}

// ── Generator failure ────────────────────────────────────────────────────────

TEST_CASE("GEN 1 switch OFF: GEN 1 goes offline, bus tie covers BUS 1", "[electrical]")
{
    auto elec = makeSystem();
    elec.setGen1Switch(false);  // GEN 1 pb to OFF
    elec.setEng1N2(80.f);
    elec.setEng2N2(80.f);
    elec.update(0.02f);

    CHECK_FALSE(elec.idg1().isOnline());
    CHECK(elec.acBus1().isPowered());    // GEN 2 covers via tie
    CHECK(elec.acBus2().isPowered());
}

TEST_CASE("Both generators offline: AC buses dead, batteries sustain DC ESS", "[electrical]")
{
    auto elec = makeSystem();
    tickNoEngines(elec);

    CHECK_FALSE(elec.acBus1().isPowered());
    CHECK_FALSE(elec.acBus2().isPowered());
    CHECK_FALSE(elec.acEss().isPowered());
    CHECK_FALSE(elec.dcBus1().isPowered());
    CHECK_FALSE(elec.dcBus2().isPowered());
    CHECK(elec.dcEss().isPowered());    // via DC BAT BUS ← battery contactors
    CHECK(elec.hotBus1().isPowered());
    CHECK(elec.hotBus2().isPowered());
}

// ── isAnythingPowered ────────────────────────────────────────────────────────

TEST_CASE("isAnythingPowered: true when batteries charged (HOT buses live)", "[electrical]")
{
    auto elec = makeSystem();
    tickNoEngines(elec);
    CHECK(elec.isAnythingPowered());
}

TEST_CASE("isAnythingPowered: true even with battery switches off (HOT buses hardwired)", "[electrical]")
{
    // HOT buses are always live while batteries have charge, so isAnythingPowered
    // remains true even with battery switches off and no AC power.
    auto elec = makeSystem();
    elec.setBat1Switch(false);
    elec.setBat2Switch(false);
    tickNoEngines(elec);
    CHECK(elec.isAnythingPowered());
}

// ── DataRef publication ──────────────────────────────────────────────────────

TEST_CASE("Custom datarefs reflect bus state after update", "[electrical]")
{
    auto elec = makeSystem();
    tickBothEngines(elec);

    auto& dm = DataRefManager::instance();
    CHECK(dm.getCustomInt("a320/systems/electrical/ac_bus1_powered") == 1);
    CHECK(dm.getCustomInt("a320/systems/electrical/ac_bus2_powered") == 1);
    CHECK(dm.getCustomInt("a320/systems/electrical/dc_bus1_powered") == 1);
    CHECK(dm.getCustomFloat("a320/systems/electrical/gen1_voltage")  > 100.f);
    CHECK(dm.getCustomFloat("a320/systems/electrical/gen1_freq")     > 390.f);
}
