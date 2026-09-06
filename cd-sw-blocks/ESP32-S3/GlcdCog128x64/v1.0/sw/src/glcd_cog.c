#include "glcd_cog.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

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

    // ST7565 init sequence
    send_cmd(CMD_RESET);
    vTaskDelay(pdMS_TO_TICKS(5));
    send_cmd(CMD_BIAS_9);                  // 1/9 bias
    send_cmd(CMD_ADC_NORMAL);             // column 0 = seg 0
    send_cmd(CMD_COM_REVERSE);            // common output: reverse for top-view
    send_cmd(CMD_DISP_NORMAL);            // not inverted
    send_cmd(CMD_ALL_NORMAL);             // normal display
    send_cmd(CMD_RESISTOR_RATIO | 0x05);  // V0 resistor ratio
    send_cmd(CMD_SET_EV);
    send_cmd(0x20);                       // contrast = 32 (mid)
    send_cmd(CMD_POWER_CTRL | 0x07);      // power: booster + regulator + follower on
    vTaskDelay(pdMS_TO_TICKS(50));
    send_cmd(CMD_SET_STARTLINE | 0);      // display start line = 0
    send_cmd(CMD_DISPLAY_ON);
}

void glcd_cog_update(void)
{
    for (uint8_t page = 0; page < GLCD_PAGES; page++) {
        send_cmd(CMD_SET_PAGE | page);
        send_cmd(CMD_SET_COL_LOW  | 0);
        send_cmd(CMD_SET_COL_HIGH | 0);
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
    if (x >= GLCD_WIDTH || y >= GLCD_HEIGHT) return;
    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    if (on)
        s_fb[page][x] |=  (1 << bit);
    else
        s_fb[page][x] &= ~(1 << bit);
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
    while (*str) {
        glcd_cog_draw_char(x, y, *str++);
        x += 6;
        if (x + 6 > GLCD_WIDTH) break;
    }
}
