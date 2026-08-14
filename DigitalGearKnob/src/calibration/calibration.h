#pragma once

#include <cstdint>

#include "geardecode/gear_types.h"

//=====================================================
// CALIBRACIÓN (v2)
//=====================================================
// La calibración por cuaterniones NVS fue removida (design: calibration
// removal). El gear ratio ahora viene de la tabla estática GearRatioConfig
// (src/geardecode/can_types.h); los datos NVS heredados se ignoran.
// Sobrevive únicamente la conversión de texto -> marcha (spec calibration:
// enum y fromString sin cambios) porque el protocolo y los tests la usan.

//=====================================================
// API
//=====================================================

bool calibration_fromString(
    const char *text,
    GearPosition &gear);
