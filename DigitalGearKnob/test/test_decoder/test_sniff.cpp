#include <unity.h>

#include "can/sniff.h"

// =====================================================
// SniffLimiter — 20 Hz output cap (spec: rate limiting)
// =====================================================

void test_limiter_first_frame_allowed()
{
    SniffLimiter lim(50); // 50 ms period = 20 Hz
    TEST_ASSERT_TRUE(lim.allow(1000));
}

void test_limiter_rejects_frame_within_period()
{
    SniffLimiter lim(50);
    TEST_ASSERT_TRUE(lim.allow(1000));
    TEST_ASSERT_FALSE(lim.allow(1049));
}

void test_limiter_allows_frame_at_period_boundary()
{
    SniffLimiter lim(50);
    TEST_ASSERT_TRUE(lim.allow(1000));
    TEST_ASSERT_TRUE(lim.allow(1050));
}

void test_limiter_keeps_cap_under_burst()
{
    SniffLimiter lim(50);
    // a burst inside one window publishes at most one frame
    TEST_ASSERT_TRUE(lim.allow(0));
    TEST_ASSERT_FALSE(lim.allow(10));
    TEST_ASSERT_FALSE(lim.allow(49));
    TEST_ASSERT_TRUE(lim.allow(50));
    TEST_ASSERT_FALSE(lim.allow(51));
}

void test_limiter_reset_opens_window()
{
    SniffLimiter lim(50);
    lim.allow(0);
    lim.reset();
    TEST_ASSERT_TRUE(lim.allow(0));
}

// =====================================================
// Runtime toggle (spec: toggleable at runtime)
// =====================================================

void test_sniff_disabled_by_default()
{
    TEST_ASSERT_FALSE(sniff_enabled());
}

void test_sniff_toggle_on()
{
    sniff_set_enabled(true);
    TEST_ASSERT_TRUE(sniff_enabled());
}

void test_sniff_toggle_off()
{
    sniff_set_enabled(true);
    sniff_set_enabled(false);
    TEST_ASSERT_FALSE(sniff_enabled());
}

// =====================================================
// sniff_format_frame — "sniff:<E|S><8hexid>:<payloadhex>"
// (design D6 / BLE protocol v2 table)
// =====================================================

void test_format_standard_frame()
{
    CanFrame f{};
    f.id = 0x280;
    f.dlc = 2;
    f.data[0] = 0x0A;
    f.data[1] = 0x1B;
    f.ext = false;
    char out[64];
    TEST_ASSERT_TRUE(sniff_format_frame(f, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sniff:S00000280:0A1B", out);
}

void test_format_extended_frame()
{
    CanFrame f{};
    f.id = 0x280;
    f.dlc = 2;
    f.data[0] = 0x0A;
    f.data[1] = 0x1B;
    f.ext = true;
    char out[64];
    TEST_ASSERT_TRUE(sniff_format_frame(f, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sniff:E00000280:0A1B", out);
}

void test_format_full_payload()
{
    CanFrame f{};
    f.id = 0x123;
    f.dlc = 8;
    for (uint8_t i = 0; i < 8; ++i) f.data[i] = 0x10 + i;
    f.ext = false;
    char out[64];
    TEST_ASSERT_TRUE(sniff_format_frame(f, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sniff:S00000123:1011121314151617", out);
}

void test_format_zero_dlc()
{
    CanFrame f{};
    f.id = 0x280;
    f.dlc = 0;
    f.ext = true;
    char out[64];
    TEST_ASSERT_TRUE(sniff_format_frame(f, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sniff:E00000280:", out);
}

void test_format_max_29bit_id()
{
    CanFrame f{};
    f.id = 0x1FFFFFFF;
    f.dlc = 1;
    f.data[0] = 0xFF;
    f.ext = true;
    char out[64];
    TEST_ASSERT_TRUE(sniff_format_frame(f, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sniff:E1FFFFFFF:FF", out);
}

void test_format_small_buffer_rejected()
{
    CanFrame f{};
    f.id = 0x280;
    f.dlc = 2;
    f.data[0] = 0x0A;
    f.data[1] = 0x1B;
    f.ext = true;
    char out[8];
    TEST_ASSERT_FALSE(sniff_format_frame(f, out, sizeof(out)));
}

void test_format_exact_buffer_accepted()
{
    CanFrame f{};
    f.id = 0x280;
    f.dlc = 2;
    f.data[0] = 0x0A;
    f.data[1] = 0x1B;
    f.ext = false;
    char out[21]; // 20 chars + NUL
    TEST_ASSERT_TRUE(sniff_format_frame(f, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("sniff:S00000280:0A1B", out);
}

void test_format_null_output_rejected()
{
    CanFrame f{};
    f.id = 0x280;
    f.dlc = 2;
    TEST_ASSERT_FALSE(sniff_format_frame(f, nullptr, 64));
}

// =====================================================
// SniffWatchdog — silent-bus "can:no_frames" signal
// (spec: silent bus scenario)
// =====================================================

void test_watchdog_silent_bus_reports()
{
    SniffWatchdog wd(2000, 2000);
    wd.poll(true, 0); // observation starts now
    TEST_ASSERT_TRUE(wd.poll(true, 2000));
}

void test_watchdog_silent_before_threshold_no_report()
{
    SniffWatchdog wd(2000, 2000);
    wd.poll(true, 0); // observation starts now
    TEST_ASSERT_FALSE(wd.poll(true, 1999));
    TEST_ASSERT_TRUE(wd.poll(true, 2000));
}

void test_watchdog_frame_resets_silence()
{
    SniffWatchdog wd(2000, 2000);
    wd.note_frame(1000);
    TEST_ASSERT_FALSE(wd.poll(true, 2999)); // 1999 ms since the frame
    TEST_ASSERT_TRUE(wd.poll(true, 3000));  // exactly 2000 ms
}

void test_watchdog_no_report_when_sniff_off()
{
    SniffWatchdog wd(2000, 2000);
    wd.note_frame(0);
    TEST_ASSERT_FALSE(wd.poll(false, 20000));
}

void test_watchdog_report_rate_limited()
{
    SniffWatchdog wd(2000, 2000);
    wd.poll(true, 0); // observation starts now
    TEST_ASSERT_TRUE(wd.poll(true, 2000));
    TEST_ASSERT_FALSE(wd.poll(true, 3500));
    TEST_ASSERT_TRUE(wd.poll(true, 4000));
}

void test_watchdog_reports_after_resume_silence()
{
    SniffWatchdog wd(2000, 2000);
    wd.note_frame(5000);
    TEST_ASSERT_FALSE(wd.poll(true, 6900));
    TEST_ASSERT_TRUE(wd.poll(true, 7000));
}

void test_watchdog_reenable_restarts_clock()
{
    SniffWatchdog wd(2000, 2000);
    wd.poll(true, 0);              // first observation session
    wd.poll(false, 500);           // sniffing off mid-way
    TEST_ASSERT_FALSE(wd.poll(true, 2499)); // fresh 2 s window from 2499
    TEST_ASSERT_FALSE(wd.poll(true, 4498));
    TEST_ASSERT_TRUE(wd.poll(true, 4499));
}

// =====================================================
// Runner
// =====================================================

void run_sniff_tests()
{
    RUN_TEST(test_limiter_first_frame_allowed);
    RUN_TEST(test_limiter_rejects_frame_within_period);
    RUN_TEST(test_limiter_allows_frame_at_period_boundary);
    RUN_TEST(test_limiter_keeps_cap_under_burst);
    RUN_TEST(test_limiter_reset_opens_window);

    RUN_TEST(test_sniff_disabled_by_default);
    RUN_TEST(test_sniff_toggle_on);
    RUN_TEST(test_sniff_toggle_off);

    RUN_TEST(test_format_standard_frame);
    RUN_TEST(test_format_extended_frame);
    RUN_TEST(test_format_full_payload);
    RUN_TEST(test_format_zero_dlc);
    RUN_TEST(test_format_max_29bit_id);
    RUN_TEST(test_format_small_buffer_rejected);
    RUN_TEST(test_format_exact_buffer_accepted);
    RUN_TEST(test_format_null_output_rejected);

    RUN_TEST(test_watchdog_silent_bus_reports);
    RUN_TEST(test_watchdog_silent_before_threshold_no_report);
    RUN_TEST(test_watchdog_frame_resets_silence);
    RUN_TEST(test_watchdog_no_report_when_sniff_off);
    RUN_TEST(test_watchdog_report_rate_limited);
    RUN_TEST(test_watchdog_reports_after_resume_silence);
    RUN_TEST(test_watchdog_reenable_restarts_clock);
}
