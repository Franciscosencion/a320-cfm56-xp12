#pragma once
#include "network/Protocol.h"

namespace a320 {

class ElectricalSystem;
class NetworkServer;

// Aggregates system state into SD page packets and pushes them to the network.
// One method per SD page; called from the main electrical (or ECAM) system tick.
class SDPages {
public:
    SDPages(NetworkServer& net) : m_net(net) {}

    // Build and send the ELEC SD page packet
    void sendELEC(const ElectricalSystem& elec, uint32_t simTimeMs, uint32_t& seqNum);

private:
    NetworkServer& m_net;
};

} // namespace a320
