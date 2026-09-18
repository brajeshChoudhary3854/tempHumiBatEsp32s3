#include "mcp3421.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Config register bit positions / masks
#define CFG_RDY_BIT   (1u << 7)  // 1 = trigger one-shot / conversion in progress
#define CFG_OC_BIT    (1u << 4)  // 1 = continuous,  0 = one-shot
#define CFG_RES_POS   2
#define CFG_GAIN_POS  0

#define VREF_MV       2048.0f    // Internal reference: 2.048 V
#define I2C_TMO_MS    100

// Positive full-scale count = 2^(bits-1): 12→2048, 14→8192, 16→32768, 18→131072
static const int32_t s_fullscale[4] = { 2048, 8192, 32768, 131072 };

// Worst-case conversion time per resolution (ms, with margin above datasheet)
static const uint32_t s_conv_ms[4] = { 5, 17, 67, 270 };

// ── Internal helpers ─────────────────────────────────────────────────────────

static uint8_t build_cfg_byte(const mcp3421_dev_config_t *d)
{
    uint8_t c = ((uint8_t)d->resolution << CFG_RES_POS)
              | ((uint8_t)d->gain       << CFG_GAIN_POS);
    if (d->mode == MCP3421_MODE_CONTINUOUS) c |= CFG_OC_BIT;
    return c;
}

static esp_err_t write_cfg(i2c_port_t port, uint8_t addr, uint8_t cfg_byte)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, cfg_byte, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(I2C_TMO_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

// ── Public API ───────────────────────────────────────────────────────────────

esp_err_t mcp3421_init(mcp3421_handle_t           *h,
                       const mcp3421_bus_config_t *bus,
                       const mcp3421_dev_config_t *dev)
{
    h->cfg  = *dev;
    h->port = bus->i2c_port;

    // Configure and install I2C master driver
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = bus->sda_pin,
        .scl_io_num       = bus->scl_pin,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = bus->clk_hz,
    };
    esp_err_t err = i2c_param_config(bus->i2c_port, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(bus->i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    // ESP_ERR_INVALID_STATE means driver is already installed on this port
    // (shared-bus scenario) — not an error for us
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    // Write config register; set RDY=1 to start the first conversion
    uint8_t cfg_byte = build_cfg_byte(dev) | CFG_RDY_BIT;
    return write_cfg(h->port, h->cfg.addr, cfg_byte);
}

esp_err_t mcp3421_set_config(mcp3421_handle_t    *h,
                             mcp3421_resolution_t res,
                             mcp3421_gain_t       gain,
                             mcp3421_mode_t       mode)
{
    h->cfg.resolution = res;
    h->cfg.gain       = gain;
    h->cfg.mode       = mode;
    uint8_t cfg_byte = build_cfg_byte(&h->cfg) | CFG_RDY_BIT;
    return write_cfg(h->port, h->cfg.addr, cfg_byte);
}

esp_err_t mcp3421_read_raw(const mcp3421_handle_t *h, int32_t *raw)
{
    if (h->cfg.mode == MCP3421_MODE_ONESHOT) {
        // Trigger: write config with RDY=1, no OC bit (one-shot)
        uint8_t trig = CFG_RDY_BIT
                     | ((uint8_t)h->cfg.resolution << CFG_RES_POS)
                     | ((uint8_t)h->cfg.gain       << CFG_GAIN_POS);
        esp_err_t err = write_cfg(h->port, h->cfg.addr, trig);
        if (err != ESP_OK) return err;
        // Wait for conversion to finish
        vTaskDelay(pdMS_TO_TICKS(s_conv_ms[h->cfg.resolution]));
    }
    // Continuous mode: conversion runs automatically; just read latest result

    // 18-bit needs 4 bytes (3 data + 1 config); others need 3 (2 data + 1 config)
    uint8_t buf[4] = {0};
    uint8_t read_len = (h->cfg.resolution == MCP3421_RES_18BIT) ? 4 : 3;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (h->cfg.addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf, read_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(h->port, cmd, pdMS_TO_TICKS(I2C_TMO_MS));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) return err;

    int32_t result;
    if (h->cfg.resolution == MCP3421_RES_18BIT) {
        // buf[0] = bits[17:16], buf[1] = bits[15:8], buf[2] = bits[7:0]
        result = ((int32_t)(buf[0] & 0x03) << 16)
               | ((int32_t)buf[1]          <<  8)
               |  (int32_t)buf[2];
        // Sign-extend bit 17 to 32 bits
        if (result & 0x00020000) result |= (int32_t)0xFFFC0000;
    } else {
        // MCP3421 sign-extends the output into the full 16-bit word,
        // so a simple int16_t cast handles 12, 14, and 16-bit correctly.
        result = (int32_t)(int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    }

    *raw = result;
    return ESP_OK;
}

esp_err_t mcp3421_read_mv(const mcp3421_handle_t *h, float *mv)
{
    int32_t raw;
    esp_err_t err = mcp3421_read_raw(h, &raw);
    if (err != ESP_OK) return err;

    // LSB (mV) = VREF / (fullscale_counts × gain)
    // E.g. 16-bit gain=2: 2048 / (32768 × 2) = 0.03125 mV = 31.25 µV ✓
    int gain_x = 1 << (int)h->cfg.gain;  // 1, 2, 4, 8
    float lsb_mv = VREF_MV / ((float)s_fullscale[h->cfg.resolution] * (float)gain_x);
    *mv = (float)raw * lsb_mv;
    return ESP_OK;
}

void mcp3421_deinit(const mcp3421_handle_t *h)
{
    i2c_driver_delete(h->port);
}
