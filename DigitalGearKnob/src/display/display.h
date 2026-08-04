#pragma once

#include <Arduino_GFX_Library.h>

// Inicializa la pantalla
void display_init();

// Devuelve el objeto gráfico para que LVGL pueda dibujar
Arduino_GFX* display_get_gfx();