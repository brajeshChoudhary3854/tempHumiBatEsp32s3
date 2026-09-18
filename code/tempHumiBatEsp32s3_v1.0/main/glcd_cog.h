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
    int dc_pin;      // data/command select
    int rst_pin;     // hardware reset (-1 = not used)
    int bl_pin;      // backlight control, active low (-1 = not used)
    int clk_hz;      // SPI clock, max 10MHz for ST7565
    uint8_t col_offset; // column start offset (typically 4 for 132-col controllers)
} glcd_cog_config_t;

void glcd_cog_backlight(bool on);            // true = full ON, false = OFF
void glcd_cog_set_backlight(uint8_t pct);   // 0 = off, 100 = full brightness

typedef enum {
    GLCD_ORIENT_H   = 0,  // DISH  : 0°   landscape, 128×64  (default)
    GLCD_ORIENT_V,        // DISV  : 180° landscape, 128×64  (hardware flip)
    GLCD_ORIENT_CW,       // DIS90 : 90°  CW portrait, logical 64×128 (software)
    GLCD_ORIENT_CCW,      // DIS270: 90° CCW portrait, logical 64×128 (software)
} glcd_orient_t;

// In DISH/DISV mode logical canvas = 128×64.
// In DIS90/DIS270 mode logical canvas = 64×128 (x: 0-63, y: 0-127).
void glcd_cog_set_orientation(glcd_orient_t orient);
uint8_t glcd_cog_log_width(void);   // current logical width
uint8_t glcd_cog_log_height(void);  // current logical height

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
