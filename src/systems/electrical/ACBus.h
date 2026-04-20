#pragma once
#include <string>

namespace a320 {

// One AC bus on the A320 electrical network.
// The bus is powered when at least one source feeds it via a closed contactor.
// Load shedding: SHED buses drop when supply is limited to a single generator.
class ACBus {
public:
    enum class ID {
        AC_BUS_1,
        AC_BUS_2,
        AC_ESS,
        AC_ESS_SHED,
    };

    explicit ACBus(ID id);

    // Called by ElectricalSystem to set bus state each tick
    void setPowered(bool powered, float voltageAC = 115.f, float freqHz = 400.f);

    bool  isPowered()   const { return m_powered; }
    float voltageAC()   const { return m_voltage; }
    float frequencyHz() const { return m_freq; }
    const char* label() const;

    ID id() const { return m_id; }

private:
    ID    m_id;
    bool  m_powered  = false;
    float m_voltage  = 0.f;
    float m_freq     = 0.f;
};

} // namespace a320
