#include "adc_sensor.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>
#include <inttypes.h>

#define LOCK_MS   20u    // mutex wait timeout
#define TAG       "adc_sensor"

// ── Internal scan task ────────────────────────────────────────────────────────

static void scan_task(void *arg)
{
    adc_sensor_t *s = (adc_sensor_t *)arg;

    while (1) {
        int32_t raw = 0;
        esp_err_t err = s->cfg.read_fn(s->cfg.read_ctx, &raw);

        if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
            if (err == ESP_OK) {
                // Store into ring buffer at head, wrap on overflow
                if (s->count == s->cfg.buf_size) {
                    // Buffer full: oldest slot is being overwritten
                    s->status.flags |= ADC_SENSOR_FLAG_BUF_WRAP;
                } else {
                    s->count++;
                }
                s->cfg.buf[s->head] = raw;
                s->head = (s->head + 1) % s->cfg.buf_size;

                // Update status counters
                s->status.total_reads++;
                s->status.consec_errors = 0;

            } else {
                // Classify the error into sticky flags
                if (err == ESP_ERR_TIMEOUT) {
                    s->status.flags |= ADC_SENSOR_FLAG_TIMEOUT;
                } else if (err == ESP_ERR_NOT_FOUND) {
                    s->status.flags |= ADC_SENSOR_FLAG_NO_DEVICE;
                } else {
                    s->status.flags |= ADC_SENSOR_FLAG_I2C_ERR;
                }
                s->status.last_err = err;
                s->status.error_count++;
                if (s->status.consec_errors < UINT32_MAX)
                    s->status.consec_errors++;

                ESP_LOGW(TAG, "[%s] read failed (0x%x), consec=%"PRIu32,
                         s->cfg.name, err, s->status.consec_errors);
            }

            xSemaphoreGive(s->mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(s->cfg.scan_ms));
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

esp_err_t adc_sensor_init(adc_sensor_t *s, const adc_sensor_config_t *cfg)
{
    if (!s || !cfg || !cfg->read_fn || !cfg->buf || cfg->buf_size == 0)
        return ESP_ERR_INVALID_ARG;

    s->cfg   = *cfg;
    s->head  = 0;
    s->count = 0;
    memset(&s->status, 0, sizeof(s->status));

    s->mutex = xSemaphoreCreateMutex();
    if (!s->mutex) return ESP_ERR_NO_MEM;

    const char *task_name = cfg->name ? cfg->name : "adc_sensor";
    BaseType_t ok = xTaskCreate(scan_task, task_name, 2048, s, 5, &s->task);
    if (ok != pdPASS) {
        vSemaphoreDelete(s->mutex);
        s->mutex = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "[%s] init OK  buf=%u  scan=%"PRIu32"ms",
             task_name, cfg->buf_size, cfg->scan_ms);
    return ESP_OK;
}

void adc_sensor_deinit(adc_sensor_t *s)
{
    if (!s) return;
    if (s->task) {
        vTaskDelete(s->task);
        s->task = NULL;
    }
    if (s->mutex) {
        vSemaphoreDelete(s->mutex);
        s->mutex = NULL;
    }
}

esp_err_t adc_sensor_get_latest(adc_sensor_t *s, int32_t *raw)
{
    if (!s || !raw) return ESP_ERR_INVALID_ARG;

    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        if (s->count > 0) {
            // head points to NEXT write slot; last written is head-1 (wrapped)
            uint16_t last = (s->head + s->cfg.buf_size - 1) % s->cfg.buf_size;
            *raw = s->cfg.buf[last];
            ret = ESP_OK;
        }
        xSemaphoreGive(s->mutex);
    }
    return ret;
}

esp_err_t adc_sensor_get_avg(adc_sensor_t *s, float *avg)
{
    if (!s || !avg) return ESP_ERR_INVALID_ARG;

    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        if (s->count > 0) {
            uint16_t  n    = s->count;
            uint16_t  size = s->cfg.buf_size;
            // Oldest entry is at (head - count + size) % size
            uint16_t  start = (uint16_t)((s->head + size - n) % size);
            double    sum   = 0.0;
            for (uint16_t i = 0; i < n; i++)
                sum += (double)s->cfg.buf[(start + i) % size];
            *avg = (float)(sum / (double)n);
            ret = ESP_OK;
        }
        xSemaphoreGive(s->mutex);
    }
    return ret;
}

esp_err_t adc_sensor_get_min(adc_sensor_t *s, int32_t *min_raw)
{
    if (!s || !min_raw) return ESP_ERR_INVALID_ARG;

    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        if (s->count > 0) {
            uint16_t n     = s->count;
            uint16_t size  = s->cfg.buf_size;
            uint16_t start = (uint16_t)((s->head + size - n) % size);
            int32_t  mn    = s->cfg.buf[start];
            for (uint16_t i = 1; i < n; i++) {
                int32_t v = s->cfg.buf[(start + i) % size];
                if (v < mn) mn = v;
            }
            *min_raw = mn;
            ret = ESP_OK;
        }
        xSemaphoreGive(s->mutex);
    }
    return ret;
}

esp_err_t adc_sensor_get_max(adc_sensor_t *s, int32_t *max_raw)
{
    if (!s || !max_raw) return ESP_ERR_INVALID_ARG;

    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        if (s->count > 0) {
            uint16_t n     = s->count;
            uint16_t size  = s->cfg.buf_size;
            uint16_t start = (uint16_t)((s->head + size - n) % size);
            int32_t  mx    = s->cfg.buf[start];
            for (uint16_t i = 1; i < n; i++) {
                int32_t v = s->cfg.buf[(start + i) % size];
                if (v > mx) mx = v;
            }
            *max_raw = mx;
            ret = ESP_OK;
        }
        xSemaphoreGive(s->mutex);
    }
    return ret;
}

uint16_t adc_sensor_get_samples(adc_sensor_t *s, int32_t *out, uint16_t n)
{
    if (!s || !out || n == 0) return 0;

    uint16_t copied = 0;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        uint16_t avail = s->count;
        if (n > avail) n = avail;
        uint16_t size  = s->cfg.buf_size;
        uint16_t start = (uint16_t)((s->head + size - avail) % size);
        // Produce oldest-first: start at the oldest valid entry
        uint16_t skip  = (avail > n) ? (avail - n) : 0;
        uint16_t begin = (uint16_t)((start + skip) % size);
        for (uint16_t i = 0; i < n; i++)
            out[copied++] = s->cfg.buf[(begin + i) % size];
        xSemaphoreGive(s->mutex);
    }
    return copied;
}

uint16_t adc_sensor_get_count(adc_sensor_t *s)
{
    if (!s) return 0;
    uint16_t cnt = 0;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        cnt = s->count;
        xSemaphoreGive(s->mutex);
    }
    return cnt;
}

void adc_sensor_flush(adc_sensor_t *s)
{
    if (!s) return;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        s->head  = 0;
        s->count = 0;
        xSemaphoreGive(s->mutex);
    }
}

bool adc_sensor_is_healthy(adc_sensor_t *s)
{
    if (!s) return false;
    bool ok = false;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        ok = (s->status.consec_errors <= (uint32_t)s->cfg.error_threshold);
        xSemaphoreGive(s->mutex);
    }
    return ok;
}

void adc_sensor_get_status(adc_sensor_t *s, adc_sensor_status_t *out)
{
    if (!s || !out) return;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        *out = s->status;
        xSemaphoreGive(s->mutex);
    }
}

void adc_sensor_clear_flags(adc_sensor_t *s)
{
    if (!s) return;
    if (xSemaphoreTake(s->mutex, pdMS_TO_TICKS(LOCK_MS)) == pdTRUE) {
        s->status.flags = 0;
        xSemaphoreGive(s->mutex);
    }
}
