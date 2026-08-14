#include <Arduino.h>
#include <lvgl.h>

#include "display/display.h"
#include "lvgl_port/lvgl_port.h"

#include "ui/ui.h"

#include "boot/boot.h"
#include "gears/gears.h"
#include "theme/theme.h"
#include "ble/ble.h"

// v2: transceptor CAN listen-only (reemplaza la detección BNO085)
#include "can/can.h"

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
    // MODULOS DE APLICACION
    // Orden de boot v2 (design data flow):
    // theme -> can -> boot -> gears -> ble.
    // can_init() es no-fatal (spec: init failure is non-fatal): si el
    // transceptor falla, el resto del sistema arranca igual y el estado
    // se reporta como CAN offline.
    // -------------------------
    theme_init();

    if (!can_init())
    {
        Serial.println("CAN offline (listen-only init failed)");
    }

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
    // Solo después del boot completo para evitar tocar la UI durante la
    // pantalla de logo. La tarea RX de CAN corre independientemente y
    // no depende de este gate (design D5).
    // -------------------------
    if (boot_completed())
    {
        gears_update();
        ble_update();
    }

    delay(1);
}
