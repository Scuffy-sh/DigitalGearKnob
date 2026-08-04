#pragma once

#include <stdint.h>

// Inicializa el módulo
void gears_init();

// Llamar en el loop
void gears_update();

// Cambiar marcha manualmente (0 = R, 1-5 = marchas)
void gears_set(uint8_t gear);

/// Devuelve la marcha actualmente detectada (0=R, 1-5=marchas, 6=N)
uint8_t gears_get_current();