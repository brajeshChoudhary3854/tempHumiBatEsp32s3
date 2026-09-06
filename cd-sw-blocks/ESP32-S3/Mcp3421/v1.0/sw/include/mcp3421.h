#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"

// MCP3421 I2C addresses (set by ADR0 pin)
#define MCP3421_ADDR_GND  0x68 // ADR0 = GND
#define MCP3421_ADDR_VDD  0x69 // ADR0 = VDD (second IC)

typedef enum {
    MCP3421_RATE_240SPS = 0, // 12-bit, 240 samples/sec
    MCP3421_RATE_60SPS  = 1, // 14-bit,  60 samples/sec
    MCP3421_RATE_15SPS  = 2, // 16-bit,  15 samples/sec
    MCP3421_RATE_3SPS   = 3, // 18-bit,   3.75 samples/sec
} mcp3421_rate_t;

typedef enum {
    MCP3421_GAIN_1  = 0,
    MCP3421_GAIN_2  = 1,
    MCP3421_GAIN_4  = 2,
    MCP3421_GAIN_8  = 3,
} mcp3421_gain_t;

typedef struct {
    i2c_port_t    i2c_port;
    int           sda_pin;
    int           scl_pin;
    uint32_t      i2c_clk_hz; // 100000 or 400000
} mcp3421_bus_config_t;

typedef struct {
    uint8_t         addr;   // MCP3421_ADDR_GND or MCP3421_ADDR_VDD
    mcp3421_rate_t  rate;
    mcp3421_gain_t  gain;
} mcp3421_dev_config_t;

typedef struct {
    mcp3421_dev_config_t cfg;
    i2c_port_t           port;
} mcp3421_dev_t;

// Bus init — call once for shared I2C bus
int mcp3421_bus_init(const mcp3421_bus_config_t *bus);

// Per-device init — call for each IC
int mcp3421_init(mcp3421_dev_t *dev, const mcp3421_dev_config_t *cfg, i2c_port_t port);

// Read raw ADC value (blocking, waits for conversion)
// Returns 0 on success, -1 on error
int mcp3421_read_raw(const mcp3421_dev_t *dev, int32_t *raw);

// Read voltage in millivolts (accounts for gain and resolution)
int mcp3421_read_mv(const mcp3421_dev_t *dev, float *mv);
