#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "pin_config.h"

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Bus_SPI _bus;

public:

  LGFX()
  {
    // ================= BUS =================
    {
      auto cfg = _bus.config();

      cfg.spi_host = SPI2_HOST;

      cfg.freq_write = 80000000;
      cfg.freq_read  = 16000000;

      cfg.spi_mode = 0;

      cfg.pin_sclk = LCD_SCLK;
      cfg.pin_mosi = LCD_SDIO0;
      cfg.pin_miso = -1;

      cfg.pin_dc = LCD_DC;

      _bus.config(cfg);
    }

    setBus(&_bus);
  }
};