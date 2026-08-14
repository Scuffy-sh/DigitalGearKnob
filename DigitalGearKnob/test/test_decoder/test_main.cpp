#include <unity.h>

// Runners registered per work unit (see test_can_types.cpp,
// test_gearratio.cpp, test_vwsignals.cpp).
void run_can_types_tests();
void run_gearratio_tests();

void setUp(void) {}
void tearDown(void) {}

void setup()
{
    UNITY_BEGIN();

    run_can_types_tests();
    run_gearratio_tests();

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
