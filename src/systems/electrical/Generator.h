#pragma once
#include <string>

namespace a320 {

enum class GeneratorType { IDG, APU, ExternalPower };

// Models one AC power source: IDG 1/2, APU GEN, or external power.
// The generator is "online" when its line contactor (GLC/AGC/EPC) is closed
// AND the source is healthy (voltage/frequency in limits).
class Generator {
public:
    Generator(GeneratorType type, int index, std::string name);

    // Called every 50 Hz tick
    void update(float dt);

    // Controls
    void setDriveConnected(bool connected); // IDG drive disconnect
    void setSwitch(bool on);                // GEN pb ON/OFF or EXT PWR pb
    void setN2(float n2Pct);               // engine N2 input (IDG only)
    void setAvailable(bool avail);          // APU/EXT: external "ready" signal

    // State queries
    bool  isOnline()    const { return m_lineContactorClosed; }
    bool  isFault()     const { return m_fault; }
    float voltageAC()   const { return m_voltageAC; }
    float frequencyHz() const { return m_freqHz; }
    float loadPct()     const { return m_loadPct; }

    void  addLoadKva(float kva);
    void  clearLoad();

    const std::string& name() const { return m_name; }

private:
    void updateIDG(float dt);
    void updateAPU(float dt);
    void updateExtPwr();

    GeneratorType m_type;
    int           m_index;
    std::string   m_name;

    bool  m_switchOn          = false;
    bool  m_driveConnected    = true;
    bool  m_externalAvailable = false;
    float m_n2Pct             = 0.f;

    float m_voltageAC         = 0.f;
    float m_freqHz            = 0.f;
    float m_loadKva           = 0.f;
    float m_loadPct           = 0.f;

    bool  m_lineContactorClosed = false;
    bool  m_fault               = false;

    static constexpr float RATED_KVA     = 90.f;   // A320 IDG rating
    static constexpr float NOMINAL_V     = 115.f;
    static constexpr float NOMINAL_HZ    = 400.f;
    static constexpr float IDG_MIN_N2    = 55.f;   // min N2 % for IDG to produce power
};

} // namespace a320
