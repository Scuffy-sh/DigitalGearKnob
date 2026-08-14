#include <unity.h>

#include "gearsource/gearsource.h"
#include "test_helpers.h"

// =====================================================
// gear_source_decide — pure selection seam (design D7/D8/D9)
// =====================================================
// Maps (bus state + snapshot freshness) to ONE estimator call:
//   -1      bus offline, snapshot not configured, or invalid estimator config
//   GEAR_N  standstill / neutral / stale inputs (estimator internals)
//   GEAR_R..GEAR_5  matched gear

void test_offline_bus_returns_minus_one()
{
    RatioEstimator est(make_test_ratio_config());
    CanSnapshot snap{};
    snap.rpm = 4000.0f;
    snap.speed_kmh = 20.0f;
    snap.ts_ms = 1000;
    snap.valid = true;

    // D8: offline -> explicit unknown, never a silently stale gear.
    TEST_ASSERT_EQUAL_INT8(-1, gear_source_decide(false, snap, est, 1000));
}

void test_offline_preserves_estimator_hysteresis()
{
    RatioEstimator est(make_test_ratio_config());
    CanSnapshot snap{};
    snap.rpm = 4500.0f;
    snap.speed_kmh = 50.0f;
    snap.ts_ms = 1000;
    snap.valid = true;

    // Warm the estimator to gear 3 while the bus is online.
    TEST_ASSERT_EQUAL_INT8(GEAR_3, gear_source_decide(true, snap, est, 1000));

    // Offline: the glue must NOT feed the estimator (no HW read), so the
    // hysteresis state survives the offline window.
    TEST_ASSERT_EQUAL_INT8(-1, gear_source_decide(false, snap, est, 1500));

    // Resume inside the stale window: hysteresis still holds gear 3.
    snap.ts_ms = 1800;
    TEST_ASSERT_EQUAL_INT8(GEAR_3, gear_source_decide(true, snap, est, 1800));
}

void test_unconfigured_snapshot_returns_minus_one()
{
    RatioEstimator est(make_test_ratio_config());
    CanSnapshot snap{}; // valid = false: D9 signals not configured yet

    TEST_ASSERT_EQUAL_INT8(-1, gear_source_decide(true, snap, est, 1000));
}

void test_valid_snapshot_maps_to_gear()
{
    RatioEstimator est(make_test_ratio_config());
    CanSnapshot snap{};
    snap.rpm = 4500.0f;
    snap.speed_kmh = 50.0f;
    snap.ts_ms = 1000;
    snap.valid = true;

    // ratio 90 -> gear 3 band [82.8, 97.2]
    TEST_ASSERT_EQUAL_INT8(GEAR_3, gear_source_decide(true, snap, est, 1000));
}

void test_standstill_reports_neutral()
{
    RatioEstimator est(make_test_ratio_config());
    CanSnapshot snap{};
    snap.rpm = 800.0f;
    snap.speed_kmh = 0.0f;
    snap.ts_ms = 1000;
    snap.valid = true;

    TEST_ASSERT_EQUAL_INT8(GEAR_N, gear_source_decide(true, snap, est, 1000));
}

void test_stale_snapshot_reports_neutral()
{
    RatioEstimator est(make_test_ratio_config());
    CanSnapshot snap{};
    snap.rpm = 4500.0f;
    snap.speed_kmh = 50.0f;
    snap.ts_ms = 1000;
    snap.valid = true;

    // now - ts = 1000 ms > stale_ms (500): must never show stale data.
    TEST_ASSERT_EQUAL_INT8(GEAR_N, gear_source_decide(true, snap, est, 2000));
}

void test_fresh_snapshot_recovers_after_stale()
{
    RatioEstimator est(make_test_ratio_config());
    CanSnapshot snap{};
    snap.rpm = 4500.0f;
    snap.speed_kmh = 50.0f;
    snap.ts_ms = 1000;
    snap.valid = true;

    TEST_ASSERT_EQUAL_INT8(GEAR_N, gear_source_decide(true, snap, est, 2000));

    // Frames resume: fresh timestamps inside the stale window.
    snap.ts_ms = 2000;
    TEST_ASSERT_EQUAL_INT8(GEAR_3, gear_source_decide(true, snap, est, 2000));
}

void test_invalid_estimator_config_returns_minus_one()
{
    GearRatioConfig bad = make_test_ratio_config();
    bad.stale_ms = 0; // violates config validation
    RatioEstimator est(bad);

    CanSnapshot snap{};
    snap.rpm = 4500.0f;
    snap.speed_kmh = 50.0f;
    snap.ts_ms = 1000;
    snap.valid = true;

    TEST_ASSERT_EQUAL_INT8(-1, gear_source_decide(true, snap, est, 1000));
}

// =====================================================
// Runner
// =====================================================

void run_gearsource_tests()
{
    RUN_TEST(test_offline_bus_returns_minus_one);
    RUN_TEST(test_offline_preserves_estimator_hysteresis);
    RUN_TEST(test_unconfigured_snapshot_returns_minus_one);
    RUN_TEST(test_valid_snapshot_maps_to_gear);
    RUN_TEST(test_standstill_reports_neutral);
    RUN_TEST(test_stale_snapshot_reports_neutral);
    RUN_TEST(test_fresh_snapshot_recovers_after_stale);
    RUN_TEST(test_invalid_estimator_config_returns_minus_one);
}
