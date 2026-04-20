#pragma once
#include <cstdint>

// Wire protocol v1.
// All multi-byte integers are little-endian.
// Transport:
//   UDP port 45010 — display data (plugin → client, lossy OK)
//   TCP port 45011 — hardware input + MCDU keypresses (client → plugin, reliable)
//
// Each UDP datagram begins with a PacketHeader followed by one payload struct.
// Each TCP message is length-prefixed: uint16_t length, then PacketHeader + payload.

namespace a320::proto {

static constexpr uint16_t PROTOCOL_VERSION = 1;

// ── Packet type IDs ────────────────────────────────────────────────────────
enum class PacketType : uint16_t {
    // Display data (UDP, plugin → client)
    Heartbeat           = 0x0001,
    PFDData             = 0x0010,
    NDData              = 0x0011,
    EWDData             = 0x0020,
    SDPageData          = 0x0021,
    MCDUDisplay         = 0x0030,
    OverheadState       = 0x0040,
    PedestalState       = 0x0041,

    // Hardware input (TCP, client → plugin)
    FCUKnobEvent        = 0x0100,
    ButtonEvent         = 0x0101,
    AxisEvent           = 0x0102,
    MCDUKeyEvent        = 0x0110,
};

#pragma pack(push, 1)

struct PacketHeader {
    uint16_t version;   // PROTOCOL_VERSION
    uint16_t type;      // PacketType
    uint32_t seqNum;    // monotonically increasing per sender
    uint32_t simTimeMs; // sim time in ms for correlation
};

// ── Heartbeat ─────────────────────────────────────────────────────────────
struct Heartbeat {
    PacketHeader hdr;
    uint32_t     uptimeMs;
};

// ── PFD Data ──────────────────────────────────────────────────────────────
struct PFDData {
    PacketHeader hdr;

    // Attitude
    float   pitch;          // deg, nose-up positive
    float   roll;           // deg, right-wing-down positive
    float   sideslip;       // deg

    // Speed tape
    float   cas;            // kt
    float   tas;            // kt
    float   mach;
    float   vls;            // kt — lowest selectable speed
    float   vAlphaProt;     // kt
    float   vAlphaMax;      // kt
    float   vMax;           // kt
    float   vTrend;         // kt/s (10-sec projection)
    float   selectedSpeedKt;
    uint8_t speedManagedDot; // 1 = managed (filled dot)

    // Alt tape
    float   altitudeFt;
    float   baroSettingHpa;
    uint8_t baroIsStd;      // 1 = STD
    float   selectedAltFt;
    float   altTrendFpm;    // ft/min projected

    // VSI
    float   verticalSpeedFpm;
    float   selectedVsFpm;

    // Heading / track
    float   heading;        // deg mag
    float   track;          // deg mag
    float   selectedHdg;
    uint8_t hdgOrTrkMode;   // 0=HDG, 1=TRK

    // FD bars (valid only when FD active)
    uint8_t fdActive;
    float   fdPitch;        // deg command
    float   fdRoll;         // deg command

    // Mode annunciations — see GuidanceModes.h for enum values
    uint8_t armedLateralMode;
    uint8_t activeLateralMode;
    uint8_t armedVerticalMode;
    uint8_t activeVerticalMode;
    uint8_t athrMode;
    uint8_t apEngaged;      // bitmask: bit0=AP1, bit1=AP2
    uint8_t fdEngaged;      // bitmask: bit0=FD1, bit1=FD2
    uint8_t athrArmed;

    // ILS deviation (valid when LOC/GS captured or armed)
    float   locDevDots;     // ±2 full scale
    float   gsDevDots;      // ±2 full scale
    uint8_t ilsAvailable;
};

// ── ND Data (minimal — full map data sent separately) ─────────────────────
struct NDData {
    PacketHeader hdr;

    uint8_t  mode;          // 0=ROSE ILS, 1=ROSE VOR, 2=ROSE NAV, 3=ARC, 4=PLAN
    uint16_t rangeNm;
    float    heading;       // current aircraft heading
    float    track;

    // Bearing pointers (1 and 2)
    uint8_t  bearing1Source; // 0=OFF, 1=ADF1, 2=VOR1
    float    bearing1Deg;
    float    bearing1DistNm;
    uint8_t  bearing2Source;
    float    bearing2Deg;
    float    bearing2DistNm;

    // Selected course
    float    selectedCourseDeg;

    // Next waypoint
    char     nextWptId[6];
    float    nextWptBearingDeg;
    float    nextWptDistNm;
    float    nextWptEtaMinutes;

    // Wind
    float    windDirDeg;
    float    windKt;
};

// ── EWD Engine data ────────────────────────────────────────────────────────
struct EWDData {
    PacketHeader hdr;

    // Engines [0]=ENG1 [1]=ENG2
    float    n1[2];         // %
    float    n1Target[2];   // % (A/THR commanded)
    float    n2[2];         // %
    float    egt[2];        // °C
    float    ff[2];         // kg/h
    uint8_t  engState[2];   // 0=off, 1=starting, 2=avail, 3=failed
    uint8_t  revUnlocked[2];// thrust reverser

    // Slat/flap handle position 0/1/2/3/4/5 (0=retracted, 5=FULL)
    uint8_t  flapLeverPos;
    float    flapAngleDeg;
    float    slatAngleDeg;
    uint8_t  speedBrakeArmed;
    float    speedBrakePos; // 0..1

    // Warning/caution title area (top two lines of EWD)
    char     warningLine1[40];
    char     warningLine2[40];
};

// ── Hardware input events (TCP client → plugin) ────────────────────────────
enum class FCUKnob : uint8_t {
    Speed = 0, Heading, Altitude, VsFpa
};
enum class KnobAction : uint8_t {
    RotateCW=0, RotateCCW, Push, Pull
};
struct FCUKnobEvent {
    PacketHeader hdr;
    uint8_t      knob;   // FCUKnob
    uint8_t      action; // KnobAction
};

struct ButtonEvent {
    PacketHeader hdr;
    uint16_t     buttonId;  // see hardware_map.h for IDs
    uint8_t      pressed;   // 1=down, 0=up
};

struct AxisEvent {
    PacketHeader hdr;
    uint8_t      axisId;
    float        value;     // normalised 0..1
};

struct MCDUKeyEvent {
    PacketHeader hdr;
    uint8_t      mcduSide;  // 0=CAPT, 1=FO
    uint8_t      keyCode;   // see MCDUData.h
};

#pragma pack(pop)

} // namespace a320::proto
