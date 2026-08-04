#pragma once

#include <Arduino.h>

//=====================================================
// QUATERNION
//=====================================================

struct Quaternion
{
    float w;
    float x;
    float y;
    float z;
};

//=====================================================
// API
//=====================================================

bool bno_init();

bool bno_readQuaternion(Quaternion &quat);

/// Intenta recuperar el sensor tras un lock-up del bus I2C.
/// Devuelve true si el sensor respondió correctamente después de la recuperación.
bool bno_reset();

/// Devuelve true si el sensor BNO085 fue inicializado correctamente.
bool bno_is_available();