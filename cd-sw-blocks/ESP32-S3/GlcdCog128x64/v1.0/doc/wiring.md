# Wiring — GlcdCog128x64 / ESP32-S3

## Controller: ST7565R (COG type, 128×64 pixels)

## SPI Connection (4-wire)

```
ESP32-S3       GLCD Module (ST7565)
---------      -------------------
GPIO12  ─────  SCK  (SPI Clock)
GPIO11  ─────  SDA  (MOSI / SI)
GPIO10  ─────  CS   (Chip Select, active low)
GPIO9   ─────  A0   (Data/Command: HIGH=data, LOW=cmd)
GPIO8   ─────  RST  (Reset, active low)
3.3V    ─────  VDD  (logic power)
GND     ─────  GND
```

## Backlight (if fitted)
```
3.3V ─── 33Ω ─── LED+ (A)
GND  ───────────  LED- (K)
```

## Notes
- SPI max clock: 10MHz for ST7565 (use 4–8MHz to start)
- VDD: 3.3V (module has onboard LDO or direct 3.3V)
- RST pin: if LCD module has RST exposed, connect to GPIO8
  - If no RST pin on module: set rst_pin = -1 in config
- A0 (DC) pin: HIGH = pixel data, LOW = command byte
- DO NOT exceed 3.3V on logic pins
- Module contrast pot (VR1) may need adjustment if fitted,
  or use glcd_cog_set_contrast() in software

## Module Power Sequence
1. VDD on → wait 10ms
2. RST low 10ms → RST high
3. Init commands → display on
