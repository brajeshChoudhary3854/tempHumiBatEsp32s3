#pragma once
#include <stdint.h>

// Voltage divider: R1 (top) + R2 (bottom)
// Vbat ─── R1 ─── ADC_PIN ─── R2 ─── GND
// ADC reads: Vadc = Vbat × R2 / (R1 + R2)
// Vbat   = Vadc × (R1 + R2) / R2

typedef struct {
    int      adc_channel;  // ADC1 channel (e.g. ADC1_CHANNEL_0 = GPIO1)
    int      adc_gpio;     // GPIO number for reference (e.g. 1)
    uint32_t r1_ohm;       // top resistor (battery side), e.g. 100000
    uint32_t r2_ohm;       // bottom resistor (GND side),  e.g.  47000
    uint32_t vbat_full_mv; // fully charged voltage, e.g. 4200 (Li-Ion)
    uint32_t vbat_empty_mv;// dead battery voltage,  e.g. 3000 (Li-Ion)
} battery_adc_config_t;

void    battery_adc_init(const battery_adc_config_t *cfg);
uint32_t battery_adc_read_mv(void);     // battery voltage in mV
uint8_t  battery_adc_read_percent(void);// 0–100%
