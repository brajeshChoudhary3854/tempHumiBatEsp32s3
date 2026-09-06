#include "mcp3421.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// MCP3421 config byte bits
#define MCP3421_RDY     (1 << 7) // 1 = conversion in progress / trigger
#define MCP3421_OC      (1 << 4) // 0 = one-shot, 1 = continuous
#define MCP3421_RATE_POS 2
#define MCP3421_GAIN_POS 0

// Full-scale voltage reference = 2048mV (2.048V)
#define MCP3421_VREF_MV  2048.0f

// Counts full-scale per resolution
static const int32_t s_fscale[] = { 2048, 8192, 32768, 131072 }; // 12,14,16,18 bit

// Conversion time in ms (a bit over spec to be safe)
static const uint32_t s_conv_ms[] = { 5, 17, 67, 267 };

int mcp3421_bus_init(const mcp3421_bus_config_t *bus)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = bus->sda_pin,
        .scl_io_num       = bus->scl_pin,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = bus->i2c_clk_hz,
    };
    i2c_param_config(bus->i2c_port, &conf);
    return i2c_driver_install(bus->i2c_port, I2C_MODE_MASTER, 0, 0, 0);
}

int mcp3421_init(mcp3421_dev_t *dev, const mcp3421_dev_config_t *cfg, i2c_port_t port)
{
    dev->cfg  = *cfg;
    dev->port = port;

    // Write config byte: continuous mode, chosen rate and gain
    uint8_t config = MCP3421_RDY | MCP3421_OC
                   | (cfg->rate << MCP3421_RATE_POS)
                   | (cfg->gain << MCP3421_GAIN_POS);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (cfg->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, config, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK) ? 0 : -1;
}

int mcp3421_read_raw(const mcp3421_dev_t *dev, int32_t *raw)
{
    // Trigger one-shot conversion
    uint8_t trig = MCP3421_RDY
                 | (dev->cfg.rate << MCP3421_RATE_POS)
                 | (dev->cfg.gain << MCP3421_GAIN_POS);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->cfg.addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, trig, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(dev->port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    // Wait for conversion
    vTaskDelay(pdMS_TO_TICKS(s_conv_ms[dev->cfg.rate]));

    // Read result: 3 bytes for 18-bit, 2 bytes for others + config byte
    uint8_t buf[4] = {0};
    uint8_t read_len = (dev->cfg.rate == MCP3421_RATE_3SPS) ? 4 : 3;

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->cfg.addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf, read_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(dev->port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) return -1;

    int32_t result;
    if (dev->cfg.rate == MCP3421_RATE_3SPS) {
        // 18-bit: bytes[0]=MSB (2 bits), bytes[1], bytes[2]; bytes[3]=config
        result = ((int32_t)(buf[0] & 0x03) << 16) | ((int32_t)buf[1] << 8) | buf[2];
        if (result & 0x20000) result |= 0xFFFC0000; // sign extend 18-bit
    } else {
        // 12/14/16-bit: bytes[0]=MSB, bytes[1]=LSB; bytes[2]=config
        result = ((int32_t)buf[0] << 8) | buf[1];
        int bits = 12 + (dev->cfg.rate * 2);
        int sign_bit = 1 << (bits - 1);
        if (result & sign_bit) result |= (~((sign_bit << 1) - 1)); // sign extend
    }

    *raw = result;
    return 0;
}

int mcp3421_read_mv(const mcp3421_dev_t *dev, float *mv)
{
    int32_t raw;
    int ret = mcp3421_read_raw(dev, &raw);
    if (ret != 0) return -1;

    int gain_factor = 1 << dev->cfg.gain; // 1,2,4,8
    float lsb_mv = (MCP3421_VREF_MV * 2.0f) / ((float)s_fscale[dev->cfg.rate] * gain_factor);
    *mv = raw * lsb_mv;
    return 0;
}
