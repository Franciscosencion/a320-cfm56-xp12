#pragma once

namespace a320 {

class DCBus {
public:
    enum class ID {
        DC_BUS_1,
        DC_BUS_2,
        DC_ESS,
        DC_BAT,        // battery bus (always powered when any battery has charge)
        DC_HOT_BUS_1,  // directly on BAT 1 — always live
        DC_HOT_BUS_2,  // directly on BAT 2 — always live
    };

    explicit DCBus(ID id);

    void setPowered(bool powered, float voltageDC = 28.f);

    bool  isPowered() const { return m_powered; }
    float voltage()   const { return m_voltage; }
    const char* label() const;
    ID id() const { return m_id; }

private:
    ID    m_id;
    bool  m_powered = false;
    float m_voltage = 0.f;
};

} // namespace a320
