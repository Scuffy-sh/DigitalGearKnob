#pragma once

#include <cstdint>

#include "geardecode/can_types.h"

//=====================================================
// RATIO ESTIMATOR (pure — design D7, TDD core)
//=====================================================

// Estimates the engaged gear from the engine RPM / wheel-speed ratio.
// Pure C++17: no Arduino, no timers — the caller passes the timestamps
// (spec: gear-ratio-estimator domain, pure and testable requirement).
class RatioEstimator
{
public:
    explicit RatioEstimator(const GearRatioConfig &config);

    // Returns:
    //   GEAR_R..GEAR_5 when the ratio matches a gear band,
    //   GEAR_N         for standstill, clutch/neutral, or stale inputs,
    //   -1             when the config is invalid.
    int8_t update(
        float rpm,
        uint32_t rpm_ts,
        float speed,
        uint32_t speed_ts,
        uint32_t now_ms);

    bool config_valid() const { return config_valid_; }

private:
    // True when `ratio` lies inside gear g's band expanded by `factor`
    // (1.0 = base band, HYST_FACTOR = hysteresis hold band).
    bool in_band(int gear, float ratio, float factor) const;

    GearRatioConfig config_;
    bool config_valid_;
    int8_t last_gear_; // last matched gear, -1 when none
};
