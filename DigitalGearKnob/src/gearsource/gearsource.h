#pragma once

#include <cstdint>

#include "can/can.h"
#include "geardecode/gearratio.h"

//=========================================================================
// GEAR SOURCE — CAN snapshot -> ratio estimator -> gear (v2 glue)
//=========================================================================
// The pluggable detection source behind the unchanged gears.h API (spec
// gear-source: pluggable detection source). The BNO085 path is removed in
// Phase 4; this module is the only active source: the CAN RX task fills the
// snapshot, gear_source_poll() feeds the pure ratio estimator, and gears.h
// consumers keep working untouched (design D8, gear-source glue).

// Pure decision seam (native-testable — no Arduino/HAL):
// Maps the CAN bus state + snapshot freshness to ONE estimator call and
// returns GearPosition | -1:
//   -1      bus offline (D8), snapshot not configured (D9), or the estimator
//           rejects its config — never a silently stale gear
//   GEAR_N  standstill / neutral / clutch / stale inputs (estimator internals)
//   GEAR_R..GEAR_5  matched gear
inline int8_t gear_source_decide(
    bool can_online,
    const CanSnapshot &snap,
    RatioEstimator &estimator,
    uint32_t now_ms)
{
    // D8: offline -> explicit unknown, never a stale gear on the display.
    if (!can_online)
    {
        return -1;
    }

    // D9: the signal layout must be sniff-confirmed (signals_configured)
    // before the source goes live; until then the snapshot is invalid.
    if (!snap.valid)
    {
        return -1;
    }

    // Stage-1 glue: one shared timestamp (the snapshot records the last
    // matching rpm OR speed frame); staleness is handled by the estimator.
    return estimator.update(
        snap.rpm,
        snap.ts_ms,
        snap.speed_kmh,
        snap.ts_ms,
        now_ms);
}

// Arduino-side glue (firmware-only — NOT compiled in the native test env):
// wires the Golf 6 default vehicle config into the CAN module and builds
// the ratio estimator. Must be called after can_init() (main boot order:
// theme -> can -> boot -> gears -> ble).
void gear_source_init(void);

// Reads the CAN snapshot + bus state and returns the current gear
// (GearPosition) or -1 when the source is offline / unknown.
int8_t gear_source_poll(void);
