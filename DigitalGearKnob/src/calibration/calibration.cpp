#include "calibration.h"

#include <cstdio>
#include <cstring>

#include <Preferences.h>

//=====================================================
// DATOS
//=====================================================

static GearCalibration calibrations[GEAR_COUNT];
static Preferences prefs;

//=====================================================
// INIT
//=====================================================

void calibration_init()
{
    prefs.begin("calibration", false);

    for (uint8_t i = 0; i < GEAR_COUNT; i++)
    {
        char key[8];
        snprintf(key, sizeof(key), "gear_%u", i);

        size_t read = prefs.getBytes(key, &calibrations[i], sizeof(GearCalibration));

        if (read != sizeof(GearCalibration))
        {
            calibrations[i].w = 0.0f;
            calibrations[i].x = 0.0f;
            calibrations[i].y = 0.0f;
            calibrations[i].z = 0.0f;
            calibrations[i].valid = false;
        }

        Serial.printf("Calibration gear %u: %s\n", i, calibrations[i].valid ? "loaded" : "empty");
    }
}

//=====================================================
// GUARDAR
//=====================================================

void calibration_save(
    GearPosition gear,
    float w,
    float x,
    float y,
    float z)
{
    calibrations[gear].w = w;
    calibrations[gear].x = x;
    calibrations[gear].y = y;
    calibrations[gear].z = z;
    calibrations[gear].valid = true;

    char key[8];
    snprintf(key, sizeof(key), "gear_%u", gear);
    prefs.putBytes(key, &calibrations[gear], sizeof(GearCalibration));
}

//=====================================================
// OBTENER
//=====================================================

GearCalibration calibration_get(GearPosition gear)
{
    return calibrations[gear];
}

//=====================================================
// VALIDAR
//=====================================================

bool calibration_is_valid(GearPosition gear)
{
    return calibrations[gear].valid;
}

//=====================================================
// CONVERTIR TEXTO -> MARCHA
//=====================================================

bool calibration_fromString(
    const char *text,
    GearPosition &gear)
{
    if (text == nullptr)
    {
        return false;
    }

    if (strcmp(text, "R") == 0)
    {
        gear = GEAR_R;
        return true;
    }

    if (strcmp(text, "1") == 0)
    {
        gear = GEAR_1;
        return true;
    }

    if (strcmp(text, "2") == 0)
    {
        gear = GEAR_2;
        return true;
    }

    if (strcmp(text, "3") == 0)
    {
        gear = GEAR_3;
        return true;
    }

    if (strcmp(text, "4") == 0)
    {
        gear = GEAR_4;
        return true;
    }

    if (strcmp(text, "5") == 0)
    {
        gear = GEAR_5;
        return true;
    }

    if (strcmp(text, "N") == 0)
    {
        gear = GEAR_N;
        return true;
    }

    return false;
}