#pragma once

namespace a320 {

// Published on EventBus when bus power state changes
struct BusPoweredEvent {
    const char* busName; // e.g. "AC_BUS_1"
    bool        powered;
};

// Published when a generator comes online or drops off
struct GeneratorStateEvent {
    int  index;   // 0=IDG1, 1=IDG2, 2=APU_GEN, 3=EXT_PWR
    bool online;
};

// Published when a battery contactor opens/closes
struct BatteryContactorEvent {
    int  index;   // 0=BAT1, 1=BAT2
    bool closed;
};

} // namespace a320
