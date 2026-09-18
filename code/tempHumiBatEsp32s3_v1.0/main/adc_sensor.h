#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// ── Generic read function pointer ────────────────────────────────────────────
// Returns one raw signed ADC count into *raw.
// ctx: opaque pointer to the IC handle (e.g. mcp3421_handle_t*).
// The AdcSensor block never calls any IC driver directly — only through this.
typedef esp_err_t (*adc_read_fn_t)(void *ctx, int32_t *raw);

// ── Default values ────────────────────────────────────────────────────────────
#define ADC_SENSOR_ERROR_THRESHOLD_DEFAULT  3u   // consec_errors > 3 → unhealthy

// ── Error / status flags (sticky, OR'd on each event) ────────────────────────
typedef enum {
    ADC_SENSOR_FLAG_I2C_ERR   = (1u << 0),  // I2C bus or protocol error
    ADC_SENSOR_FLAG_NO_DEVICE = (1u << 1),  // device did not ACK (disconnected)
    ADC_SENSOR_FLAG_TIMEOUT   = (1u << 2),  // read timed out
    ADC_SENSOR_FLAG_BUF_WRAP  = (1u << 3),  // buffer full — oldest sample overwritten
} adc_sensor_flag_t;

// ── Live status snapshot ──────────────────────────────────────────────────────
typedef struct {
    uint32_t   total_reads;    // successful reads since init
    uint32_t   error_count;    // total errors since init
    uint32_t   consec_errors;  // consecutive errors — reset to 0 on every success
    esp_err_t  last_err;       // exact ESP-IDF error code of last failure
    uint32_t   flags;          // adc_sensor_flag_t bitmask (sticky)
} adc_sensor_status_t;

// ── Configuration ─────────────────────────────────────────────────────────────
// Caller allocates the int32_t buffer and the adc_sensor_t handle.
// No heap allocation inside the library.
typedef struct {
    adc_read_fn_t  read_fn;          // IC-agnostic read function pointer
    void          *read_ctx;         // opaque handle passed to read_fn
    int32_t       *buf;              // caller-allocated raw-count ring buffer
    uint16_t       buf_size;         // number of int32_t slots in buf
    uint32_t       scan_ms;          // sampling interval in milliseconds
    uint8_t        error_threshold;  // consec_errors > this → is_healthy() = false
    const char    *name;             // label used as FreeRTOS task name / debug tag
} adc_sensor_config_t;

// ── Handle ────────────────────────────────────────────────────────────────────
// Declare one per sensor. Caller provides storage (static or global).
// Do not access fields directly — use the API functions below.
typedef struct {
    adc_sensor_config_t  cfg;
    uint16_t             head;    // index of next write slot
    uint16_t             count;   // valid samples currently in buffer (0..buf_size)
    adc_sensor_status_t  status;
    SemaphoreHandle_t    mutex;
    TaskHandle_t         task;
} adc_sensor_t;

// ── API ───────────────────────────────────────────────────────────────────────

// Validate config, create mutex, start background scan task.
esp_err_t adc_sensor_init(adc_sensor_t *s, const adc_sensor_config_t *cfg);

// Stop scan task and release mutex. Safe to call after init.
void      adc_sensor_deinit(adc_sensor_t *s);

// Most recent raw sample. ESP_ERR_INVALID_STATE if buffer is still empty.
esp_err_t adc_sensor_get_latest(adc_sensor_t *s, int32_t *raw);

// Mean of all valid buffered samples (float to preserve sub-count precision).
// ESP_ERR_INVALID_STATE if buffer is empty.
esp_err_t adc_sensor_get_avg(adc_sensor_t *s, float *avg);

// Min / max of all valid buffered samples.
// ESP_ERR_INVALID_STATE if buffer is empty.
esp_err_t adc_sensor_get_min(adc_sensor_t *s, int32_t *min_raw);
esp_err_t adc_sensor_get_max(adc_sensor_t *s, int32_t *max_raw);

// Copy the n most recent raw samples into out[] (oldest first, newest last).
// Returns the number of samples actually copied (may be less than n).
uint16_t  adc_sensor_get_samples(adc_sensor_t *s, int32_t *out, uint16_t n);

// Current number of valid samples in the buffer (0..buf_size).
uint16_t  adc_sensor_get_count(adc_sensor_t *s);

// Discard all buffered samples.
void      adc_sensor_flush(adc_sensor_t *s);

// Returns true if consec_errors <= error_threshold.
bool      adc_sensor_is_healthy(adc_sensor_t *s);

// Fill *out with a snapshot of the status counters and flags.
void      adc_sensor_get_status(adc_sensor_t *s, adc_sensor_status_t *out);

// Clear sticky flags. Does not reset counters.
void      adc_sensor_clear_flags(adc_sensor_t *s);
