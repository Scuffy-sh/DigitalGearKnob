#include <unity.h>

#include "can/can.h"

// =====================================================
// CanHwConfig — safety-critical construction/validation
// (design D4: listen-only + zero TX queue are the non-negotiable
// invariants of the HAL; the config seam must reject any config
// that could open a transmit path)
// =====================================================

void test_default_config_is_listen_only()
{
    CanHwConfig cfg;
    TEST_ASSERT_TRUE(cfg.listen_only);
}

void test_default_config_pins()
{
    CanHwConfig cfg;
    TEST_ASSERT_EQUAL_UINT8(3, cfg.tx_gpio);
    TEST_ASSERT_EQUAL_UINT8(5, cfg.rx_gpio);
}

void test_default_config_bitrate()
{
    CanHwConfig cfg;
    TEST_ASSERT_EQUAL_UINT32(500000, cfg.bitrate);
}

void test_default_config_tx_queue_disabled()
{
    CanHwConfig cfg;
    TEST_ASSERT_EQUAL_UINT32(0, cfg.tx_queue_len);
}

void test_default_config_rx_queue_sized()
{
    CanHwConfig cfg;
    TEST_ASSERT_EQUAL_UINT32(64, cfg.rx_queue_len);
}

void test_default_config_valid()
{
    CanHwConfig cfg;
    TEST_ASSERT_TRUE(cfg.is_valid());
}

void test_non_listen_only_rejected()
{
    CanHwConfig cfg;
    cfg.listen_only = false; // safety: a TX-capable mode must never pass
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_tx_queue_enabled_rejected()
{
    CanHwConfig cfg;
    cfg.tx_queue_len = 1; // safety: any TX queue opens a transmit path
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_tx_rx_same_pin_rejected()
{
    CanHwConfig cfg;
    cfg.rx_gpio = cfg.tx_gpio;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_out_of_range_pin_rejected()
{
    CanHwConfig cfg;
    cfg.tx_gpio = 200; // outside ESP32-S3 GPIO range
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_zero_rx_queue_rejected()
{
    CanHwConfig cfg;
    cfg.rx_queue_len = 0;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_unsupported_bitrate_rejected()
{
    CanHwConfig cfg;
    cfg.bitrate = 300000; // no TWAI timing config exists for this rate
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_hal_zero_bitrate_rejected()
{
    CanHwConfig cfg;
    cfg.bitrate = 0;
    TEST_ASSERT_FALSE(cfg.is_valid());
}

void test_other_supported_bitrate_valid()
{
    CanHwConfig cfg;
    cfg.bitrate = 250000; // supported by TWAI timing macros
    TEST_ASSERT_TRUE(cfg.is_valid());
}

// =====================================================
// CanStatusTracker — status state machine
// (spec: alert reporting; recovery without transmitting)
// =====================================================

void test_tracker_starts_offline()
{
    CanStatusTracker t;
    TEST_ASSERT_FALSE(t.is_online());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanBusState::OFFLINE),
                            static_cast<uint8_t>(t.state()));
}

void test_started_goes_online()
{
    CanStatusTracker t;
    t.on_started();
    TEST_ASSERT_TRUE(t.is_online());
}

void test_start_failed_stays_offline()
{
    CanStatusTracker t;
    t.on_start_failed();
    TEST_ASSERT_FALSE(t.is_online());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanBusState::OFFLINE),
                            static_cast<uint8_t>(t.state()));
}

void test_bus_off_takes_offline()
{
    CanStatusTracker t;
    t.on_started();
    t.on_bus_off_alert();
    TEST_ASSERT_FALSE(t.is_online());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanBusState::BUS_OFF),
                            static_cast<uint8_t>(t.state()));
}

void test_recovery_cycle_returns_online()
{
    CanStatusTracker t;
    t.on_started();
    t.on_bus_off_alert();
    t.on_recovery_started();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanBusState::RECOVERING),
                            static_cast<uint8_t>(t.state()));
    t.on_bus_recovered_alert();
    TEST_ASSERT_TRUE(t.is_online());
}

void test_recovery_without_bus_off_ignored()
{
    CanStatusTracker t;
    t.on_started();
    t.on_recovery_started(); // invalid transition — must be ignored
    TEST_ASSERT_TRUE(t.is_online());
}

void test_error_passive_does_not_break_online()
{
    CanStatusTracker t;
    t.on_started();
    t.on_error_passive_alert();
    TEST_ASSERT_TRUE(t.is_online());
    TEST_ASSERT_TRUE(t.is_error_passive());
}

void test_error_passive_clear_by_default()
{
    CanStatusTracker t;
    TEST_ASSERT_FALSE(t.is_error_passive());
}

// =====================================================
// Runner
// =====================================================

void run_can_hal_tests()
{
    RUN_TEST(test_default_config_is_listen_only);
    RUN_TEST(test_default_config_pins);
    RUN_TEST(test_default_config_bitrate);
    RUN_TEST(test_default_config_tx_queue_disabled);
    RUN_TEST(test_default_config_rx_queue_sized);
    RUN_TEST(test_default_config_valid);
    RUN_TEST(test_non_listen_only_rejected);
    RUN_TEST(test_tx_queue_enabled_rejected);
    RUN_TEST(test_tx_rx_same_pin_rejected);
    RUN_TEST(test_out_of_range_pin_rejected);
    RUN_TEST(test_zero_rx_queue_rejected);
    RUN_TEST(test_unsupported_bitrate_rejected);
    RUN_TEST(test_hal_zero_bitrate_rejected);
    RUN_TEST(test_other_supported_bitrate_valid);

    RUN_TEST(test_tracker_starts_offline);
    RUN_TEST(test_started_goes_online);
    RUN_TEST(test_start_failed_stays_offline);
    RUN_TEST(test_bus_off_takes_offline);
    RUN_TEST(test_recovery_cycle_returns_online);
    RUN_TEST(test_recovery_without_bus_off_ignored);
    RUN_TEST(test_error_passive_does_not_break_online);
    RUN_TEST(test_error_passive_clear_by_default);
}
