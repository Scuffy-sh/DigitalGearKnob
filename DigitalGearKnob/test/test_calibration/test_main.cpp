#include <unity.h>
#include "calibration/calibration.h"

void setUp(void) {}
void tearDown(void) {}

// =====================================================
// calibration_fromString — valid inputs
// =====================================================

void test_fromString_R()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_TRUE(calibration_fromString("R", gear));
    TEST_ASSERT_EQUAL(GEAR_R, gear);
}

void test_fromString_1()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_TRUE(calibration_fromString("1", gear));
    TEST_ASSERT_EQUAL(GEAR_1, gear);
}

void test_fromString_2()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_TRUE(calibration_fromString("2", gear));
    TEST_ASSERT_EQUAL(GEAR_2, gear);
}

void test_fromString_3()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_TRUE(calibration_fromString("3", gear));
    TEST_ASSERT_EQUAL(GEAR_3, gear);
}

void test_fromString_4()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_TRUE(calibration_fromString("4", gear));
    TEST_ASSERT_EQUAL(GEAR_4, gear);
}

void test_fromString_5()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_TRUE(calibration_fromString("5", gear));
    TEST_ASSERT_EQUAL(GEAR_5, gear);
}

void test_fromString_N()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_TRUE(calibration_fromString("N", gear));
    TEST_ASSERT_EQUAL(GEAR_N, gear);
}

// =====================================================
// calibration_fromString — invalid inputs
// =====================================================

void test_fromString_empty()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_FALSE(calibration_fromString("", gear));
}

void test_fromString_invalid_letter()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_FALSE(calibration_fromString("X", gear));
}

void test_fromString_out_of_range()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_FALSE(calibration_fromString("6", gear));
}

void test_fromString_lowercase()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_FALSE(calibration_fromString("r", gear));
}

void test_fromString_with_space()
{
    GearPosition gear = GEAR_COUNT;
    TEST_ASSERT_FALSE(calibration_fromString(" R", gear));
}

// =====================================================
// calibration_init + calibration_is_valid
// =====================================================

void test_init_all_invalid()
{
    calibration_init();

    for (uint8_t i = 0; i < GEAR_COUNT; i++)
    {
        GearPosition pos = static_cast<GearPosition>(i);
        TEST_ASSERT_FALSE(calibration_is_valid(pos));
    }
}

void test_init_all_zeroed()
{
    calibration_init();

    for (uint8_t i = 0; i < GEAR_COUNT; i++)
    {
        GearPosition pos = static_cast<GearPosition>(i);
        GearCalibration cal = calibration_get(pos);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cal.w);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cal.x);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cal.y);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cal.z);
    }
}

// =====================================================
// calibration_save + calibration_get
// =====================================================

void test_save_and_get()
{
    calibration_init();

    calibration_save(GEAR_1, 1.0f, 2.0f, 3.0f, 4.0f);

    GearCalibration cal = calibration_get(GEAR_1);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, cal.w);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, cal.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, cal.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, cal.z);
    TEST_ASSERT_TRUE(cal.valid);
}

void test_save_marks_valid()
{
    calibration_init();

    TEST_ASSERT_FALSE(calibration_is_valid(GEAR_3));

    calibration_save(GEAR_3, 0.1f, 0.2f, 0.3f, 0.4f);

    TEST_ASSERT_TRUE(calibration_is_valid(GEAR_3));
}

void test_save_does_not_affect_other_gears()
{
    calibration_init();

    calibration_save(GEAR_2, 5.0f, 6.0f, 7.0f, 8.0f);

    for (uint8_t i = 0; i < GEAR_COUNT; i++)
    {
        if (i == GEAR_2)
            continue;
        GearPosition pos = static_cast<GearPosition>(i);
        TEST_ASSERT_FALSE(calibration_is_valid(pos));
    }
}

void test_overwrite_calibration()
{
    calibration_init();

    calibration_save(GEAR_R, 1.0f, 1.0f, 1.0f, 1.0f);
    calibration_save(GEAR_R, 9.0f, 8.0f, 7.0f, 6.0f);

    GearCalibration cal = calibration_get(GEAR_R);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.0f, cal.w);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f, cal.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, cal.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, cal.z);
}

void test_negative_values()
{
    calibration_init();

    calibration_save(GEAR_N, -1.5f, -2.5f, -3.5f, -4.5f);

    GearCalibration cal = calibration_get(GEAR_N);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.5f, cal.w);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.5f, cal.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -3.5f, cal.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -4.5f, cal.z);
}

void test_all_gears_independently()
{
    calibration_init();

    for (uint8_t i = 0; i < GEAR_COUNT; i++)
    {
        GearPosition pos = static_cast<GearPosition>(i);
        float val = (float)(i + 1) * 1.1f;
        calibration_save(pos, val, val, val, val);
    }

    for (uint8_t i = 0; i < GEAR_COUNT; i++)
    {
        GearPosition pos = static_cast<GearPosition>(i);
        float expected = (float)(i + 1) * 1.1f;
        GearCalibration cal = calibration_get(pos);
        TEST_ASSERT_TRUE(cal.valid);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, cal.w);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, cal.x);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, cal.y);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, cal.z);
    }
}

// =====================================================
// GearPosition enum completeness
// =====================================================

void test_gear_count_is_7()
{
    TEST_ASSERT_EQUAL(7, GEAR_COUNT);
}

void test_enum_values()
{
    TEST_ASSERT_EQUAL(0, GEAR_R);
    TEST_ASSERT_EQUAL(1, GEAR_1);
    TEST_ASSERT_EQUAL(2, GEAR_2);
    TEST_ASSERT_EQUAL(3, GEAR_3);
    TEST_ASSERT_EQUAL(4, GEAR_4);
    TEST_ASSERT_EQUAL(5, GEAR_5);
    TEST_ASSERT_EQUAL(6, GEAR_N);
}

// =====================================================
// Main
// =====================================================

void setup()
{
    UNITY_BEGIN();

    RUN_TEST(test_fromString_R);
    RUN_TEST(test_fromString_1);
    RUN_TEST(test_fromString_2);
    RUN_TEST(test_fromString_3);
    RUN_TEST(test_fromString_4);
    RUN_TEST(test_fromString_5);
    RUN_TEST(test_fromString_N);

    RUN_TEST(test_fromString_empty);
    RUN_TEST(test_fromString_invalid_letter);
    RUN_TEST(test_fromString_out_of_range);
    RUN_TEST(test_fromString_lowercase);
    RUN_TEST(test_fromString_with_space);

    RUN_TEST(test_init_all_invalid);
    RUN_TEST(test_init_all_zeroed);

    RUN_TEST(test_save_and_get);
    RUN_TEST(test_save_marks_valid);
    RUN_TEST(test_save_does_not_affect_other_gears);
    RUN_TEST(test_overwrite_calibration);
    RUN_TEST(test_negative_values);
    RUN_TEST(test_all_gears_independently);

    RUN_TEST(test_gear_count_is_7);
    RUN_TEST(test_enum_values);

    UNITY_END();
}

void loop() {}

// Native entry point: the native env has no Arduino loop runner,
// so bridge the Arduino-style setup()/loop() into a host main().
int main()
{
    setup();
    loop();
    return 0;
}
