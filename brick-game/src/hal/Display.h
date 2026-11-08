#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ES3C28P / ES3N28P: ILI9341V 240×320 on SPI2. See HARDWARE.md §1.
//   SCK 12 · MOSI 11 · MISO 13 · DC 46 · CS 10 · RST = EN (shared, so -1) · BL 45 (PWM)
// If colours look wrong on the real panel, flip these two:
#ifndef LCD_INVERT
#define LCD_INVERT false
#endif
#ifndef LCD_RGB_ORDER
#define LCD_RGB_ORDER false
#endif
// 2 = 180°: the board sits upside down in the case so USB-C faces the top edge.
#ifndef LCD_ROTATION
#define LCD_ROTATION 2
#endif

class Display : public lgfx::LGFX_Device {
public:
    Display() {
        {
            auto cfg = bus_.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 12;
            cfg.pin_mosi = 11;
            cfg.pin_miso = 13;
            cfg.pin_dc = 46;
            bus_.config(cfg);
            panel_.setBus(&bus_);
        }
        {
            auto cfg = panel_.config();
            cfg.pin_cs = 10;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.readable = true;
            cfg.invert = LCD_INVERT;
            cfg.rgb_order = LCD_RGB_ORDER;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;   // SD card uses SDMMC, not this SPI bus
            panel_.config(cfg);
        }
        {
            auto cfg = light_.config();
            cfg.pin_bl = 45;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            light_.config(cfg);
            panel_.setLight(&light_);
        }
        setPanel(&panel_);
    }

private:
    lgfx::Bus_SPI bus_;
    lgfx::Panel_ILI9341 panel_;
    lgfx::Light_PWM light_;
};
