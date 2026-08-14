#include "geardecode/gearratio.h"

#include <cmath>

namespace
{
// Hysteresis hold multiplier: the last gear's band is widened by this
// factor so band-edge jitter and between-band ratios keep the current
// gear (spec: hysteresis requirement).
constexpr float HYST_FACTOR = 2.0f;
} // namespace

RatioEstimator::RatioEstimator(const GearRatioConfig &config)
    : config_(config),
      config_valid_(config.is_valid()),
      last_gear_(-1)
{
}

bool RatioEstimator::in_band(int gear, float ratio, float factor) const
{
    const float center = config_.ratio_rpm_per_kmh[gear];
    const float half = center * (config_.band_half_pct / 100.0f) * factor;
    return ratio >= center - half && ratio <= center + half;
}

int8_t RatioEstimator::update(
    float rpm,
    uint32_t rpm_ts,
    float speed,
    uint32_t speed_ts,
    uint32_t now_ms)
{
    if (!config_valid_)
    {
        return -1;
    }

    // Stale input: never display stale data (spec: stale-input timeout).
    if (now_ms - rpm_ts > config_.stale_ms ||
        now_ms - speed_ts > config_.stale_ms)
    {
        last_gear_ = -1;
        return GEAR_N;
    }

    // Standstill / creep / impossible inputs: the ratio is meaningless
    // (spec: neutral/standstill/clutch handling). min_speed_kmh >= 0 is
    // guaranteed by config validation, so speed <= min_speed implies a
    // non-positive speed here and the division below is always safe.
    if (!(speed > config_.min_speed_kmh))
    {
        last_gear_ = -1;
        return GEAR_N;
    }

    const float ratio = rpm / speed;

    // Hysteresis: hold the last gear while the ratio stays near its band.
    if (last_gear_ >= GEAR_R && last_gear_ < GEAR_N &&
        in_band(last_gear_, ratio, HYST_FACTOR))
    {
        return last_gear_;
    }

    // Firm band match: switch to the closest matching gear.
    int best = -1;
    float best_delta = INFINITY;
    for (int g = GEAR_R; g < GEAR_N; ++g)
    {
        if (!in_band(g, ratio, 1.0f))
        {
            continue;
        }
        const float delta = std::fabs(ratio - config_.ratio_rpm_per_kmh[g]);
        if (delta < best_delta)
        {
            best_delta = delta;
            best = g;
        }
    }
    if (best >= 0)
    {
        last_gear_ = static_cast<int8_t>(best);
        return last_gear_;
    }

    // No band matches: neutral, clutch slip, or wheel slip.
    last_gear_ = -1;
    return GEAR_N;
}
