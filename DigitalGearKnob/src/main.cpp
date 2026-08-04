#include <Arduino.h>
#include <lvgl.h>

#include "display/display.h"
#include "lvgl_port/lvgl_port.h"

#include "ui/ui.h"

#include "boot/boot.h"
#include "gears/gears.h"
#include "theme/theme.h"
#include "ble/ble.h"

// NUEVO
#include "bno/bno.h"
#include "calibration/calibration.h"

//=====================================================
// SETUP
//=====================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== START SYSTEM ===");

    // -------------------------
    // HARDWARE / DISPLAY
    // -------------------------
    display_init();
    lvgl_port_init();

    // -------------------------
    // UI GENERADA POR SQUARELINE
    // -------------------------
    ui_init();

    // -------------------------
    // NUEVOS MODULOS
    // -------------------------
    calibration_init();

    if (!bno_init())
    {
        Serial.println("ERROR inicializando BNO085");
    }

    // -------------------------
    // MODULOS DE APLICACION
    // -------------------------
    theme_init();
    boot_init();
    gears_init();
    ble_init();

    Serial.println("Setup completado");
}

//=====================================================
// LOOP
//=====================================================

void loop()
{
    // LVGL necesita correr SIEMPRE
    lv_timer_handler();

    // -------------------------
    // LOGICA DE MODULOS
    // Solo después del boot completo para evitar
    // lecturas I2C del BNO085 durante la pantalla de logo,
    // que pueden causar resets si el sensor no responde.
    // -------------------------
    if (boot_completed())
    {
        gears_update();
        ble_update();
    }

    delay(1);
}