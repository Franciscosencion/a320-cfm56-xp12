#include "SDPages.h"
#include "network/NetworkServer.h"
#include "network/Protocol.h"
#include "systems/electrical/ElectricalSystem.h"

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

} // namespace a320
