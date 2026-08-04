#include "display.h"

#include <Arduino.h>
#include "pin_config.h"

// =======================================================
// BUS QSPI
// =======================================================

static Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS,
    LCD_SCLK,
    LCD_SDIO0,
    LCD_SDIO1,
    LCD_SDIO2,
    LCD_SDIO3);

// =======================================================
// DISPLAY
// =======================================================

static Arduino_GFX *gfx = new Arduino_CO5300(
    bus,
    LCD_RST,
    0,
    false,
    LCD_WIDTH,
    LCD_HEIGHT,
    6,
    0,
    0,
    0);

// =======================================================
// GETTER
// =======================================================

Arduino_GFX* display_get_gfx()
{
    return gfx;
}

// =======================================================
// INIT
// =======================================================

void display_init()
{
    pinMode(LCD_EN, OUTPUT);
    digitalWrite(LCD_EN, HIGH);

    delay(50);

    gfx->begin();

    gfx->fillScreen(BLACK);

    gfx->setRotation(0);

    delay(100);
}