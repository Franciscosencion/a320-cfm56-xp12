#include "ACBus.h"

namespace a320 {

ACBus::ACBus(ID id) : m_id(id) {}

void ACBus::setPowered(bool powered, float voltageAC, float freqHz)
{
    m_powered = powered;
    m_voltage = powered ? voltageAC : 0.f;
    m_freq    = powered ? freqHz    : 0.f;
}

const char* ACBus::label() const
{
    switch (m_id) {
        case ID::AC_BUS_1:    return "AC BUS 1";
        case ID::AC_BUS_2:    return "AC BUS 2";
        case ID::AC_ESS:      return "AC ESS BUS";
        case ID::AC_ESS_SHED: return "AC ESS SHED";
    }
    return "?";
}

} // namespace a320
