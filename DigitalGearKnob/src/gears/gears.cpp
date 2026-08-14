#include "gears.h"

#include <Arduino.h>
#include <lvgl.h>
#include "ui/ui.h"
#include "gearsource/gearsource.h"

//=====================================================
// ESTADO
//=====================================================

static uint8_t current_gear = 0;   // 0 = R
static uint32_t last_change = 0;

//=====================================================
// LÓGICA DE UI / ANIMACIÓN ARC
//=====================================================

static int last_displayed_gear = -1;
static int8_t anim_target_gear = -1;  // marcha destino de la animación

static void arc_anim_cb(void *var, int32_t v)
{
    lv_arc_set_value(ui_CargaMarchas, v);

    // La etiqueta muestra SIEMPRE la marcha destino durante la
    // animación; el arco solo es un efecto visual de "carga" y
    // no cuenta marchas intermedias (evita N-5-4-3-2-1 al bajar).
    int gear = anim_target_gear;

    if (gear < 0)
    {
        // Fallback defensivo (no debería ocurrir): mapeo por valor.
        if (v < 20) gear = 0;
        else if (v < 40) gear = 1;
        else if (v < 60) gear = 2;
        else if (v < 80) gear = 3;
        else if (v < 95) gear = 4;
        else gear = 5;
    }

    if (gear != last_displayed_gear)
    {
        last_displayed_gear = gear;

        char txt[4];

        if (gear == 0)
        {
            strcpy(txt, "R");
        }
        else if (gear == 6)
        {
            strcpy(txt, "N");
        }
        else
        {
            sprintf(txt, "%d", gear);
        }

        lv_label_set_text(ui_Marchas, txt);
    }
}

//=====================================================
// ANIMACIÓN HACIA NUEVA MARCHA
//=====================================================

static void animate_to_gear(uint8_t gear)
{
    if (!ui_CargaMarchas) return;

    int target;

    if (gear == 0) target = 0;
    else if (gear == 6) target = 100;  // N = arco completo (como el arranque)
    else target = gear * 20;

    anim_target_gear = gear;

    lv_anim_t a;
    lv_anim_init(&a);

    lv_anim_set_var(&a, ui_CargaMarchas);

    lv_anim_set_exec_cb(&a, arc_anim_cb);

    lv_anim_set_values(
        &a,
        lv_arc_get_value(ui_CargaMarchas),
        target);

    lv_anim_set_time(&a, 2000);

    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_start(&a);
}

//=====================================================
// DETECCIÓN (v2: fuente CAN — src/gearsource/)
//=====================================================
// La detección BNO085 (cuaterniones, zonas pitch/roll, recaptura de
// neutral) fue removida. La marcha proviene del pipeline CAN:
// snapshot -> ratio estimator -> gear (gearsource.h). La API pública
// gears.h y el debounce de 3 frames quedan intactos.

// Intervalo de detección (ms)
static const uint32_t DETECTION_INTERVAL_MS = 200;

// Umbral de stabilización: cuántos frames consecutivos
// deben coincidir para aceptar un cambio de marcha.
static const uint8_t STABILITY_FRAMES = 3;

static uint32_t last_detection_time = 0;
static uint8_t stability_counter = 0;
static int8_t pending_gear = -1;

//=====================================================
// ESTADO DESCONOCIDO ("--")
//=====================================================
// Design D8: cuando la fuente CAN no produce una marcha válida (bus
// offline, señales sin configurar, estimador en neutral/stale) la
// pantalla muestra el estado explícito "--", nunca una marcha vieja.
static void show_unknown_state()
{
    if (!ui_Marchas) return;

    // Congelar cualquier animación en curso para que su callback no
    // pise la etiqueta con una marcha destino obsoleta.
    anim_target_gear = -1;
    last_displayed_gear = -1;

    if (ui_CargaMarchas)
    {
        lv_anim_del(ui_CargaMarchas, NULL);
    }

    lv_label_set_text(ui_Marchas, "--");
}

//=====================================================
// API PUBLICA
//=====================================================

void gears_init()
{
    current_gear = 6;  // Neutral
    last_change = millis();

    // v2: la fuente de detección es el pipeline CAN (requiere can_init()
    // ya ejecutado — orden de boot: theme -> can -> boot -> gears -> ble).
    gear_source_init();

    if (ui_CargaMarchas)
    {
        lv_arc_set_range(ui_CargaMarchas, 0, 100);
        lv_arc_set_value(ui_CargaMarchas, 100);
    }

    if (ui_Marchas)
    {
        lv_label_set_text(ui_Marchas, "N");
    }

    last_displayed_gear = 6;
}

void gears_set(uint8_t gear)
{
    current_gear = gear;
    last_change = millis();

    animate_to_gear(gear);

    Serial.print("Gear changed to: ");
    Serial.println(gear);
}

uint8_t gears_get_current()
{
    return current_gear;
}

void gears_update()
{
    if (millis() - last_detection_time < DETECTION_INTERVAL_MS)
    {
        return;
    }
    last_detection_time = millis();

    int8_t detected = gear_source_poll();

    // Debug
    if (detected >= 0)
    {
        const char *names[] = {"R", "1", "2", "3", "4", "5", "N"};
        Serial.print("Gear source: ");
        Serial.println(names[detected]);
    }
    else
    {
        Serial.println("Gear source: ??? (CAN offline / sin configurar)");

        stability_counter = 0;
        pending_gear = -1;

        // D8: estado explícito, nunca una marcha obsoleta en pantalla.
        show_unknown_state();
        return;
    }

    // ── Stabilization: exigir N frames consecutivos ──
    if (detected == pending_gear)
    {
        stability_counter++;
    }
    else
    {
        pending_gear = detected;
        stability_counter = 1;
    }

    if (stability_counter >= STABILITY_FRAMES && (uint8_t)detected != current_gear)
    {
        gears_set((uint8_t)detected);
        stability_counter = 0;
        pending_gear = -1;
    }
}
