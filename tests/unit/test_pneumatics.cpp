#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include "systems/pneumatics/PneumaticSystem.h"
#include "systems/pneumatics/Pressurization.h"
#include "core/DataRefManager.h"

using namespace a320;
using Catch::Matchers::WithinAbs;

static PneumaticSystem makeSystem()
{
    PneumaticSystem pneu;
    pneu.init();
    return pneu;
}

static void tickBothEngines(PneumaticSystem& pneu, float dt = 0.02f)
{
    pneu.setEng1N2(80.f);
    pneu.setEng2N2(80.f);
    pneu.update(dt);
}

static void tickNoEngines(PneumaticSystem& pneu, float dt = 0.02f)
{
    pneu.setEng1N2(0.f);
    pneu.setEng2N2(0.f);
    pneu.update(dt);
}

// ── Bleed source ─────────────────────────────────────────────────────────────

TEST_CASE("Bleed: N2 > 68% -> bleed valve open at 45 PSI (regulated delivery)", "[pneumatics]")
{
    // FCOM DSC-36-10-20: bleed valve maintains delivery pressure at 45 +/-5 PSI
    auto pneu = makeSystem();
    tickBothEngines(pneu);

    CHECK(pneu.bleed1().isFlowAvailable());
    CHECK(pneu.bleed2().isFlowAvailable());
    CHECK_THAT(pneu.bleed1().pressurePsi(), WithinAbs(45.f, 0.1f));
    CHECK_THAT(pneu.bleed2().pressurePsi(), WithinAbs(45.f, 0.1f));
}

TEST_CASE("Bleed: N2 between 15-68% -> bleed still available at 45 PSI (IP stage, BMC-regulated)", "[pneumatics]")
{
    // FCOM DSC-36-10-20: IP is normal source; bleed valve regulates to 45 PSI regardless of stage
    auto pneu = makeSystem();
    pneu.setEng1N2(40.f);
    pneu.setEng2N2(40.f);
    pneu.update(0.02f);

    CHECK(pneu.bleed1().isFlowAvailable());
    CHECK_THAT(pneu.bleed1().pressurePsi(), WithinAbs(45.f, 0.1f));
}

TEST_CASE("Bleed: N2 < 15% -> no bleed flow", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setEng1N2(10.f);
    pneu.setEng2N2(10.f);
    pneu.update(0.02f);

    CHECK_FALSE(pneu.bleed1().isFlowAvailable());
    CHECK_FALSE(pneu.bleed2().isFlowAvailable());
}

TEST_CASE("Bleed switch OFF: no flow even with N2 > 68%", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setBleed1Switch(false);
    pneu.setEng1N2(80.f);
    pneu.setEng2N2(80.f);
    pneu.update(0.02f);

    CHECK_FALSE(pneu.bleed1().isFlowAvailable());
    CHECK(pneu.bleed2().isFlowAvailable());  // ENG2 still on
}

// ── Manifold pressure ────────────────────────────────────────────────────────

TEST_CASE("Both engines: both manifolds pressurised", "[pneumatics]")
{
    auto pneu = makeSystem();
    tickBothEngines(pneu);

    CHECK(pneu.manifold1PressurePsi() > 0.f);
    CHECK(pneu.manifold2PressurePsi() > 0.f);
}

TEST_CASE("X-bleed AUTO: ENG1 loss -> X-bleed stays CLOSED, left manifold unpressurised", "[pneumatics]")
{
    // FCOM DSC-36-20: AUTO opens crossbleed ONLY when APU bleed valve is open.
    // Single engine loss does NOT auto-open X-bleed; crew must select OPEN manually.
    auto pneu = makeSystem();
    pneu.setXBleedSelector(XBleedMode::Auto);
    pneu.setEng1N2(0.f);   // ENG1 bleed lost
    pneu.setEng2N2(80.f);
    pneu.update(0.02f);

    CHECK_FALSE(pneu.isXBleedOpen());
    CHECK_THAT(pneu.manifold1PressurePsi(), WithinAbs(0.f, 0.1f));  // ENG1 lost, no X-bleed
    CHECK(pneu.manifold2PressurePsi() > 0.f);                        // ENG2 still feeds right
}

TEST_CASE("X-bleed CLOSED: ENG1 loss -> left manifold unpressurised", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setXBleedSelector(XBleedMode::Closed);
    pneu.setEng1N2(0.f);
    pneu.setEng2N2(80.f);
    pneu.update(0.02f);

    CHECK_FALSE(pneu.isXBleedOpen());
    CHECK_THAT(pneu.manifold1PressurePsi(), WithinAbs(0.f, 0.1f));
    CHECK(pneu.manifold2PressurePsi() > 0.f);
}

TEST_CASE("X-bleed OPEN: both manifolds from single ENG2 source", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setXBleedSelector(XBleedMode::Open);
    pneu.setEng1N2(0.f);
    pneu.setEng2N2(80.f);
    pneu.update(0.02f);

    CHECK(pneu.isXBleedOpen());
    CHECK(pneu.manifold1PressurePsi() > 0.f);
    CHECK(pneu.manifold2PressurePsi() > 0.f);
}

// ── APU bleed ────────────────────────────────────────────────────────────────

TEST_CASE("APU bleed: feeds left manifold; X-bleed AUTO opens to feed right", "[pneumatics]")
{
    // FCOM DSC-36-10-30: APU bleed available when N > 95%; DSC-36-20: X-bleed AUTO opens when APU bleed active
    auto pneu = makeSystem();
    pneu.setApuNPct(100.f);   // APU at full speed (> 95% threshold)
    tickNoEngines(pneu);

    CHECK(pneu.isApuBleedActive());
    CHECK(pneu.manifold1PressurePsi() > 0.f);  // APU feeds left directly
    CHECK(pneu.isXBleedOpen());                 // X-bleed AUTO opens when APU bleed active
    CHECK(pneu.manifold2PressurePsi() > 0.f);  // right via X-bleed
}

TEST_CASE("APU bleed: not available below 95% N (FCOM DSC-36-10-30)", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setApuNPct(90.f);   // below 95% threshold
    tickNoEngines(pneu);

    CHECK_FALSE(pneu.isApuBleedActive());
}

TEST_CASE("APU bleed switch OFF: no APU bleed even if N > 95%", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setApuBleedSwitch(false);
    pneu.setApuNPct(100.f);
    tickNoEngines(pneu);

    CHECK_FALSE(pneu.isApuBleedActive());
}

// ── Pack operation ────────────────────────────────────────────────────────────

TEST_CASE("Packs operating when manifold pressurised", "[pneumatics]")
{
    auto pneu = makeSystem();
    tickBothEngines(pneu);

    CHECK(pneu.pack1().isOperating());
    CHECK(pneu.pack2().isOperating());
    CHECK(pneu.pack1().flowKgS() > 0.f);
}

TEST_CASE("Pack switch OFF: pack not operating", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setPack1Switch(false);
    tickBothEngines(pneu);

    CHECK_FALSE(pneu.pack1().isOperating());
    CHECK(pneu.pack2().isOperating());
}

TEST_CASE("No bleed: packs not operating", "[pneumatics]")
{
    auto pneu = makeSystem();
    tickNoEngines(pneu);

    CHECK_FALSE(pneu.pack1().isOperating());
    CHECK_FALSE(pneu.pack2().isOperating());
}

TEST_CASE("Hi-flow: pack flow doubles", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setPackHiFlow(true);
    tickBothEngines(pneu);

    // Hi flow should produce higher mass flow than NORM
    float normFlow = 0.35f;
    CHECK(pneu.pack1().flowKgS() > normFlow);
}

// ── Anti-ice ─────────────────────────────────────────────────────────────────

TEST_CASE("WAI active when switch on and manifold available", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setWingAntiIce(true);
    tickBothEngines(pneu);

    CHECK(pneu.antiIce().isWaiActive());
}

TEST_CASE("WAI inactive when no bleed pressure", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setWingAntiIce(true);
    tickNoEngines(pneu);

    CHECK_FALSE(pneu.antiIce().isWaiActive());
}

TEST_CASE("EAI: each engine anti-ice independent", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setEng1AntiIce(true);
    pneu.setEng2AntiIce(false);
    tickBothEngines(pneu);

    CHECK(pneu.antiIce().isEai1Active());
    CHECK_FALSE(pneu.antiIce().isEai2Active());
}

// ── Pressurization ───────────────────────────────────────────────────────────

TEST_CASE("Pressurization: on ground, diff press = 0", "[pneumatics]")
{
    auto pneu = makeSystem();
    pneu.setOnGround(true);
    pneu.setAmbientPressHpa(1013.25f);
    tickBothEngines(pneu);

    CHECK_THAT(pneu.pressurization().diffPressPsi(), WithinAbs(0.f, 0.01f));
}

TEST_CASE("Pressurization: safety valve caps diff press at 8.35 PSI", "[pneumatics]")
{
    // Aircraft suddenly at FL390, cabin at sea level → immediate overpressure
    Pressurization press;
    press.init();

    float ambientHpa = 1013.25f * std::pow(1.f - 6.8756e-6f * 39000.f, 5.2561f);
    press.setAircraftAltFt(39000.f);
    press.setAmbientPressHpa(ambientHpa);
    press.setOnGround(false);
    press.setLandingElevationFt(0.f);

    press.update(0.02f);

    // Safety relief valve must prevent more than 8.35 PSI
    CHECK(press.diffPressPsi() <= 8.35f + 0.01f);
    CHECK(press.isSafetyValveOpen());
}

TEST_CASE("Pressurization: steady-state cruise, diff press converges to ~8.06 PSI", "[pneumatics]")
{
    // Cabin rate limit is 300 FPM; need ~27 min sim time to climb from 0 to 8000 ft.
    // Use dt=1.0s for speed: 1800 ticks = 30 min sim time, fast to execute.
    Pressurization press;
    press.init();

    float ambientHpa = 1013.25f * std::pow(1.f - 6.8756e-6f * 39000.f, 5.2561f);
    press.setAircraftAltFt(39000.f);
    press.setAmbientPressHpa(ambientHpa);
    press.setOnGround(false);
    press.setLandingElevationFt(0.f);

    for (int i = 0; i < 1800; ++i) press.update(1.0f);

    CHECK(press.diffPressPsi() > 7.5f);
    CHECK(press.diffPressPsi() <= 8.06f + 0.05f);
    CHECK(press.cabinAltFt() < 10000.f);
}

TEST_CASE("Pressurization: cabin alt stays below aircraft alt (pressurised)", "[pneumatics]")
{
    Pressurization press;
    press.init();

    float ambientHpa = 1013.25f * std::pow(1.f - 6.8756e-6f * 39000.f, 5.2561f);
    press.setAircraftAltFt(39000.f);
    press.setAmbientPressHpa(ambientHpa);
    press.setOnGround(false);
    press.setLandingElevationFt(0.f);

    for (int i = 0; i < 100; ++i) press.update(1.0f);

    CHECK(press.cabinAltFt() < 39000.f - 20000.f);
}

// ── DataRef publication ───────────────────────────────────────────────────────

TEST_CASE("Pneumatic datarefs reflect system state after update", "[pneumatics]")
{
    auto pneu = makeSystem();
    tickBothEngines(pneu);

    auto& dm = DataRefManager::instance();
    CHECK(dm.getCustomFloat("a320/systems/pneumatics/manifold1_psi") > 0.f);
    CHECK(dm.getCustomFloat("a320/systems/pneumatics/manifold2_psi") > 0.f);
    CHECK(dm.getCustomInt  ("a320/systems/pneumatics/pack1_on") == 1);
    CHECK(dm.getCustomInt  ("a320/systems/pneumatics/pack2_on") == 1);
}
