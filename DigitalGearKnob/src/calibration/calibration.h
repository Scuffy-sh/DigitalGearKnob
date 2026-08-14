#pragma once

#include <cstdint>

#include "geardecode/gear_types.h"

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
    const char *text,
    GearPosition &gear);