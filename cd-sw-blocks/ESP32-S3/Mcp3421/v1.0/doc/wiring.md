# Wiring — Mcp3421 / ESP32-S3 (2 ICs)

## I2C Connection (shared bus, 2 MCP3421 ICs)

```
ESP32-S3        MCP3421 IC1 (addr 0x68)    MCP3421 IC2 (addr 0x69)
---------       -----------------------    -----------------------
GPIO5  ─────── SDA ────────────────────── SDA
GPIO6  ─────── SCL ────────────────────── SCL
3.3V   ─────── VDD ────────────────────── VDD
GND    ─────── GND ────────────────────── GND
GND    ─────── ADR0 (IC1 → addr 0x68)
3.3V   ─────────────────────────────────  ADR0 (IC2 → addr 0x69)
```

## MCP3421 Differential Input

```
MCP3421 (each IC)
-----------------
IN+  ──── Signal positive (0 to +2.048V max)
IN-  ──── Signal reference (GND for single-ended)
```

## Pull-up Resistors
- SDA and SCL: 4.7kΩ to 3.3V (external, shared on bus)

## Notes
- MCP3421 input range: ±2.048V differential (full scale)
- For single-ended: connect IN- to GND, IN+ to signal (0–2.048V)
- With voltage divider: scale signal to 0–2.048V before ADC input
- Two ICs share I2C bus — different addresses set by ADR0 pin
- ADR0 = GND → 0x68, ADR0 = VDD → 0x69
- Only 2 unique addresses possible with MCP3421; use MCP3422 (4-ch)
  or MCP3424 (4-ch, 4 addresses) if more channels needed

## Example: NTC Temperature Sensing (with voltage divider)
```
3.3V ── 10kΩ (fixed) ── IN+ ── NTC (10k@25°C) ── GND
                              IN-  ── GND
```
