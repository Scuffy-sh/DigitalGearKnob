#include <unity.h>

#include "geardecode/gearratio.h"

// =====================================================
// Test fixture
// =====================================================
// Well-separated synthetic bands (R=200, 1=160, 2=120, 3=90, 4=65, 5=45)
// so every spec scenario can be exercised unambiguously.
// Base bands at 8%:      R[184,216] 1[147.2,172.8] 2[110.4,129.6]
//                        3[82.8,97.2] 4[59.8,70.2] 5[41.4,48.6]

GearRatioConfig make_test_config()
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

// =====================================================
// Ratio in band -> that gear
// =====================================================

void test_band_match_reverse()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_R, est.update(4000.0f, 1000, 20.0f, 1000, 1000));
}

void test_band_match_first()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_1, est.update(4000.0f, 1000, 25.0f, 1000, 1000));
}

void test_band_match_second()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_2, est.update(3000.0f, 1000, 25.0f, 1000, 1000));
}

void test_band_match_third()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
}

void test_band_match_fourth()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_4, est.update(2600.0f, 1000, 40.0f, 1000, 1000));
}

void test_band_match_fifth()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_5, est.update(2250.0f, 1000, 50.0f, 1000, 1000));
}

// =====================================================
// Hysteresis
// =====================================================

void test_ratio_between_bands_holds_last_gear()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    // ratio 78: below gear 3's base band (82.8), above gear 4's base band
    // (70.2) -> inside gear 3's hysteresis hold, must stay in gear 3.
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(3900.0f, 1000, 50.0f, 1000, 1000));
}

void test_band_edge_jitter_stays_stable()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));

    // Dither around the gear 3 / gear 4 boundary: 99, 103, 96, 99.
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4950.0f, 1000, 50.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(5150.0f, 1000, 50.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4800.0f, 1000, 50.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4950.0f, 1000, 50.0f, 1000, 1000));
}

void test_genuine_shift_up_3_to_4()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    // ratio 65: outside gear 3's hold, inside gear 4's base band.
    TEST_ASSERT_EQUAL_INT8(GEAR_4, est.update(2600.0f, 1000, 40.0f, 1000, 1000));
}

void test_genuine_shift_up_4_to_5()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_4, est.update(2600.0f, 1000, 40.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_5, est.update(2250.0f, 1000, 50.0f, 1000, 1000));
}

void test_genuine_shift_down_3_to_2()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_2, est.update(3000.0f, 1000, 25.0f, 1000, 1000));
}

// =====================================================
// Neutral / standstill / clutch
// =====================================================

void test_standstill_idle_is_neutral()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(800.0f, 1000, 0.0f, 1000, 1000));
}

void test_standstill_revving_is_neutral()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(4000.0f, 1000, 0.0f, 1000, 1000));
}

void test_speed_below_minimum_is_neutral()
{
    RatioEstimator est(make_test_config());
    // ratio would be 90 (gear 3) but 4 km/h is below min_speed_kmh = 5.
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(360.0f, 1000, 4.0f, 1000, 1000));
}

void test_clutch_slip_while_moving_is_neutral()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    // 3000 rpm at 12 km/h = ratio 250: above every band -> N, never gear 3.
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(3000.0f, 1000, 12.0f, 1000, 1000));
}

void test_neutral_coasting_is_neutral()
{
    RatioEstimator est(make_test_config());
    // 800 rpm at 40 km/h = ratio 20: below every band.
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(800.0f, 1000, 40.0f, 1000, 1000));
}

void test_negative_speed_is_neutral()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(1000.0f, 1000, -5.0f, 1000, 1000));
}

// =====================================================
// Stale input + resume
// =====================================================

void test_stale_rpm_is_neutral()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    // rpm frame is 1000 ms old at now=2000 (stale_ms = 500).
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(4500.0f, 1000, 50.0f, 2000, 2000));
}

void test_stale_speed_is_neutral()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(4500.0f, 2000, 50.0f, 1000, 2000));
}

void test_resume_after_stale_timeout()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(4500.0f, 1000, 50.0f, 2000, 2000));
    // Frames resume: ratio 120 -> gear 2, not a stale hold.
    TEST_ASSERT_EQUAL_INT8(GEAR_2, est.update(3000.0f, 2500, 25.0f, 2500, 2500));
}

void test_resume_after_neutral()
{
    RatioEstimator est(make_test_config());
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
    TEST_ASSERT_EQUAL_INT8(GEAR_N, est.update(3000.0f, 1000, 12.0f, 1000, 1000));
    // Back to a firm gear 3 ratio after the clutch release.
    TEST_ASSERT_EQUAL_INT8(GEAR_3, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
}

// =====================================================
// Config validation
// =====================================================

void test_invalid_band_config_returns_minus_one()
{
    GearRatioConfig cfg = make_test_config();
    cfg.band_half_pct = 0.0f;
    RatioEstimator est(cfg);
    TEST_ASSERT_EQUAL_INT8(-1, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
}

void test_invalid_stale_config_returns_minus_one()
{
    GearRatioConfig cfg = make_test_config();
    cfg.stale_ms = 0;
    RatioEstimator est(cfg);
    TEST_ASSERT_EQUAL_INT8(-1, est.update(4500.0f, 1000, 50.0f, 1000, 1000));
}

void test_config_validity_flag()
{
    RatioEstimator valid(make_test_config());
    TEST_ASSERT_TRUE(valid.config_valid());

    GearRatioConfig cfg = make_test_config();
    cfg.min_speed_kmh = -1.0f;
    RatioEstimator invalid(cfg);
    TEST_ASSERT_FALSE(invalid.config_valid());
}

// =====================================================
// Runner
// =====================================================

void run_gearratio_tests()
{
    RUN_TEST(test_band_match_reverse);
    RUN_TEST(test_band_match_first);
    RUN_TEST(test_band_match_second);
    RUN_TEST(test_band_match_third);
    RUN_TEST(test_band_match_fourth);
    RUN_TEST(test_band_match_fifth);

    RUN_TEST(test_ratio_between_bands_holds_last_gear);
    RUN_TEST(test_band_edge_jitter_stays_stable);
    RUN_TEST(test_genuine_shift_up_3_to_4);
    RUN_TEST(test_genuine_shift_up_4_to_5);
    RUN_TEST(test_genuine_shift_down_3_to_2);

    RUN_TEST(test_standstill_idle_is_neutral);
    RUN_TEST(test_standstill_revving_is_neutral);
    RUN_TEST(test_speed_below_minimum_is_neutral);
    RUN_TEST(test_clutch_slip_while_moving_is_neutral);
    RUN_TEST(test_neutral_coasting_is_neutral);
    RUN_TEST(test_negative_speed_is_neutral);

    RUN_TEST(test_stale_rpm_is_neutral);
    RUN_TEST(test_stale_speed_is_neutral);
    RUN_TEST(test_resume_after_stale_timeout);
    RUN_TEST(test_resume_after_neutral);

    RUN_TEST(test_invalid_band_config_returns_minus_one);
    RUN_TEST(test_invalid_stale_config_returns_minus_one);
    RUN_TEST(test_config_validity_flag);
}
