#include "battery_adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <stddef.h>

#define OVERSAMPLE_COUNT  16  // average 16 readings to reduce noise

static battery_adc_config_t   s_cfg;
static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t         s_cali_handle;
static bool                      s_cali_ok = false;

void battery_adc_init(const battery_adc_config_t *cfg)
{
    s_cfg = *cfg;

    // ADC unit init
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);

    // Channel config: 12-bit, 11dB attenuation (0–3.3V range)
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(s_adc_handle, cfg->adc_channel, &chan_cfg);

    // Calibration (uses eFuse curve fitting if available)
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = cfg->adc_channel,
        .atten   = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    s_cali_ok = (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle) == ESP_OK);
}

uint32_t battery_adc_read_mv(void)
{
    int32_t sum = 0;
    int raw;
    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
        adc_oneshot_read(s_adc_handle, s_cfg.adc_channel, &raw);
        sum += raw;
    }
    int avg_raw = (int)(sum / OVERSAMPLE_COUNT);

    int vadc_mv;
    if (s_cali_ok) {
        adc_cali_raw_to_voltage(s_cali_handle, avg_raw, &vadc_mv);
    } else {
        // Fallback linear: 12-bit, 3300mV full scale
        vadc_mv = (avg_raw * 3300) / 4095;
    }

    // Scale back through voltage divider
    uint64_t vbat_mv = (uint64_t)vadc_mv * (s_cfg.r1_ohm + s_cfg.r2_ohm) / s_cfg.r2_ohm;
    return (uint32_t)vbat_mv;
}

uint8_t battery_adc_read_percent(void)
{
    uint32_t vbat = battery_adc_read_mv();
    if (vbat >= s_cfg.vbat_full_mv)  return 100;
    if (vbat <= s_cfg.vbat_empty_mv) return 0;
    uint32_t range   = s_cfg.vbat_full_mv - s_cfg.vbat_empty_mv;
    uint32_t above   = vbat - s_cfg.vbat_empty_mv;
    return (uint8_t)((above * 100) / range);
}
