#pragma once
#include "network/Protocol.h"

namespace a320 {

class ElectricalSystem;
class PneumaticSystem;
class NetworkServer;

// Aggregates system state into SD page packets and pushes them to the network.
// One method per SD page; called from the main system tick.
class SDPages {
public:
    explicit SDPages(NetworkServer& net) : m_net(net) {}

    void sendELEC (const ElectricalSystem&  elec,  uint32_t simTimeMs, uint32_t& seqNum);
    void sendBLEED(const PneumaticSystem&   pneu,  uint32_t simTimeMs, uint32_t& seqNum);
    void sendPRESS(const PneumaticSystem&   pneu,  uint32_t simTimeMs, uint32_t& seqNum);
    void sendCOND (const PneumaticSystem&   pneu,  uint32_t simTimeMs, uint32_t& seqNum);

private:
    NetworkServer& m_net;
};

} // namespace a320
