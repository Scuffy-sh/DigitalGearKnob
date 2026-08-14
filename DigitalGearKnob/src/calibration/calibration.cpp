#include "calibration.h"

#include <cstring>

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
