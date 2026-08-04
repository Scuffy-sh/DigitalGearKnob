#include "theme.h"

#include <Arduino.h>
#include <Preferences.h>
#include <lvgl.h>

#include "ui/screens/ui_InterfazMarchas.h"

//=====================================================
// ESTADO GLOBAL
//=====================================================

static uint32_t primary_color = 0x0052FF;

static Preferences prefs;

//=====================================================
// APLICAR A LVGL
//=====================================================

static void apply_theme()
{
    lv_color_t c = lv_color_hex(primary_color);

    //=========================================
    // ARC
    //=========================================

    if (ui_CargaMarchas != nullptr)
    {
        // Indicador
        lv_obj_set_style_arc_color(
            ui_CargaMarchas,
            c,
            LV_PART_INDICATOR | LV_STATE_DEFAULT);

        // Fondo del arco
        lv_obj_set_style_arc_color(
            ui_CargaMarchas,
            lv_color_hex(0x222222),
            LV_PART_MAIN | LV_STATE_DEFAULT);

        // Botón del arc
        lv_obj_set_style_bg_color(
            ui_CargaMarchas,
            c,
            LV_PART_KNOB | LV_STATE_DEFAULT);

        lv_obj_invalidate(ui_CargaMarchas);
    }

    //=========================================
    // SPINNER
    //=========================================

    if (ui_BarraRotacion != nullptr)
    {
        lv_obj_set_style_arc_color(
            ui_BarraRotacion,
            c,
            LV_PART_INDICATOR | LV_STATE_DEFAULT);

        lv_obj_invalidate(ui_BarraRotacion);
    }

    //=========================================
    // LABEL SCUFFY
    //=========================================

    if (ui_Scuffy != nullptr)
    {
        lv_obj_set_style_text_color(
            ui_Scuffy,
            c,
            LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_invalidate(ui_Scuffy);
    }
}

//=====================================================
// INIT
//=====================================================

void theme_init()
{
    prefs.begin("theme", false);

    primary_color = prefs.getUInt("primary", 0x0052FF);

    Serial.print("Theme cargado: 0x");
    Serial.println(primary_color, HEX);

    apply_theme();
}

//=====================================================
// SET COLOR
//=====================================================

void theme_set_primary(uint32_t color)
{
    Serial.print("Aplicando color: 0x");
    Serial.println(color, HEX);

    primary_color = color;

    apply_theme();
}

//=====================================================
// GET COLOR
//=====================================================

uint32_t theme_get_primary()
{
    return primary_color;
}

//=====================================================
// SAVE
//=====================================================

void theme_save()
{
    prefs.putUInt("primary", primary_color);
}