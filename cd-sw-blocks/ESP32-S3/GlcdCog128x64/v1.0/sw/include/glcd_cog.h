#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"

#define GLCD_WIDTH  128
#define GLCD_HEIGHT  64
#define GLCD_PAGES   (GLCD_HEIGHT / 8) // 8 pages

typedef struct {
    spi_host_device_t spi_host;  // SPI2_HOST or SPI3_HOST
    int sck_pin;
    int mosi_pin;
    int cs_pin;
    int dc_pin;   // data/command select
    int rst_pin;  // hardware reset (-1 = not used)
    int clk_hz;   // SPI clock, max 10MHz for ST7565
} glcd_cog_config_t;

void glcd_cog_init(const glcd_cog_config_t *cfg);
void glcd_cog_clear(void);
void glcd_cog_update(void);           // flush framebuffer to display
void glcd_cog_set_contrast(uint8_t val); // 0–63
void glcd_cog_invert(bool on);

// Drawing — all operate on framebuffer, call glcd_cog_update() to show
void glcd_cog_set_pixel(uint8_t x, uint8_t y, bool on);
void glcd_cog_draw_hline(uint8_t x, uint8_t y, uint8_t len, bool on);
void glcd_cog_draw_vline(uint8_t x, uint8_t y, uint8_t len, bool on);
void glcd_cog_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);
void glcd_cog_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);
void glcd_cog_draw_line(int x0, int y0, int x1, int y1, bool on);

// Text — 5×7 font, 1px spacing → 6px per char
void glcd_cog_draw_char(uint8_t x, uint8_t y, char c);
void glcd_cog_draw_string(uint8_t x, uint8_t y, const char *str);
