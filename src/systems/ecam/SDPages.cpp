#include "SDPages.h"
#include "network/NetworkServer.h"
#include "network/Protocol.h"
#include "systems/electrical/ElectricalSystem.h"
#include "systems/pneumatics/PneumaticSystem.h"

namespace a320 {

void SDPages::sendELEC(const ElectricalSystem& elec, uint32_t simTimeMs, uint32_t& seqNum)
{
    proto::SDElecPage pkt{};
    pkt.hdr.version   = proto::PROTOCOL_VERSION;
    pkt.hdr.type      = static_cast<uint16_t>(proto::PacketType::SDPageData);
    pkt.hdr.seqNum    = seqNum++;
    pkt.hdr.simTimeMs = simTimeMs;

    // AC buses
    pkt.acBus1Powered    = elec.acBus1().isPowered()    ? 1 : 0;
    pkt.acBus1Voltage    = elec.acBus1().voltageAC();
    pkt.acBus1Freq       = elec.acBus1().frequencyHz();

    pkt.acBus2Powered    = elec.acBus2().isPowered()    ? 1 : 0;
    pkt.acBus2Voltage    = elec.acBus2().voltageAC();
    pkt.acBus2Freq       = elec.acBus2().frequencyHz();

    pkt.acEssPowered     = elec.acEss().isPowered()     ? 1 : 0;
    pkt.acEssVoltage     = elec.acEss().voltageAC();
    pkt.acEssFreq        = elec.acEss().frequencyHz();

    pkt.acEssShedPowered = elec.acEssShed().isPowered() ? 1 : 0;

    // DC buses
    pkt.dcBus1Powered    = elec.dcBus1().isPowered()    ? 1 : 0;
    pkt.dcBus1Voltage    = elec.dcBus1().voltage();

    pkt.dcBus2Powered    = elec.dcBus2().isPowered()    ? 1 : 0;
    pkt.dcBus2Voltage    = elec.dcBus2().voltage();

    pkt.dcEssPowered     = elec.dcEss().isPowered()     ? 1 : 0;
    pkt.dcEssVoltage     = elec.dcEss().voltage();

    pkt.dcBatPowered     = elec.dcBat().isPowered()     ? 1 : 0;
    pkt.dcBatVoltage     = elec.dcBat().voltage();

    pkt.hotBus1Powered   = elec.hotBus1().isPowered()   ? 1 : 0;
    pkt.hotBus2Powered   = elec.hotBus2().isPowered()   ? 1 : 0;

    // Generators: IDG1, IDG2, APU GEN, EXT PWR
    const Generator* gens[4] = { &elec.idg1(), &elec.idg2(), &elec.apuGen(), &elec.extPwr() };
    for (int i = 0; i < 4; ++i) {
        pkt.genOnline[i]  = gens[i]->isOnline()  ? 1 : 0;
        pkt.genVoltage[i] = gens[i]->voltageAC();
        pkt.genFreq[i]    = gens[i]->frequencyHz();
        pkt.genLoadPct[i] = gens[i]->loadPct();
    }

    // Batteries
    pkt.bat1Voltage    = elec.bat1().voltage();
    pkt.bat2Voltage    = elec.bat2().voltage();
    pkt.bat1ChargePct  = elec.bat1().chargePct();
    pkt.bat2ChargePct  = elec.bat2().chargePct();

    m_net.pushDisplayData(&pkt, sizeof(pkt));
}

void SDPages::sendBLEED(const PneumaticSystem& pneu, uint32_t simTimeMs, uint32_t& seqNum)
{
    proto::SDBleedPage pkt{};
    pkt.hdr.version   = proto::PROTOCOL_VERSION;
    pkt.hdr.type      = static_cast<uint16_t>(proto::PacketType::SDBleedPage);
    pkt.hdr.seqNum    = seqNum++;
    pkt.hdr.simTimeMs = simTimeMs;

    pkt.bleedOn[0]       = pneu.bleed1().isFlowAvailable() ? 1 : 0;
    pkt.bleedOn[1]       = pneu.bleed2().isFlowAvailable() ? 1 : 0;
    pkt.bleedPressPsi[0] = pneu.bleed1().pressurePsi();
    pkt.bleedPressPsi[1] = pneu.bleed2().pressurePsi();
    pkt.bleedTempC[0]    = pneu.bleed1().bleedTempC();
    pkt.bleedTempC[1]    = pneu.bleed2().bleedTempC();

    pkt.apuBleedOn      = pneu.isApuBleedActive() ? 1 : 0;
    pkt.apuBleedPressPsi = pneu.isApuBleedActive() ? 30.f : 0.f;

    pkt.xBleedOpen    = pneu.isXBleedOpen() ? 1 : 0;
    pkt.manifold1Psi  = pneu.manifold1PressurePsi();
    pkt.manifold2Psi  = pneu.manifold2PressurePsi();

    pkt.wingAntiIce   = pneu.antiIce().isWaiActive()  ? 1 : 0;
    pkt.eng1AntiIce   = pneu.antiIce().isEai1Active() ? 1 : 0;
    pkt.eng2AntiIce   = pneu.antiIce().isEai2Active() ? 1 : 0;

    pkt.pack1On           = pneu.pack1().isOperating() ? 1 : 0;
    pkt.pack2On           = pneu.pack2().isOperating() ? 1 : 0;
    pkt.pack1InletTempC   = pneu.manifold1PressurePsi() > 0.f ? pneu.bleed1().bleedTempC() : 0.f;
    pkt.pack2InletTempC   = pneu.manifold2PressurePsi() > 0.f ? pneu.bleed2().bleedTempC() : 0.f;

    m_net.pushDisplayData(&pkt, sizeof(pkt));
}

void SDPages::sendPRESS(const PneumaticSystem& pneu, uint32_t simTimeMs, uint32_t& seqNum)
{
    proto::SDPressPage pkt{};
    pkt.hdr.version   = proto::PROTOCOL_VERSION;
    pkt.hdr.type      = static_cast<uint16_t>(proto::PacketType::SDPressPage);
    pkt.hdr.seqNum    = seqNum++;
    pkt.hdr.simTimeMs = simTimeMs;

    const auto& press = pneu.pressurization();
    pkt.cabinAltFt       = press.cabinAltFt();
    pkt.diffPressPsi     = press.diffPressPsi();
    pkt.cabinVsFpm       = press.cabinVsFpm();
    pkt.outflowValvePct  = press.outflowValvePct();
    pkt.safetyValveOpen  = press.isSafetyValveOpen() ? 1 : 0;
    pkt.excessCabinAlt   = press.isExcessCabinAlt()  ? 1 : 0;
    pkt.excessDiffPress  = press.isExcessDiffPress()  ? 1 : 0;
    pkt.landingElevFt    = 0.f;  // TODO: feed from FMGC

    m_net.pushDisplayData(&pkt, sizeof(pkt));
}

void SDPages::sendCOND(const PneumaticSystem& pneu, uint32_t simTimeMs, uint32_t& seqNum)
{
    proto::SDCondPage pkt{};
    pkt.hdr.version   = proto::PROTOCOL_VERSION;
    pkt.hdr.type      = static_cast<uint16_t>(proto::PacketType::SDCondPage);
    pkt.hdr.seqNum    = seqNum++;
    pkt.hdr.simTimeMs = simTimeMs;

    pkt.pack1OutletTempC = pneu.pack1().outletTempC();
    pkt.pack2OutletTempC = pneu.pack2().outletTempC();
    pkt.pack1FlowKgS     = pneu.pack1().flowKgS();
    pkt.pack2FlowKgS     = pneu.pack2().flowKgS();

    // Zone temperatures — stub at 24°C until cabin zone model is implemented
    for (int i = 0; i < 4; ++i) {
        pkt.zoneTempC[i]    = 24.f;
        pkt.zoneTempSetC[i] = 24.f;
    }
    pkt.hotAirOn = 1;  // trim air on in normal ops

    m_net.pushDisplayData(&pkt, sizeof(pkt));
}

} // namespace a320
