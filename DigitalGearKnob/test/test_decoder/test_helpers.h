#pragma once

#include "geardecode/can_types.h"

//=====================================================
// Shared decoder test fixtures
//=====================================================

// Well-separated synthetic bands (R=200, 1=160, 2=120, 3=90, 4=65, 5=45)
// so every spec scenario can be exercised unambiguously.
// Base bands at 8%: R[184,216] 1[147.2,172.8] 2[110.4,129.6]
//                   3[82.8,97.2] 4[59.8,70.2] 5[41.4,48.6]
inline GearRatioConfig make_test_ratio_config()
{
    GearRatioConfig cfg{};
    cfg.ratio_rpm_per_kmh[GEAR_R] = 200.0f;
    cfg.ratio_rpm_per_kmh[GEAR_1] = 160.0f;
    cfg.ratio_rpm_per_kmh[GEAR_2] = 120.0f;
    cfg.ratio_rpm_per_kmh[GEAR_3] = 90.0f;
    cfg.ratio_rpm_per_kmh[GEAR_4] = 65.0f;
    cfg.ratio_rpm_per_kmh[GEAR_5] = 45.0f;
    cfg.ratio_rpm_per_kmh[GEAR_N] = 0.0f;
    cfg.band_half_pct = 8.0f;
    cfg.stale_ms = 500;
    cfg.min_speed_kmh = 5.0f;
    return cfg;
}
