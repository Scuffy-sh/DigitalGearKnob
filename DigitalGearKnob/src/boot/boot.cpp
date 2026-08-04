#include "boot.h"

#include <lvgl.h>
#include "ui/ui.h"

//=====================================================
// ESTADO
//=====================================================

static bool g_boot_completed = false;

//=====================================================
// CAMBIO A PANTALLA PRINCIPAL
//=====================================================

static void switch_to_gears(lv_timer_t *t)
{
    lv_timer_del(t);

    g_boot_completed = true;

    lv_scr_load_anim(
        ui_InterfazMarchas,
        LV_SCR_LOAD_ANIM_FADE_ON,
        300,
        0,
        false);
}

//=====================================================
// INICIO DEL BOOT
//=====================================================

void boot_init()
{
    // Mostrar logo

    lv_obj_set_style_opa(ui_InterfazLogo, 255, 0);

    lv_obj_set_style_transform_zoom(
        ui_InterfazLogo,
        256,
        0);

    lv_scr_load(ui_InterfazLogo);

    // Esperar 4 segundos

    lv_timer_create(
        switch_to_gears,
        4000,
        NULL);
}

//=====================================================
// ESTADO
//=====================================================

bool boot_completed()
{
    return g_boot_completed;
}