# BLOCK PLAN: ESP32-S3 / BatteryAdc / v1.0

Generated: 2026-09-06
Platform:  ESP32-S3
Interface: Internal ADC1 (GPIO1)
Reference: None (new block)
Standard:  Follow cd-sw-blocks/CLAUDE.md

## Block Description
Battery voltage monitor using ESP32-S3 internal ADC1.
Voltage divider scales battery voltage to ADC range.
Returns battery voltage (mV) and charge percentage (0–100%).
Uses ESP-IDF ADC calibration and 16-sample oversampling.

## Hardware
- R1 = 100kΩ (battery + to ADC pin)
- R2 = 47kΩ  (ADC pin to GND)
- 100nF bypass cap (ADC pin to GND)
- Li-Ion or any battery type (configure full/empty voltage)

## Pin Reference
| Signal  | ESP32-S3 Pin | Notes              |
|---------|--------------|--------------------|
| Vbat_in | GPIO1        | ADC1_CH0, via divider |

## Development Steps

### Step 1: ADC Init + Calibration
- [x] `adc_oneshot_new_unit()` — ADC1 unit
- [x] `adc_oneshot_config_channel()` — 12-bit, 12dB atten
- [x] `adc_cali_create_scheme_curve_fitting()` — eFuse calibration
- **Test**: Read raw value, not all zeros

### Step 2: Battery Voltage Read
- [x] 16-sample oversample loop
- [x] raw → mV via calibration (or linear fallback)
- [x] Scale through divider: Vbat = Vadc × (R1+R2)/R2
- [x] `battery_adc_read_mv()` function
- **Test**: Apply 3.6V → reading ≈ 3600mV (±50mV)

### Step 3: Percentage Calculation
- [x] Linear interpolation between empty and full voltage
- [x] Clamped 0–100%
- [x] `battery_adc_read_percent()` function
- **Test**: At full charge → 100%, at 3.0V → 0%

## Usage Example
```c
battery_adc_config_t bat = {
    .adc_channel   = ADC1_CHANNEL_0, // GPIO1
    .adc_gpio      = 1,
    .r1_ohm        = 100000,
    .r2_ohm        = 47000,
    .vbat_full_mv  = 4200,
    .vbat_empty_mv = 3000,
};
battery_adc_init(&bat);

uint32_t vbat = battery_adc_read_mv();    // e.g. 3750
uint8_t  pct  = battery_adc_read_percent(); // e.g. 62
printf("Battery: %u mV (%u%%)\n", vbat, pct);
```
