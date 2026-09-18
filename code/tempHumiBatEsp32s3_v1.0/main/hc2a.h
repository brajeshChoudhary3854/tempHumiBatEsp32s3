#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "mcp3421.h"
#include "adc_sensor.h"

// ── HC2A-S3 sensor driver ─────────────────────────────────────────────────────
// Rotronic HygroClip2 Advanced — analog output variant
//
// Sensor outputs:
//   Pin 5  Humidity     0...1 V = 0...100 %RH
//   Pin 6  Temperature  0...1 V = -40...60 °C
//
// Each output is read by one MCP3421 ADC (16-bit, gain 2×, 0–1 V range).
// Each MCP3421 runs on its own I2C bus so both can use the fixed address 0x68.
// Raw counts are buffered by an AdcSensor ring buffer.
// Conversion to physical units happens in get/get_avg functions.

// ── Defaults ─────────────────────────────────────────────────────────────────
#define HC2A_BUF_SIZE_DEFAULT   32u
#define HC2A_SCAN_MS_DEFAULT    200u    // 5 Hz — well above 15 s sensor response time

// ── Configuration ─────────────────────────────────────────────────────────────
// Caller allocates the two int32_t buffers and the hc2a_t handle.
// No heap allocation inside this driver.
typedef struct {
    mcp3421_bus_config_t temp_bus;        // I2C bus wired to HC2A pin 6 (temp output)
    mcp3421_bus_config_t humi_bus;        // I2C bus wired to HC2A pin 5 (humi output)
    int32_t             *temp_buf;        // caller-allocated ring buffer, temperature
    int32_t             *humi_buf;        // caller-allocated ring buffer, humidity
    uint16_t             buf_size;        // slots in each buffer
    uint32_t             scan_ms;         // ADC sampling interval in ms
    uint8_t              error_threshold; // passed to each AdcSensor instance
} hc2a_config_t;

// ── Handle ────────────────────────────────────────────────────────────────────
typedef struct {
    mcp3421_handle_t temp_ic;
    mcp3421_handle_t humi_ic;
    adc_sensor_t     temp_sensor;
    adc_sensor_t     humi_sensor;
} hc2a_t;

// ── API ───────────────────────────────────────────────────────────────────────

// Init both MCP3421 ADCs and start background scan tasks.
esp_err_t hc2a_init(hc2a_t *h, const hc2a_config_t *cfg);

// Stop scan tasks and release resources.
void      hc2a_deinit(hc2a_t *h);

// Most recent temperature sample converted to °C.
// Returns ESP_ERR_INVALID_STATE if buffer is still empty.
esp_err_t hc2a_get_temp(hc2a_t *h, float *temp_c);

// Most recent humidity sample converted to %RH.
esp_err_t hc2a_get_humi(hc2a_t *h, float *humi_pct);

// Mean of all buffered temperature samples, converted to °C.
esp_err_t hc2a_get_temp_avg(hc2a_t *h, float *temp_c);

// Mean of all buffered humidity samples, converted to %RH.
esp_err_t hc2a_get_humi_avg(hc2a_t *h, float *humi_pct);

// true if temperature ADC has no recent errors.
bool      hc2a_temp_healthy(hc2a_t *h);

// true if humidity ADC has no recent errors.
bool      hc2a_humi_healthy(hc2a_t *h);

// Get underlying AdcSensor status (error flags, consec_errors, etc.).
void      hc2a_get_temp_status(hc2a_t *h, adc_sensor_status_t *out);
void      hc2a_get_humi_status(hc2a_t *h, adc_sensor_status_t *out);
