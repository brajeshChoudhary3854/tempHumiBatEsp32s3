# Wiring — LedBlink / ESP32-S3

## Typical Connection

```
ESP32-S3                LED
---------              -----
GPIO48  ──── 330Ω ──── Anode (+)
GND     ────────────── Cathode (-)
```

## Notes
- GPIO48 = onboard RGB LED data pin on many ESP32-S3 devkits (use single color)
- For external LED: 330Ω current-limiting resistor required
- active_high = true: GPIO HIGH = LED ON
- active_high = false: GPIO LOW = LED ON (LED connected to VCC via resistor)

## Current limit
- ESP32-S3 GPIO max sink/source: 40mA per pin
- Typical LED current: 5–20mA
- Use 330Ω for 3.3V supply → ~(3.3 - 2.0) / 330 ≈ 4mA (safe)
