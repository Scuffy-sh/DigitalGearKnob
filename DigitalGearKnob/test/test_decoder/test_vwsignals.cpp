#include <unity.h>

#include "geardecode/vwsignals.h"

// =====================================================
// Fixture helpers
// =====================================================

CanFrame make_frame(uint32_t id, uint8_t dlc, const uint8_t *bytes)
{
    CanFrame f{};
    f.id = id;
    f.dlc = dlc;
    f.ext = false;
    f.ts = 0;
    for (uint8_t i = 0; i < dlc; ++i)
    {
        f.data[i] = bytes[i];
    }
    return f;
}

VehicleCanConfig make_configured_config()
{
    VehicleCanConfig cfg = make_golf6_default_config();
    cfg.signals_configured = true;
    cfg.rpm_sig = SignalDesc{0x540, 16, 16, 0.25f, 0.0f, false};
    cfg.speed_sig = SignalDesc{0x48A, 0, 16, 0.01f, 0.0f, false};
    return cfg;
}

// =====================================================
// vw_extract_raw — little-endian bit unpacking
// =====================================================

void test_extract_16bit_from_byte_pair()
{
    // Signal: start_bit 16, length 16 -> bytes 2..3, LSB first.
    const uint8_t data[8] = {0, 0, 0xA0, 0x0F, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 4, data);

    uint32_t raw = 0;
    TEST_ASSERT_TRUE(vw_extract_raw(frame, SignalDesc{0x540, 16, 16, 1.0f, 0.0f, false}, raw));
    TEST_ASSERT_EQUAL_UINT32(0x0FA0, raw); // 4000
}

void test_extract_cross_byte_little_endian()
{
    // start_bit 4, length 12: value bits 0..3 live in byte 0 bits 4..7,
    // value bits 4..11 continue in byte 1 bits 0..7 (LE bit numbering).
    const uint8_t data[8] = {0xC0, 0xAB, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x100, 2, data);

    uint32_t raw = 0;
    TEST_ASSERT_TRUE(vw_extract_raw(frame, SignalDesc{0x100, 4, 12, 1.0f, 0.0f, false}, raw));
    TEST_ASSERT_EQUAL_UINT32(0x0ABC, raw);
}

void test_extract_32bit_value()
{
    const uint8_t data[8] = {0xEF, 0xCD, 0xAB, 0x00, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x300, 4, data);

    uint32_t raw = 0;
    TEST_ASSERT_TRUE(vw_extract_raw(frame, SignalDesc{0x300, 0, 32, 1.0f, 0.0f, false}, raw));
    TEST_ASSERT_EQUAL_UINT32(0x00ABCDEF, raw);
}

void test_extract_wrong_id_returns_false()
{
    const uint8_t data[8] = {0xA0, 0x0F, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x200, 2, data);

    uint32_t raw = 0;
    TEST_ASSERT_FALSE(vw_extract_raw(frame, SignalDesc{0x540, 0, 16, 1.0f, 0.0f, false}, raw));
}

void test_extract_short_dlc_returns_false()
{
    // Signal spans bytes 2..3 but the frame only carries 2 bytes.
    const uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 2, data);

    uint32_t raw = 0;
    TEST_ASSERT_FALSE(vw_extract_raw(frame, SignalDesc{0x540, 16, 16, 1.0f, 0.0f, false}, raw));
}

void test_extract_zero_dlc_returns_false()
{
    const uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 0, data);

    uint32_t raw = 0;
    TEST_ASSERT_FALSE(vw_extract_raw(frame, SignalDesc{0x540, 0, 8, 1.0f, 0.0f, false}, raw));
}

// =====================================================
// vw_decode_signal — signedness + scaling
// =====================================================

void test_decode_scaled_value()
{
    // raw 100 * scale 0.25 + offset -5 = 20
    const uint8_t data[8] = {100, 0, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 1, data);

    float value = 0.0f;
    TEST_ASSERT_TRUE(vw_decode_signal(frame, SignalDesc{0x540, 0, 8, 0.25f, -5.0f, false}, value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, value);
}

void test_decode_signed_negative()
{
    // 12-bit raw 0xFFF has the sign bit set -> sign-extended to -1.
    const uint8_t data[8] = {0xF0, 0xFF, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 2, data);

    float value = 0.0f;
    TEST_ASSERT_TRUE(vw_decode_signal(frame, SignalDesc{0x540, 4, 12, 1.0f, 0.0f, true}, value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, value);
}

void test_decode_signed_positive()
{
    // 12-bit raw 0x555 has the sign bit clear -> positive 1365.
    const uint8_t data[8] = {0x50, 0x55, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 2, data);

    float value = 0.0f;
    TEST_ASSERT_TRUE(vw_decode_signal(frame, SignalDesc{0x540, 4, 12, 1.0f, 0.0f, true}, value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1365.0f, value);
}

void test_decode_unsigned_keeps_full_range()
{
    // Same 12-bit raw 0xFFF interpreted unsigned -> 4095.
    const uint8_t data[8] = {0xF0, 0xFF, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 2, data);

    float value = 0.0f;
    TEST_ASSERT_TRUE(vw_decode_signal(frame, SignalDesc{0x540, 4, 12, 1.0f, 0.0f, false}, value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4095.0f, value);
}

void test_decode_signed_32bit()
{
    // 32-bit raw 0xFFFFFFFF signed -> -1.
    const uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 4, data);

    float value = 0.0f;
    TEST_ASSERT_TRUE(vw_decode_signal(frame, SignalDesc{0x540, 0, 32, 1.0f, 0.0f, true}, value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, value);
}

void test_decode_wrong_id_returns_false()
{
    const uint8_t data[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x999, 1, data);

    float value = 0.0f;
    TEST_ASSERT_FALSE(vw_decode_signal(frame, SignalDesc{0x540, 0, 8, 1.0f, 0.0f, false}, value));
}

// =====================================================
// vw_decode_rpm / vw_decode_speed — configured channels
// =====================================================

void test_decode_rpm_from_matching_frame()
{
    // RPM 16-bit at bytes 2..3, scale 0.25: raw 4000 -> 1000 rpm.
    const uint8_t data[8] = {0, 0, 0xA0, 0x0F, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 4, data);

    float rpm = 0.0f;
    TEST_ASSERT_TRUE(vw_decode_rpm(frame, make_configured_config(), rpm));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1000.0f, rpm);
}

void test_decode_rpm_rejects_other_frame()
{
    const uint8_t data[8] = {0xA0, 0x0F, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x48A, 2, data);

    float rpm = 0.0f;
    TEST_ASSERT_FALSE(vw_decode_rpm(frame, make_configured_config(), rpm));
}

void test_decode_speed_from_matching_frame()
{
    // Speed 16-bit at bytes 0..1, scale 0.01: raw 4500 -> 45 km/h.
    const uint8_t data[8] = {0x94, 0x11, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x48A, 2, data);

    float speed = 0.0f;
    TEST_ASSERT_TRUE(vw_decode_speed(frame, make_configured_config(), speed));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.0f, speed);
}

void test_decode_speed_rejects_other_frame()
{
    const uint8_t data[8] = {0, 0, 0xA0, 0x0F, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 4, data);

    float speed = 0.0f;
    TEST_ASSERT_FALSE(vw_decode_speed(frame, make_configured_config(), speed));
}

void test_decode_channels_reject_unconfigured_config()
{
    VehicleCanConfig cfg = make_golf6_default_config(); // signals_configured = false
    const uint8_t data[8] = {0xA0, 0x0F, 0, 0, 0, 0, 0, 0};
    CanFrame frame = make_frame(0x540, 2, data);

    float value = 0.0f;
    TEST_ASSERT_FALSE(vw_decode_rpm(frame, cfg, value));
    TEST_ASSERT_FALSE(vw_decode_speed(frame, cfg, value));
}

// =====================================================
// Runner
// =====================================================

void run_vwsignals_tests()
{
    RUN_TEST(test_extract_16bit_from_byte_pair);
    RUN_TEST(test_extract_cross_byte_little_endian);
    RUN_TEST(test_extract_32bit_value);
    RUN_TEST(test_extract_wrong_id_returns_false);
    RUN_TEST(test_extract_short_dlc_returns_false);
    RUN_TEST(test_extract_zero_dlc_returns_false);

    RUN_TEST(test_decode_scaled_value);
    RUN_TEST(test_decode_signed_negative);
    RUN_TEST(test_decode_signed_positive);
    RUN_TEST(test_decode_unsigned_keeps_full_range);
    RUN_TEST(test_decode_signed_32bit);
    RUN_TEST(test_decode_wrong_id_returns_false);

    RUN_TEST(test_decode_rpm_from_matching_frame);
    RUN_TEST(test_decode_rpm_rejects_other_frame);
    RUN_TEST(test_decode_speed_from_matching_frame);
    RUN_TEST(test_decode_speed_rejects_other_frame);
    RUN_TEST(test_decode_channels_reject_unconfigured_config);
}
