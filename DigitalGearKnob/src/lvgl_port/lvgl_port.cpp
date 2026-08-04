#include "lvgl_port.h"

#include <Arduino.h>
#include <lvgl.h>

#include "display/display.h"
#include "pin_config.h"

//=====================================================
// BUFFER
//=====================================================

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;

//=====================================================
// TICK
//=====================================================

static hw_timer_t *timer = NULL;

void IRAM_ATTR lv_tick()
{
    lv_tick_inc(1);
}

//=====================================================
// FLUSH
//=====================================================

static void my_flush(lv_disp_drv_t *disp,
                     const lv_area_t *area,
                     lv_color_t *color_p)
{
    display_get_gfx()->draw16bitRGBBitmap(
        area->x1,
        area->y1,
        (uint16_t *)color_p,
        area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1);

    // CO5300 GRAM write latency: after the QSPI transfer completes, the display
    // controller needs time to process received pixels into its internal GRAM
    // (which includes a compression pipeline). A new command before GRAM settles
    // causes display corruption.
    delayMicroseconds(100);

    lv_disp_flush_ready(disp);
}

//=====================================================
// INIT
//=====================================================

void lvgl_port_init()
{
    lv_init();

    //-------------------------------------------------
    // Buffer completo en PSRAM
    //-------------------------------------------------

    buf = (lv_color_t *)ps_malloc(
        LCD_WIDTH *
        LCD_HEIGHT *
        sizeof(lv_color_t));

    if (buf == NULL)
    {
        Serial.println("Error: No se pudo asignar buffer completo");

        buf = (lv_color_t *)ps_malloc(
            LCD_WIDTH *
            40 *
            sizeof(lv_color_t));

        lv_disp_draw_buf_init(
            &draw_buf,
            buf,
            NULL,
            LCD_WIDTH * 40);
    }
    else
    {
        Serial.println("Buffer completo asignado");

        lv_disp_draw_buf_init(
            &draw_buf,
            buf,
            NULL,
            LCD_WIDTH * LCD_HEIGHT);
    }

    //-------------------------------------------------
    // Driver
    //-------------------------------------------------

    static lv_disp_drv_t disp;

    lv_disp_drv_init(&disp);

    disp.hor_res = LCD_WIDTH;
    disp.ver_res = LCD_HEIGHT;

    disp.flush_cb = my_flush;

    disp.draw_buf = &draw_buf;

    disp.full_refresh = 1;

    disp.direct_mode = 0;

    lv_disp_drv_register(&disp);

    //-------------------------------------------------
    // Tick
    //-------------------------------------------------

    timer = timerBegin(0, 80, true);

    timerAttachInterrupt(timer, &lv_tick, true);

    timerAlarmWrite(timer, 1000, true);

    timerAlarmEnable(timer);
}