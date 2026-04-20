#include "Generator.h"
#include <algorithm>
#include <cmath>

namespace a320 {

Generator::Generator(GeneratorType type, int index, std::string name)
    : m_type(type), m_index(index), m_name(std::move(name))
{}

void Generator::update(float dt)
{
    switch (m_type) {
        case GeneratorType::IDG:       updateIDG(dt);   break;
        case GeneratorType::APU:       updateAPU(dt);   break;
        case GeneratorType::ExternalPower: updateExtPwr(); break;
    }

    m_loadPct = (RATED_KVA > 0.f) ? std::min(m_loadKva / RATED_KVA * 100.f, 100.f) : 0.f;
}

void Generator::updateIDG(float /*dt*/)
{
    if (!m_driveConnected || m_n2Pct < IDG_MIN_N2) {
        m_voltageAC = 0.f;
        m_freqHz    = 0.f;
        m_lineContactorClosed = false;
        return;
    }

    // IDG output is constant-frequency (CSD keeps 400 Hz above ~55% N2)
    m_voltageAC = NOMINAL_V;
    m_freqHz    = NOMINAL_HZ;

    // Line contactor closes when switch is ON and no fault
    m_lineContactorClosed = m_switchOn && !m_fault;
}

void Generator::updateAPU(float /*dt*/)
{
    if (!m_externalAvailable) {
        m_voltageAC = 0.f;
        m_freqHz    = 0.f;
        m_lineContactorClosed = false;
        return;
    }

    m_voltageAC = NOMINAL_V;
    m_freqHz    = NOMINAL_HZ;
    // APU GEN contactor: AUTO — closes automatically when APU is available
    // and no other source is powering that bus (logic in ElectricalSystem).
    // Here we just set the raw availability; bus tie logic decides the contactor.
    m_lineContactorClosed = m_switchOn && !m_fault;
}

void Generator::updateExtPwr()
{
    if (!m_externalAvailable) {
        m_voltageAC = 0.f;
        m_freqHz    = 0.f;
        m_lineContactorClosed = false;
        return;
    }

    m_voltageAC = NOMINAL_V;
    m_freqHz    = NOMINAL_HZ;
    m_lineContactorClosed = m_switchOn && !m_fault;
}

void Generator::setDriveConnected(bool connected) { m_driveConnected = connected; }
void Generator::setSwitch(bool on)                { m_switchOn = on; }
void Generator::setN2(float n2Pct)                { m_n2Pct = n2Pct; }
void Generator::setAvailable(bool avail)          { m_externalAvailable = avail; }

void Generator::addLoadKva(float kva) { m_loadKva += kva; }
void Generator::clearLoad()           { m_loadKva = 0.f; }

} // namespace a320
