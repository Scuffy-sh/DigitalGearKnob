#include <unity.h>

#include "ble/ble_format.h"

// =====================================================
// ble_format_debug — v2 debug notification line
// (design BLE protocol v2 table / spec ble-protocol):
//   debug:<GEAR>,<RPM>,<SPEED>,<CANSTAT>
// GEAR is R/1..5/N ("??" out of range), RPM/SPEED are
// rounded to integers, CANSTAT is ok|offline.
// =====================================================

void test_ble_format_online_mid_gear()
{
    char out[64];
    TEST_ASSERT_TRUE(
        ble_format_debug(out, sizeof(out), 3, 2450.0f, 52.0f, true));
    TEST_ASSERT_EQUAL_STRING("debug:3,2450,52,ok", out);
}

void test_ble_format_reverse_offline()
{
    char out[64];
    TEST_ASSERT_TRUE(
        ble_format_debug(out, sizeof(out), 0, 800.0f, 0.0f, false));
    TEST_ASSERT_EQUAL_STRING("debug:R,800,0,offline", out);
}

void test_ble_format_neutral_offline()
{
    char out[64];
    TEST_ASSERT_TRUE(
        ble_format_debug(out, sizeof(out), 6, 0.0f, 0.0f, false));
    TEST_ASSERT_EQUAL_STRING("debug:N,0,0,offline", out);
}

void test_ble_format_unknown_gear_out_of_range()
{
    char out[64];
    TEST_ASSERT_TRUE(
        ble_format_debug(out, sizeof(out), -1, 1200.0f, 30.0f, true));
    TEST_ASSERT_EQUAL_STRING("debug:??,1200,30,ok", out);
}

void test_ble_format_rounds_rpm_and_speed()
{
    char out[64];
    TEST_ASSERT_TRUE(
        ble_format_debug(out, sizeof(out), 2, 2450.6f, 52.4f, true));
    TEST_ASSERT_EQUAL_STRING("debug:2,2451,52,ok", out);
}

void test_ble_format_small_buffer_rejected()
{
    char out[8];
    TEST_ASSERT_FALSE(
        ble_format_debug(out, sizeof(out), 3, 2450.0f, 52.0f, true));
}

void test_ble_format_exact_buffer_accepted()
{
    // "debug:3,2450,52,ok" = 18 chars + NUL = 19
    char out[19];
    TEST_ASSERT_TRUE(
        ble_format_debug(out, sizeof(out), 3, 2450.0f, 52.0f, true));
    TEST_ASSERT_EQUAL_STRING("debug:3,2450,52,ok", out);
}

void test_ble_format_null_output_rejected()
{
    TEST_ASSERT_FALSE(
        ble_format_debug(nullptr, 64, 3, 2450.0f, 52.0f, true));
}

// =====================================================
// Runner
// =====================================================

void run_ble_format_tests()
{
    RUN_TEST(test_ble_format_online_mid_gear);
    RUN_TEST(test_ble_format_reverse_offline);
    RUN_TEST(test_ble_format_neutral_offline);
    RUN_TEST(test_ble_format_unknown_gear_out_of_range);
    RUN_TEST(test_ble_format_rounds_rpm_and_speed);
    RUN_TEST(test_ble_format_small_buffer_rejected);
    RUN_TEST(test_ble_format_exact_buffer_accepted);
    RUN_TEST(test_ble_format_null_output_rejected);
}
