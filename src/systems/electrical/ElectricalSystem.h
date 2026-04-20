#pragma once
#include "Generator.h"
#include "Battery.h"
#include "ACBus.h"
#include "DCBus.h"
#include "ElectricalEvents.h"
#include <array>

namespace a320 {

class DataRefManager;

// A320 Electrical System
// AC topology (simplified FCOM schema):
//
//   EXT PWR ──┐
//   APU GEN ──┤──[AGC/EPC]──┬── AC BUS 1 ──[BTB1]──┐
//   IDG 1 ───┤──[GLC1]──────┘                        │
//                                                      ├── (tie)
//   IDG 2 ───┐──[GLC2]──────┐                        │
//   APU GEN ──┤──[AGC/EPC]──┴── AC BUS 2 ──[BTB2]──┘
//
//   AC BUS 1 or 2 ──[ACESSB CONT]── AC ESS BUS ──[AC ESS SHED CONT]── AC ESS SHED
//
// DC topology:
//   AC BUS 1 ──[TR1]── DC BUS 1 ──[BTC1]──┐
//   AC BUS 2 ──[TR2]── DC BUS 2 ──[BTC2]──┴── DC BAT BUS ──[BPCU]── DC ESS BUS
//   AC ESS  ──[ESS TR]── DC ESS BUS (alternate feed)
//   BAT 1 ──[BAT1 CONT]── DC BAT BUS / HOT BUS 1
//   BAT 2 ──[BAT2 CONT]── DC BAT BUS / HOT BUS 2
//
class ElectricalSystem {
public:
    ElectricalSystem();

    void init();
    void update(float dt);

    // ── Source controls (called from overhead panel events) ────────────────
    void setGen1Switch(bool on);
    void setGen2Switch(bool on);
    void setApuGenSwitch(bool on);
    void setExtPwrSwitch(bool on);
    void setBat1Switch(bool on);
    void setBat2Switch(bool on);
    void setAcBusTieSwitch(bool on);   // BUS TIE pb (AUTO=on)
    void setAcEssFeedAltn(bool altn);  // AC ESS FEED pb (NORM=false, ALTN=true)

    // ── Engine inputs (fed from FADEC/engine model each tick) ─────────────
    void setEng1N2(float n2Pct);
    void setEng2N2(float n2Pct);
    void setApuAvailable(bool avail);
    void setExtPwrAvailable(bool avail);

    // ── State queries ──────────────────────────────────────────────────────
    const ACBus& acBus1()    const { return m_acBus1; }
    const ACBus& acBus2()    const { return m_acBus2; }
    const ACBus& acEss()     const { return m_acEss; }
    const ACBus& acEssShed() const { return m_acEssShed; }

    const DCBus& dcBus1()    const { return m_dcBus1; }
    const DCBus& dcBus2()    const { return m_dcBus2; }
    const DCBus& dcEss()     const { return m_dcEss; }
    const DCBus& dcBat()     const { return m_dcBat; }
    const DCBus& hotBus1()   const { return m_hotBus1; }
    const DCBus& hotBus2()   const { return m_hotBus2; }

    const Generator& idg1()    const { return m_idg1; }
    const Generator& idg2()    const { return m_idg2; }
    const Generator& apuGen()  const { return m_apuGen; }
    const Generator& extPwr()  const { return m_extPwr; }
    const Battery&   bat1()    const { return m_bat1; }
    const Battery&   bat2()    const { return m_bat2; }

    // Convenience: number of online AC sources (0–4)
    int  onlineACSourceCount() const;
    bool isAnythingPowered()   const;

private:
    void updateACNetwork();
    void updateDCNetwork();
    void updateBatteries(float dt);
    void publishEvents();
    void registerDataRefs();

    // Sources
    Generator m_idg1   { GeneratorType::IDG,           0, "GEN 1"   };
    Generator m_idg2   { GeneratorType::IDG,           1, "GEN 2"   };
    Generator m_apuGen { GeneratorType::APU,           2, "APU GEN" };
    Generator m_extPwr { GeneratorType::ExternalPower, 3, "EXT PWR" };
    Battery   m_bat1   { 0 };
    Battery   m_bat2   { 1 };

    // AC buses
    ACBus m_acBus1    { ACBus::ID::AC_BUS_1    };
    ACBus m_acBus2    { ACBus::ID::AC_BUS_2    };
    ACBus m_acEss     { ACBus::ID::AC_ESS      };
    ACBus m_acEssShed { ACBus::ID::AC_ESS_SHED };

    // DC buses
    DCBus m_dcBus1    { DCBus::ID::DC_BUS_1    };
    DCBus m_dcBus2    { DCBus::ID::DC_BUS_2    };
    DCBus m_dcEss     { DCBus::ID::DC_ESS      };
    DCBus m_dcBat     { DCBus::ID::DC_BAT      };
    DCBus m_hotBus1   { DCBus::ID::DC_HOT_BUS_1 };
    DCBus m_hotBus2   { DCBus::ID::DC_HOT_BUS_2 };

    // Contactor / logic state
    bool m_busTieAuto    = true;   // BUS TIE pb AUTO
    bool m_acEssFeedAltn = false;  // AC ESS FEED pb NORM

    // For event diffing
    bool m_prevAcBus1Powered    = false;
    bool m_prevAcBus2Powered    = false;
    bool m_prevAcEssPowered     = false;
    bool m_prevAcEssShedPowered = false;
};

} // namespace a320
