#include "hc2a.h"
#include "esp_log.h"

#define TAG "hc2a"

// ── Conversion constants ──────────────────────────────────────────────────────
// MCP3421 LSB at 16-bit resolution, gain 2×:
//   LSB = VREF / (fullscale × gain) = 2048 / (32768 × 2) = 0.03125 mV = 31.25 µV
//
// HC2A-S3 output scaling:
//   Temperature: 0 V = -40 °C,  1 V = 60 °C  → span = 100 °C / 1000 mV → 0.1 °C/mV
//   Humidity:    0 V = 0 %RH,   1 V = 100 %RH → span = 100 %RH / 1000 mV → 0.1 %RH/mV
//
// Combined (counts → physical):
//   raw_to_temp: count × 0.03125 mV/count × 0.1 °C/mV  = count × 0.003125 °C/count
//   raw_to_humi: count × 0.03125 mV/count × 0.1 %RH/mV = count × 0.003125 %RH/count

#define HC2A_COUNTS_PER_DEG  (1.0f / 0.003125f)   // ~320 counts per °C
#define HC2A_SCALE           0.003125f             // °C or %RH per count
#define HC2A_TEMP_OFFSET     (-40.0f)              // °C at 0 V (0 counts)

static inline float raw_to_temp(float raw_avg)
{
    return raw_avg * HC2A_SCALE + HC2A_TEMP_OFFSET;
}

static inline float raw_to_humi(float raw_avg)
{
    return raw_avg * HC2A_SCALE;
}

// ── MCP3421 function pointer adapter ─────────────────────────────────────────
// Matches adc_read_fn_t: esp_err_t fn(void *ctx, int32_t *raw)

static esp_err_t s_mcp3421_read_raw_fn(void *ctx, int32_t *raw)
{
    return mcp3421_read_raw((mcp3421_handle_t *)ctx, raw);
}

// ── Public API ────────────────────────────────────────────────────────────────

esp_err_t hc2a_init(hc2a_t *h, const hc2a_config_t *cfg)
{
    if (!h || !cfg || !cfg->temp_buf || !cfg->humi_buf || cfg->buf_size == 0)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err;

    // MCP3421 device config: 16-bit, gain 2× (0–1 V range), continuous
    static const mcp3421_dev_config_t s_dev = MCP3421_DEV_CONFIG_DEFAULT();

    // ── Init temperature ADC (HC2A pin 6 → IC1) ──────────────────────────────
    err = mcp3421_init(&h->temp_ic, &cfg->temp_bus, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "temp MCP3421 init failed: 0x%x", err);
        return err;
    }

    adc_sensor_config_t temp_sensor_cfg = {
        .read_fn         = s_mcp3421_read_raw_fn,
        .read_ctx        = &h->temp_ic,
        .buf             = cfg->temp_buf,
        .buf_size        = cfg->buf_size,
        .scan_ms         = cfg->scan_ms,
        .error_threshold = cfg->error_threshold,
        .name            = "hc2a_temp",
    };
    err = adc_sensor_init(&h->temp_sensor, &temp_sensor_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "temp AdcSensor init failed: 0x%x", err);
        mcp3421_deinit(&h->temp_ic);
        return err;
    }

    // ── Init humidity ADC (HC2A pin 5 → IC2) ─────────────────────────────────
    err = mcp3421_init(&h->humi_ic, &cfg->humi_bus, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "humi MCP3421 init failed: 0x%x", err);
        adc_sensor_deinit(&h->temp_sensor);
        mcp3421_deinit(&h->temp_ic);
        return err;
    }

    adc_sensor_config_t humi_sensor_cfg = {
        .read_fn         = s_mcp3421_read_raw_fn,
        .read_ctx        = &h->humi_ic,
        .buf             = cfg->humi_buf,
        .buf_size        = cfg->buf_size,
        .scan_ms         = cfg->scan_ms,
        .error_threshold = cfg->error_threshold,
        .name            = "hc2a_humi",
    };
    err = adc_sensor_init(&h->humi_sensor, &humi_sensor_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "humi AdcSensor init failed: 0x%x", err);
        adc_sensor_deinit(&h->temp_sensor);
        mcp3421_deinit(&h->temp_ic);
        mcp3421_deinit(&h->humi_ic);
        return err;
    }

    ESP_LOGI(TAG, "HC2A-S3 ready — scan %"PRIu32" ms  buf %u slots",
             cfg->scan_ms, cfg->buf_size);
    return ESP_OK;
}

void hc2a_deinit(hc2a_t *h)
{
    if (!h) return;
    adc_sensor_deinit(&h->temp_sensor);
    adc_sensor_deinit(&h->humi_sensor);
    mcp3421_deinit(&h->temp_ic);
    mcp3421_deinit(&h->humi_ic);
}

esp_err_t hc2a_get_temp(hc2a_t *h, float *temp_c)
{
    if (!h || !temp_c) return ESP_ERR_INVALID_ARG;
    int32_t raw;
    esp_err_t err = adc_sensor_get_latest(&h->temp_sensor, &raw);
    if (err == ESP_OK) *temp_c = raw_to_temp((float)raw);
    return err;
}

esp_err_t hc2a_get_humi(hc2a_t *h, float *humi_pct)
{
    if (!h || !humi_pct) return ESP_ERR_INVALID_ARG;
    int32_t raw;
    esp_err_t err = adc_sensor_get_latest(&h->humi_sensor, &raw);
    if (err == ESP_OK) *humi_pct = raw_to_humi((float)raw);
    return err;
}

esp_err_t hc2a_get_temp_avg(hc2a_t *h, float *temp_c)
{
    if (!h || !temp_c) return ESP_ERR_INVALID_ARG;
    float avg;
    esp_err_t err = adc_sensor_get_avg(&h->temp_sensor, &avg);
    if (err == ESP_OK) *temp_c = raw_to_temp(avg);
    return err;
}

esp_err_t hc2a_get_humi_avg(hc2a_t *h, float *humi_pct)
{
    if (!h || !humi_pct) return ESP_ERR_INVALID_ARG;
    float avg;
    esp_err_t err = adc_sensor_get_avg(&h->humi_sensor, &avg);
    if (err == ESP_OK) *humi_pct = raw_to_humi(avg);
    return err;
}

bool hc2a_temp_healthy(hc2a_t *h)
{
    return h && adc_sensor_is_healthy(&h->temp_sensor);
}

bool hc2a_humi_healthy(hc2a_t *h)
{
    return h && adc_sensor_is_healthy(&h->humi_sensor);
}

void hc2a_get_temp_status(hc2a_t *h, adc_sensor_status_t *out)
{
    if (h && out) adc_sensor_get_status(&h->temp_sensor, out);
}

void hc2a_get_humi_status(hc2a_t *h, adc_sensor_status_t *out)
{
    if (h && out) adc_sensor_get_status(&h->humi_sensor, out);
}
