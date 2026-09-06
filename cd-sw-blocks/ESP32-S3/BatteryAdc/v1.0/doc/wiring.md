# Wiring — BatteryAdc / ESP32-S3

## Voltage Divider Circuit

```
Li-Ion Battery (+)
      │
     R1 = 100kΩ
      │
      ├─────── GPIO1 (ADC1_CH0, ADC input)
      │
     R2 = 47kΩ
      │
     GND
```

## Divider Ratio
- Vadc = Vbat × R2 / (R1 + R2)
- Vadc = Vbat × 47000 / 147000 = Vbat × 0.3197
- At full charge (4.2V): Vadc = 4200 × 0.32 = 1342mV ✓ (within 3.3V ADC range)
- At 3.0V: Vadc = 3000 × 0.32 = 959mV ✓

## Battery Type Reference
| Type        | Full (mV) | Empty (mV) |
|-------------|-----------|------------|
| Li-Ion 1S   | 4200      | 3000       |
| Li-Po  1S   | 4200      | 3000       |
| 3× AA NiMH  | 3900      | 3000       |
| 3× AA Alk   | 4500      | 3300       |

## Notes
- ESP32-S3 internal ADC: ADC1_CHANNEL_0 = GPIO1
- ADC1 safe to use alongside WiFi (ADC2 not safe during WiFi)
- Use 12dB attenuation (0–3.3V range)
- Use 16-sample oversampling for stable reading
- Add 100nF bypass cap from ADC pin to GND for noise filtering
- DO NOT connect battery directly to ADC — always use divider
