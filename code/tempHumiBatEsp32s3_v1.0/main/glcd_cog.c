#include "glcd_cog.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

#define BL_LEDC_SPEED    LEDC_LOW_SPEED_MODE
#define BL_LEDC_TIMER    LEDC_TIMER_0
#define BL_LEDC_CHANNEL  LEDC_CHANNEL_0
#define BL_LEDC_FREQ     5000
#define BL_DUTY_MAX      255u   // 8-bit resolution: 0–255

// ST7565 command bytes
#define CMD_DISPLAY_OFF      0xAE
#define CMD_DISPLAY_ON       0xAF
#define CMD_SET_STARTLINE    0x40 // | line (0-63)
#define CMD_SET_PAGE         0xB0 // | page (0-7)
#define CMD_SET_COL_HIGH     0x10 // | col[7:4]
#define CMD_SET_COL_LOW      0x00 // | col[3:0]
#define CMD_ADC_NORMAL       0xA0
#define CMD_ADC_REVERSE      0xA1
#define CMD_DISP_NORMAL      0xA6
#define CMD_DISP_REVERSE     0xA7
#define CMD_ALL_ON           0xA5
#define CMD_ALL_NORMAL       0xA4
#define CMD_BIAS_9           0xA2
#define CMD_BIAS_7           0xA3
#define CMD_COM_NORMAL       0xC0
#define CMD_COM_REVERSE      0xC8
#define CMD_BOOSTER_RATIO    0xF8
#define CMD_POWER_CTRL       0x28 // | 0x07 = booster+reg+follower on
#define CMD_RESISTOR_RATIO   0x20 // | 0-7
#define CMD_SET_EV           0x81 // followed by EV byte (0-63)
#define CMD_RESET            0xE2
#define CMD_NOP              0xE3

extern const uint8_t glcd_font5x7[][5];
extern const uint8_t GLCD_FONT_FIRST;
extern const uint8_t GLCD_FONT_LAST;

static spi_device_handle_t s_spi;
static glcd_cog_config_t   s_cfg;
static uint8_t             s_fb[GLCD_PAGES][GLCD_WIDTH]; // framebuffer
static glcd_orient_t       s_orient = GLCD_ORIENT_H;

static void send_cmd(uint8_t cmd)
{
    gpio_set_level(s_cfg.dc_pin, 0); // command
    spi_transaction_t t = {
        .length    = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(s_spi, &t);
}

static void send_data(const uint8_t *data, size_t len)
{
    gpio_set_level(s_cfg.dc_pin, 1); // data
    spi_transaction_t t = {
        .length    = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(s_spi, &t);
}

void glcd_cog_init(const glcd_cog_config_t *cfg)
{
    s_cfg = *cfg;
    memset(s_fb, 0, sizeof(s_fb));

    // DC pin
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << cfg->dc_pin),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    // RST pin (optional)
    if (cfg->rst_pin >= 0) {
        gpio_config_t rst = {
            .pin_bit_mask = (1ULL << cfg->rst_pin),
            .mode         = GPIO_MODE_OUTPUT,
        };
        gpio_config(&rst);
        gpio_set_level(cfg->rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(cfg->rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Backlight pin (optional) — PWM via LEDC, active low
    if (cfg->bl_pin >= 0) {
        ledc_timer_config_t bl_timer = {
            .speed_mode      = BL_LEDC_SPEED,
            .timer_num       = BL_LEDC_TIMER,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .freq_hz         = BL_LEDC_FREQ,
            .clk_cfg         = LEDC_AUTO_CLK,
        };
        ledc_timer_config(&bl_timer);

        ledc_channel_config_t bl_ch = {
            .gpio_num   = cfg->bl_pin,
            .speed_mode = BL_LEDC_SPEED,
            .channel    = BL_LEDC_CHANNEL,
            .timer_sel  = BL_LEDC_TIMER,
            .duty       = BL_DUTY_MAX,  // start OFF (active low: full duty = off)
            .hpoint     = 0,
        };
        ledc_channel_config(&bl_ch);
    }

    // SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num  = cfg->mosi_pin,
        .miso_io_num  = -1,
        .sclk_io_num  = cfg->sck_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(cfg->spi_host, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = cfg->clk_hz,
        .mode           = 0,  // CPOL=0, CPHA=0
        .spics_io_num   = cfg->cs_pin,
        .queue_size     = 1,
    };
    spi_bus_add_device(cfg->spi_host, &devcfg, &s_spi);

    // ST7565 init sequence (matched to working reference project)
    send_cmd(CMD_SET_STARTLINE | 0);      // display start line = 0
    send_cmd(CMD_ADC_REVERSE);            // 0xA1 - ADC reverse (connector at top)
    send_cmd(CMD_COM_NORMAL);             // 0xC0 - COM normal
    send_cmd(CMD_DISP_NORMAL);            // 0xA6 - not inverted
    send_cmd(CMD_BIAS_9);                 // 0xA2 - 1/9 bias
    send_cmd(CMD_POWER_CTRL | 0x07);      // 0x2F - power: booster+regulator+follower ON
    vTaskDelay(pdMS_TO_TICKS(50));
    send_cmd(0xf8);                       // booster ratio set
    send_cmd(0x00);                       // booster ratio = 4x  (missing = blank display)
    send_cmd(CMD_RESISTOR_RATIO | 0x07);  // 0x27 - V0 resistor ratio = 7 (max)
    send_cmd(CMD_SET_EV);                 // 0x81 - contrast mode
    send_cmd(0x01);                       // contrast = 1 (tune via serial: 0x00–0x3F)
    send_cmd(0xac);                       // static indicator
    send_cmd(0x00);                       // static indicator off
    send_cmd(CMD_DISPLAY_ON);             // 0xAF

    // Turn backlight on now that display is initialised
    if (cfg->bl_pin >= 0)
        glcd_cog_set_backlight(100);
}

void glcd_cog_update(void)
{
    uint8_t col = s_cfg.col_offset;
    for (uint8_t page = 0; page < GLCD_PAGES; page++) {
        send_cmd(CMD_SET_PAGE | page);
        send_cmd(CMD_SET_COL_LOW  | (col & 0x0F));
        send_cmd(CMD_SET_COL_HIGH | (col >> 4));
        send_data(s_fb[page], GLCD_WIDTH);
    }
}

void glcd_cog_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

void glcd_cog_set_contrast(uint8_t val)
{
    if (val > 63) val = 63;
    send_cmd(CMD_SET_EV);
    send_cmd(val);
}

void glcd_cog_invert(bool on)
{
    send_cmd(on ? CMD_DISP_REVERSE : CMD_DISP_NORMAL);
}

void glcd_cog_set_pixel(uint8_t x, uint8_t y, bool on)
{
    // Transform logical (x,y) to physical (px,py) based on orientation.
    // DISH/DISV: hardware handles flip — no software transform needed.
    // CW/CCW:   physical display stays in DISH state; SW rotates framebuffer.
    //   CW  logical canvas 64×128 → physical 128×64: px=127-y, py=x
    //   CCW logical canvas 64×128 → physical 128×64: px=y,     py=63-x
    uint8_t px, py;
    switch (s_orient) {
    case GLCD_ORIENT_CW:
        px = (GLCD_WIDTH  - 1) - y;
        py = x;
        break;
    case GLCD_ORIENT_CCW:
        px = y;
        py = (GLCD_HEIGHT - 1) - x;
        break;
    default:
        px = x;
        py = y;
        break;
    }
    if (px >= GLCD_WIDTH || py >= GLCD_HEIGHT) return;
    uint8_t page = py / 8;
    uint8_t bit  = py % 8;
    if (on) s_fb[page][px] |=  (1 << bit);
    else    s_fb[page][px] &= ~(1 << bit);
}

void glcd_cog_draw_hline(uint8_t x, uint8_t y, uint8_t len, bool on)
{
    for (uint8_t i = 0; i < len; i++)
        glcd_cog_set_pixel(x + i, y, on);
}

void glcd_cog_draw_vline(uint8_t x, uint8_t y, uint8_t len, bool on)
{
    for (uint8_t i = 0; i < len; i++)
        glcd_cog_set_pixel(x, y + i, on);
}

void glcd_cog_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on)
{
    glcd_cog_draw_hline(x,         y,         w, on);
    glcd_cog_draw_hline(x,         y + h - 1, w, on);
    glcd_cog_draw_vline(x,         y,         h, on);
    glcd_cog_draw_vline(x + w - 1, y,         h, on);
}

void glcd_cog_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on)
{
    for (uint8_t row = y; row < y + h; row++)
        glcd_cog_draw_hline(x, row, w, on);
}

void glcd_cog_draw_line(int x0, int y0, int x1, int y1, bool on)
{
    int dx  =  abs(x1 - x0);
    int dy  = -abs(y1 - y0);
    int sx  = x0 < x1 ? 1 : -1;
    int sy  = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (1) {
        glcd_cog_set_pixel((uint8_t)x0, (uint8_t)y0, on);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void glcd_cog_draw_char(uint8_t x, uint8_t y, char c)
{
    if (c < GLCD_FONT_FIRST || c > GLCD_FONT_LAST) c = '?';
    const uint8_t *glyph = glcd_font5x7[c - GLCD_FONT_FIRST];
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (uint8_t row = 0; row < 7; row++)
            glcd_cog_set_pixel(x + col, y + row, (bits >> row) & 1);
    }
    // 1px spacing column
    for (uint8_t row = 0; row < 7; row++)
        glcd_cog_set_pixel(x + 5, y + row, false);
}

void glcd_cog_draw_string(uint8_t x, uint8_t y, const char *str)
{
    uint8_t max_x = glcd_cog_log_width();
    while (*str) {
        glcd_cog_draw_char(x, y, *str++);
        x += 6;
        if (x + 6 > max_x) break;
    }
}

void glcd_cog_set_backlight(uint8_t pct)
{
    if (s_cfg.bl_pin < 0) return;
    if (pct > 100) pct = 100;
    // active low: 100% brightness = duty 0, 0% brightness = duty 255
    uint32_t duty = (uint32_t)(100 - pct) * BL_DUTY_MAX / 100;
    ledc_set_duty(BL_LEDC_SPEED, BL_LEDC_CHANNEL, duty);
    ledc_update_duty(BL_LEDC_SPEED, BL_LEDC_CHANNEL);
}

void glcd_cog_backlight(bool on)
{
    glcd_cog_set_backlight(on ? 100 : 0);
}

void glcd_cog_set_orientation(glcd_orient_t orient)
{
    s_orient = orient;
    if (orient == GLCD_ORIENT_V) {
        // 180°: flip both ADC and COM in hardware
        send_cmd(CMD_ADC_NORMAL);   // 0xA0 — col 0 → SEG0
        send_cmd(CMD_COM_REVERSE);  // 0xC8 — COM63 becomes top row
        s_cfg.col_offset = 0;       // 4 extra SEGs fall off the right edge
    } else {
        // DISH, CW, CCW: hardware stays in normal landscape state;
        // CW/CCW rotation handled by set_pixel coordinate transform.
        send_cmd(CMD_ADC_REVERSE);  // 0xA1 — col 0 → SEG131
        send_cmd(CMD_COM_NORMAL);   // 0xC0 — COM0 is top row
        s_cfg.col_offset = 4;       // skip 4 invisible SEGs on left
    }
    // Caller must clear + redraw + update after this call.
}

uint8_t glcd_cog_log_width(void)
{
    return (s_orient == GLCD_ORIENT_CW || s_orient == GLCD_ORIENT_CCW)
           ? GLCD_HEIGHT : GLCD_WIDTH;
}

uint8_t glcd_cog_log_height(void)
{
    return (s_orient == GLCD_ORIENT_CW || s_orient == GLCD_ORIENT_CCW)
           ? GLCD_WIDTH : GLCD_HEIGHT;
}
