# Architecture

## Overview

A C++17 X-Plane 12 plugin (`XPLM SDK 4.3.0`) that simulates all Airbus A320 CFM56 aircraft systems. No 3D cockpit is required — all displays are driven by external hardware over a network protocol.

## Threading Model

| Thread | Responsibilities |
|---|---|
| X-Plane flight loop (pre-FM, 50 Hz) | All systems simulation — FADEC, FBW, ELEC, HYD, PNEU, FUEL, ECAM |
| X-Plane flight loop (post-FM, per-frame) | FBW control surface writes, engine dataref overrides |
| Network IO thread (Asio) | UDP broadcast of display data, TCP accept/receive of hardware events |

The `EventBus` and all system state are accessed only from the flight loop thread. The network thread communicates via a lock-free ring buffer (outbound) and a mutex-protected queue (inbound).

## System Execution Order (pre-FM tick)

1. ADIRS (provides air data to all consumers)
2. Electrical (provides bus state to all powered systems)
3. Pneumatics (depends on electrical for bleed valve actuation)
4. Fuel (depends on electrical for pump control)
5. Hydraulics (depends on electrical for electric pumps)
6. APU
7. FADEC / Engines (depends on electrical, fuel, pneumatics)
8. Landing Gear / Brakes / NWS (depends on hydraulics)
9. FBW (depends on ADIRS, FADEC)
10. Auto-Flight / FCU (depends on FBW, ADIRS)
11. FMGC (depends on auto-flight, ADIRS, radio)
12. ECAM (aggregates all system states → warnings)
13. DisplayBridge (serializes state → network outbound ring)

## DataRef Naming Convention

All custom plugin datarefs follow: `a320/systems/<subsystem>/<parameter>`

Examples:
- `a320/systems/electrical/ac_bus1_powered`
- `a320/systems/fadec/eng1_n1`
- `a320/systems/fbw/active_law`

## Network Protocol

See `docs/network_protocol.md`.

- **UDP 45010**: display data (plugin → clients), high-frequency, lossy-tolerant
- **TCP 45011**: hardware events + MCDU keypresses (clients → plugin), reliable

## Override Strategy

### FADEC
Set `sim/operation/override/override_engines = 1` on enable. Write N1/EGT/FF datarefs every post-FM tick. Plugin computes thrust from CFM56-5B polynomial tables; X-Plane uses the written values.

### FBW
Set `sim/operation/override/override_joystick = 1` on enable. Write `sim/joystick/yoke_pitch_ratio`, `yoke_roll_ratio`, `yoke_heading_ratio` every post-FM tick. X-Plane's aerodynamics model still runs; FBW shapes the control commands.
