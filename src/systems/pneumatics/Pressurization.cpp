#include "Pressurization.h"
#include <algorithm>
#include <cmath>

namespace a320 {

// ISA standard atmosphere (troposphere, altitude in feet).
// P(h_ft) = 1013.25 * (1 - 6.8756e-6 * h_ft)^5.2561
// The coefficient 6.8756e-6 ft^-1 = (L/T0) * 0.3048  where L=0.0065 K/m, T0=288.15 K.
float Pressurization::pressHpaFromAltFt(float altFt) const
{
    return 1013.25f * std::pow(std::max(1.f - 6.8756e-6f * altFt, 0.01f), 5.2561f);
}

float Pressurization::altFtFromPressHpa(float pressHpa) const
{
    float ratio = pressHpa / 1013.25f;
    return (1.f - std::pow(std::max(ratio, 0.01f), 1.f / 5.2561f)) / 6.8756e-6f;
}

void Pressurization::init()
{
    m_cabinAltFt    = 0.f;
    m_cabinPressHpa = 1013.25f;
    m_outflowValvePct = 100.f;
}

void Pressurization::update(float dt)
{
    m_prevCabinAltFt = m_cabinAltFt;

    if (m_onGround) {
        // On ground: outflow fully open, cabin pressure = ambient
        m_cabinPressHpa   = m_ambientPressHpa;
        m_outflowValvePct = 100.f;
        m_cabinAltFt      = altFtFromPressHpa(m_cabinPressHpa);
        m_diffPressPsi    = 0.f;
        m_safetyValveOpen = false;
        m_cabinVsFpm      = 0.f;
        return;
    }

    if (!m_autoMode) {
        // Manual: pilot controls outflow valve directly
        // Pressure change proportional to valve position (simple model)
        // Open valve → pressure equalises with ambient; closed → holds current
        float leak_rate = m_manualOutflowPct / 100.f * 2.f;  // hPa/s max leak
        m_cabinPressHpa = m_cabinPressHpa - leak_rate * dt;
        m_cabinPressHpa = std::max(m_cabinPressHpa, m_ambientPressHpa);
        m_outflowValvePct = m_manualOutflowPct;
    } else {
        // AUTO mode: CPCS targets a cabin altitude schedule
        // Target cabin pressure = ambient + min(max_diff, sched_diff)
        float max_cabin_press = m_ambientPressHpa + MAX_DIFF_PRESS_PSI * HPA_PER_PSI;

        // During cruise/climb: target cabin alt converges toward 6000–7000 ft
        // During descent: converge to landing elevation
        float target_cabin_press;
        if (m_aircraftAltFt > m_landingElevFt + 500.f) {
            // Airborne: target = ambient + 8.06 PSI diff (full pressurisation)
            target_cabin_press = std::min(max_cabin_press, m_ambientPressHpa + MAX_DIFF_PRESS_PSI * HPA_PER_PSI);
            // But never exceed field pressure (don't go below landing elevation)
            float field_press  = pressHpaFromAltFt(m_landingElevFt);
            target_cabin_press = std::min(target_cabin_press, field_press);
        } else {
            // Near field elevation: equalise to landing elevation
            target_cabin_press = pressHpaFromAltFt(m_landingElevFt);
        }

        // Rate-limit cabin pressure change to keep cabin VS within limits
        float target_cabin_alt  = altFtFromPressHpa(target_cabin_press);
        float cabin_alt_error   = target_cabin_alt - m_cabinAltFt;

        // Max allowed cabin altitude rate of change per tick
        float max_climb_ft  =  MAX_CABIN_VS_FPM / 60.f * dt;
        float max_desc_ft   = -MAX_CABIN_DESC_FPM / 60.f * dt;
        float clamped_delta = std::max(max_desc_ft, std::min(max_climb_ft, cabin_alt_error));

        m_cabinAltFt += clamped_delta;
        m_cabinPressHpa = pressHpaFromAltFt(m_cabinAltFt);
        m_cabinPressHpa = std::max(m_cabinPressHpa, m_ambientPressHpa); // never below ambient

        // Outflow valve: P-controller on pressure error
        float press_error = m_cabinPressHpa - (m_ambientPressHpa + MAX_DIFF_PRESS_PSI * HPA_PER_PSI * 0.9f);
        m_outflowValvePct = 50.f + press_error * 0.5f;
        m_outflowValvePct = std::max(0.f, std::min(100.f, m_outflowValvePct));
    }

    // Differential pressure
    m_diffPressPsi = (m_cabinPressHpa - m_ambientPressHpa) / HPA_PER_PSI;
    m_diffPressPsi = std::max(0.f, m_diffPressPsi);

    // Safety relief valve: opens at 8.35 PSI to vent excess pressure
    m_safetyValveOpen = (m_diffPressPsi >= SAFETY_RELIEF_PSI);
    if (m_safetyValveOpen) {
        m_cabinPressHpa = m_ambientPressHpa + SAFETY_RELIEF_PSI * HPA_PER_PSI;
        m_diffPressPsi  = SAFETY_RELIEF_PSI;
        m_cabinAltFt    = altFtFromPressHpa(m_cabinPressHpa);
    }

    // Cabin vertical speed (FPM)
    m_cabinVsFpm = (m_cabinAltFt - m_prevCabinAltFt) / dt * 60.f;
}

void Pressurization::setAircraftAltFt(float alt)       { m_aircraftAltFt   = alt; }
void Pressurization::setAmbientPressHpa(float hpa)     { m_ambientPressHpa = hpa; }
void Pressurization::setOnGround(bool onGround)        { m_onGround        = onGround; }
void Pressurization::setLandingElevationFt(float elev) { m_landingElevFt   = elev; }
void Pressurization::setAutoMode(bool autoMode)        { m_autoMode        = autoMode; }
void Pressurization::setManualOutflowPct(float pct)    { m_manualOutflowPct = std::max(0.f, std::min(100.f, pct)); }

} // namespace a320
