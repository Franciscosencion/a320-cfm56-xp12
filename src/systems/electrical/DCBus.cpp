#include "DCBus.h"

namespace a320 {

DCBus::DCBus(ID id) : m_id(id) {}

void DCBus::setPowered(bool powered, float voltageDC)
{
    m_powered = powered;
    m_voltage = powered ? voltageDC : 0.f;
}

const char* DCBus::label() const
{
    switch (m_id) {
        case ID::DC_BUS_1:    return "DC BUS 1";
        case ID::DC_BUS_2:    return "DC BUS 2";
        case ID::DC_ESS:      return "DC ESS BUS";
        case ID::DC_BAT:      return "DC BAT BUS";
        case ID::DC_HOT_BUS_1: return "HOT BUS 1";
        case ID::DC_HOT_BUS_2: return "HOT BUS 2";
    }
    return "?";
}

} // namespace a320
