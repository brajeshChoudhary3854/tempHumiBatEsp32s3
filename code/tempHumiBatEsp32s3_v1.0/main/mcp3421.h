#pragma once
#include <stdint.h>
#include "driver/i2c.h"
#include "esp_err.h"

// MCP3421 fixed I2C address (no address pin).
// For multiple devices: use separate I2C ports, or MCP3422/3423/3424 variants.
#define MCP3421_ADDR_DEFAULT   0x68

// Resolution — also determines sample rate
typedef enum {
    MCP3421_RES_12BIT = 0,  // 240 SPS  — 1000.0 µV/LSB
    MCP3421_RES_14BIT = 1,  //  60 SPS  —  250.0 µV/LSB
    MCP3421_RES_16BIT = 2,  //  15 SPS  —   62.5 µV/LSB  ← default
    MCP3421_RES_18BIT = 3,  // 3.75 SPS —   15.6 µV/LSB
} mcp3421_resolution_t;

// PGA gain — determines effective input range (VREF = 2.048 V)
typedef enum {
    MCP3421_GAIN_1 = 0,  // ±2.048 V input range
    MCP3421_GAIN_2 = 1,  // ±1.024 V  ← default (covers 0–1 V sensor output)
    MCP3421_GAIN_4 = 2,  // ±0.512 V
    MCP3421_GAIN_8 = 3,  // ±0.256 V
} mcp3421_gain_t;

// Conversion mode
typedef enum {
    MCP3421_MODE_ONESHOT    = 0,
    MCP3421_MODE_CONTINUOUS = 1,  // ← default
} mcp3421_mode_t;

// ── Defaults for 0–1 V sensor input, 2 decimal places ───────────────────────
// 16-bit + gain 2× → 31.25 µV/LSB, ~32 000 counts over 1 V, 15 SPS
#define MCP3421_RES_DEFAULT   MCP3421_RES_16BIT
#define MCP3421_GAIN_DEFAULT  MCP3421_GAIN_2
#define MCP3421_MODE_DEFAULT  MCP3421_MODE_CONTINUOUS
#define MCP3421_CLK_DEFAULT   400000u

// Convenience initialiser — fills mcp3421_dev_config_t with defaults
#define MCP3421_DEV_CONFIG_DEFAULT() {      \
    .addr       = MCP3421_ADDR_DEFAULT,     \
    .resolution = MCP3421_RES_DEFAULT,      \
    .gain       = MCP3421_GAIN_DEFAULT,     \
    .mode       = MCP3421_MODE_DEFAULT,     \
}

// I2C bus wiring — one per physical bus
typedef struct {
    i2c_port_t  i2c_port;   // I2C_NUM_0 or I2C_NUM_1
    int         sda_pin;
    int         scl_pin;
    uint32_t    clk_hz;     // use MCP3421_CLK_DEFAULT (400 kHz)
} mcp3421_bus_config_t;

// Per-device ADC settings
typedef struct {
    uint8_t              addr;
    mcp3421_resolution_t resolution;
    mcp3421_gain_t       gain;
    mcp3421_mode_t       mode;
} mcp3421_dev_config_t;

// Device handle — one per chip; caller provides storage
typedef struct {
    mcp3421_dev_config_t cfg;
    i2c_port_t           port;
} mcp3421_handle_t;

// Initialise I2C bus and device. Installs I2C driver and writes config register.
// If the driver is already installed on this port (shared bus scenario), the
// install step is skipped gracefully.
esp_err_t mcp3421_init(mcp3421_handle_t           *h,
                       const mcp3421_bus_config_t *bus,
                       const mcp3421_dev_config_t *dev);

// Read signed raw ADC count (sign-extended to int32).
esp_err_t mcp3421_read_raw(const mcp3421_handle_t *h, int32_t *raw);

// Read input voltage in millivolts. Applies resolution LSB and PGA gain.
esp_err_t mcp3421_read_mv(const mcp3421_handle_t *h, float *mv);

// Change resolution / gain / mode at runtime without re-initialising the bus.
esp_err_t mcp3421_set_config(mcp3421_handle_t    *h,
                             mcp3421_resolution_t res,
                             mcp3421_gain_t       gain,
                             mcp3421_mode_t       mode);

// Uninstall I2C driver for this port.
void mcp3421_deinit(const mcp3421_handle_t *h);
