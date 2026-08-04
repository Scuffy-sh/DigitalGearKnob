#pragma once

#include <Arduino.h>

//=====================================================
// MARCHAS
//=====================================================

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

//=====================================================
// CALIBRACIÓN
//=====================================================

struct GearCalibration
{
    float w;
    float x;
    float y;
    float z;

    bool valid;
};

//=====================================================
// API
//=====================================================

void calibration_init();

void calibration_save(
    GearPosition gear,
    float w,
    float x,
    float y,
    float z);

GearCalibration calibration_get(GearPosition gear);

bool calibration_is_valid(GearPosition gear);

bool calibration_fromString(
    const String &text,
    GearPosition &gear);