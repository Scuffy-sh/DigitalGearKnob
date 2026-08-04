#pragma once

#include <stdint.h>

// Inicializa tema (carga desde memoria si existe)
void theme_init();

// Cambia color principal en runtime
void theme_set_primary(uint32_t color);

// Devuelve color actual
uint32_t theme_get_primary();

// Guarda en memoria (NVS)
void theme_save();