#pragma once

#include <cstdint>

//=====================================================
// GEAR POSITIONS
//=====================================================
// Pure header - zero Arduino dependencies. Shared by the
// calibration module and the v2 CAN gear decoding chain
// (runs unchanged in the native test env).

enum GearPosition : uint8_t
{
    GEAR_R = 0,
    GEAR_1,
    GEAR_2,
    GEAR_3,
    GEAR_4,
    GEAR_5,
    GEAR_N,

    GEAR_COUNT
};
