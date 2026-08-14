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
