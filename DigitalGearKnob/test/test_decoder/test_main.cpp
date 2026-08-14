#include <unity.h>

// Runners registered per work unit (see test_can_types.cpp,
// test_gearratio.cpp, test_vwsignals.cpp, test_can_hal.cpp,
// test_sniff.cpp, test_gearsource.cpp).
void run_can_types_tests();
void run_gearratio_tests();
void run_vwsignals_tests();
void run_can_hal_tests();
void run_sniff_tests();
void run_gearsource_tests();

void setUp(void) {}
void tearDown(void) {}

void setup()
{
    UNITY_BEGIN();

    run_can_types_tests();
    run_gearratio_tests();
    run_vwsignals_tests();
    run_can_hal_tests();
    run_sniff_tests();
    run_gearsource_tests();

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
