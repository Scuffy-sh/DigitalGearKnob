#include <unity.h>

#include "geardecode/can_types.h"

// =====================================================
// Default Golf 6 config (starting values, unverified)
// =====================================================

void test_golf6_default_bitrate()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    TEST_ASSERT_EQUAL_UINT32(500000, cfg.bitrate);
}

void test_golf6_default_signals_not_configured()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    TEST_ASSERT_FALSE(cfg.signals_configured);
}

void test_golf6_ratio_table_has_positive_entries()
{
    GearRatioConfig cfg = make_golf6_ratio_config();

    for (int g = GEAR_R; g < GEAR_N; ++g)
    {
        TEST_ASSERT_TRUE_MESSAGE(
            cfg.ratio_rpm_per_kmh[g] > 0.0f,
            "every gear ratio must be positive");
    }
}

void test_golf6_ratio_config_valid()
{
    GearRatioConfig cfg = make_golf6_ratio_config();
    TEST_ASSERT_TRUE(cfg.is_valid());
}

void test_golf6_default_config_valid()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    TEST_ASSERT_TRUE(cfg.is_valid());
}

// =====================================================
// GearRatioConfig::is_valid
// =====================================================

GearRatioConfig make_valid_ratio_config()
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

void test_valid_ratio_config_is_valid()
{
    TEST_ASSERT_TRUE(make_valid_ratio_config().is_valid());
}

void test_zero_band_rejected()
{
    GearRatioConfig cfg = make_valid_ratio_config();
    cfg.band_half_pct = 0.0f;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_oversized_band_rejected()
{
    GearRatioConfig cfg = make_valid_ratio_config();
    cfg.band_half_pct = 60.0f;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_zero_stale_ms_rejected()
{
    GearRatioConfig cfg = make_valid_ratio_config();
    cfg.stale_ms = 0;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_negative_min_speed_rejected()
{
    GearRatioConfig cfg = make_valid_ratio_config();
    cfg.min_speed_kmh = -1.0f;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_zero_ratio_rejected()
{
    GearRatioConfig cfg = make_valid_ratio_config();
    cfg.ratio_rpm_per_kmh[GEAR_3] = 0.0f;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_negative_ratio_rejected()
{
    GearRatioConfig cfg = make_valid_ratio_config();
    cfg.ratio_rpm_per_kmh[GEAR_2] = -5.0f;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

// =====================================================
// SignalDesc::is_valid
// =====================================================

void test_zero_length_signal_rejected()
{
    SignalDesc s{};
    TEST_ASSERT_FALSE(s.is_valid());
}

void test_overlong_signal_rejected()
{
    SignalDesc s{};
    s.start_bit = 0;
    s.length = 33;
    TEST_ASSERT_FALSE(s.is_valid());
}

void test_signal_spilling_past_64_bits_rejected()
{
    SignalDesc s{};
    s.start_bit = 60;
    s.length = 8; // 60 + 8 = 68 > 64
    TEST_ASSERT_FALSE(s.is_valid());
}

void test_maximum_size_signal_accepted()
{
    SignalDesc s{};
    s.start_bit = 32;
    s.length = 32; // fits exactly in bytes 4..7
    s.scale = 1.0f;
    s.offset = 0.0f;
    TEST_ASSERT_TRUE(s.is_valid());
}

// =====================================================
// VehicleCanConfig::is_valid
// =====================================================

void test_zero_bitrate_rejected()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    cfg.bitrate = 0;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_configured_signals_required_when_enabled()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    cfg.signals_configured = true;
    cfg.rpm_sig = SignalDesc{}; // zeroed = broken signal descriptor
    cfg.speed_sig = SignalDesc{};
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_configured_signals_accepted_when_sane()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    cfg.signals_configured = true;
    cfg.rpm_sig = SignalDesc{0x540, 16, 16, 0.25f, 0.0f, false};
    cfg.speed_sig = SignalDesc{0x48A, 0, 16, 0.01f, 0.0f, false};
    TEST_ASSERT_TRUE(cfg.is_valid());
}

void test_unconfigured_signals_ignored()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    cfg.rpm_sig = SignalDesc{};
    cfg.speed_sig = SignalDesc{};
    TEST_ASSERT_TRUE(cfg.is_valid());
}

// =====================================================
// Runner
// =====================================================

void run_can_types_tests()
{
    RUN_TEST(test_golf6_default_bitrate);
    RUN_TEST(test_golf6_default_signals_not_configured);
    RUN_TEST(test_golf6_ratio_table_has_positive_entries);
    RUN_TEST(test_golf6_ratio_config_valid);
    RUN_TEST(test_golf6_default_config_valid);

    RUN_TEST(test_valid_ratio_config_is_valid);
    RUN_TEST(test_zero_band_rejected);
    RUN_TEST(test_oversized_band_rejected);
    RUN_TEST(test_zero_stale_ms_rejected);
    RUN_TEST(test_negative_min_speed_rejected);
    RUN_TEST(test_zero_ratio_rejected);
    RUN_TEST(test_negative_ratio_rejected);

    RUN_TEST(test_zero_length_signal_rejected);
    RUN_TEST(test_overlong_signal_rejected);
    RUN_TEST(test_signal_spilling_past_64_bits_rejected);
    RUN_TEST(test_maximum_size_signal_accepted);

    RUN_TEST(test_zero_bitrate_rejected);
    RUN_TEST(test_configured_signals_required_when_enabled);
    RUN_TEST(test_configured_signals_accepted_when_sane);
    RUN_TEST(test_unconfigured_signals_ignored);
}
