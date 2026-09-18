# Pin Details — tempHumiBatEsp32s3 v1.0

## ESP32-S3 Module — Full Connection Table

| GPIO | Net / Signal  | Peripheral      | Direction | Notes                              |
|------|---------------|-----------------|-----------|------------------------------------|
| 1    | BAT_ADC       | BatteryAdc      | Input     | Voltage divider: 100 kΩ + 47 kΩ from VBAT |
| 2    | SW_UP         | Tactile switch  | Input     | Active low — internal pull-up enabled      |
| 3    | SW_DOWN       | Tactile switch  | Input     | Active low — internal pull-up enabled      |
| 4    | SW_SELECT     | Tactile switch  | Input     | Active low — internal pull-up enabled      |
| 5    | I2C_SDA       | MCP3421 IC1+IC2 | Bidir     | 4.7 kΩ pull-up to 3.3 V                   |
| 6    | I2C_SCL       | MCP3421 IC1+IC2 | Output    | 4.7 kΩ pull-up to 3.3 V                   |
| 7    | SW_BACK       | Tactile switch  | Input     | Active low — internal pull-up enabled      |
| 10   | GLCD_CS       | ST7565R GLCD    | Output    | SPI chip select, active low                |
| 11   | GLCD_RST      | ST7565R GLCD    | Output    | Active low hardware reset                  |
| 12   | GLCD_DC (A0)  | ST7565R GLCD    | Output    | LOW = command, HIGH = data                 |
| 13   | SPI_SCK       | ST7565R GLCD    | Output    | SPI2_HOST clock, 8 MHz                     |
| 14   | SPI_MOSI      | ST7565R GLCD    | Output    | SPI2_HOST data out                         |
| 40   | BL_CTRL       | GLCD backlight  | Output    | PWM active-low (LOW = full bright)         |
| 48   | LED           | Status LED      | Output    | Active high — 330 Ω series resistor        |

---

## MCP3421 — 18-bit I2C ADC (SOT-23-6)

```
        ┌─────────┐
  IN+  ─┤ 1     6 ├─ VDD  3.3 V
  IN-  ─┤ 2     5 ├─ SCL  → GPIO 6
  VSS  ─┤ 3     4 ├─ SDA  → GPIO 5
        └─────────┘
```

| Pin | Name | Connect to        | Notes                                    |
|-----|------|-------------------|------------------------------------------|
| 1   | IN+  | Signal positive   | Analog input +                           |
| 2   | IN−  | Signal negative / GND | Analog input − (GND for single-ended)|
| 3   | VSS  | GND               | Ground                                   |
| 4   | SDA  | GPIO 5            | I2C data — shared with IC2               |
| 5   | SCL  | GPIO 6            | I2C clock — shared with IC2              |
| 6   | VDD  | 3.3 V             | Decoupling cap 100 nF to GND             |

### I2C Addresses (ADR0 pin selection)

| Device | ADR0 tie | 7-bit Address |
|--------|----------|---------------|
| IC1    | GND      | 0x68          |
| IC2    | VDD      | 0x69          |

> For two devices on the same bus, use **MCP3422 / MCP3423 / MCP3424** — identical SOT-23 footprint, adds ADR0 address-select pin.  Plain MCP3421 has no address pin (fixed 0x68 only).

---

## ST7565R GLCD Module — 128 × 64 COG

```
Module header (typical 7-pin or 8-pin FPC/header)

  VDD  ─── 3.3 V
  GND  ─── GND
  CS   ─── GPIO 10   (GLCD_CS)
  RST  ─── GPIO 11   (GLCD_RST)
  A0   ─── GPIO 12   (GLCD_DC)
  SCK  ─── GPIO 13   (SPI_SCK)
  SI   ─── GPIO 14   (SPI_MOSI)
  BL   ─── GPIO 40   (BL_CTRL, via transistor or FET)
```

| Signal | GPIO | Notes                                              |
|--------|------|----------------------------------------------------|
| CS     | 10   | Active low chip select — SPI hardware managed      |
| RST    | 11   | Pull low 10 ms at power-on for hardware reset      |
| A0/DC  | 12   | 0 = command byte, 1 = display data byte            |
| SCK    | 13   | SPI clock, CPOL=0 CPHA=0, max 20 MHz              |
| SI/MOSI| 14   | Serial data, MSB first                             |
| BL     | 40   | Backlight enable — active low PWM (5 kHz, 8-bit)  |

**Column offset:** ST7565R has 132 column drivers; only 128 are visible. `col_offset = 4` with `ADC = 0xA1` (reverse).

---

## Tactile Switches

All four switches are wired identically:

```
  GPIO (input, pull-up enabled)
       │
       ┤  switch
       │
      GND
```

| GPIO | Switch   | Function in menu     |
|------|----------|----------------------|
| 2    | SW_UP    | Scroll up / increment|
| 3    | SW_DOWN  | Scroll down / decrement|
| 4    | SW_SELECT| Confirm / enter      |
| 7    | SW_BACK  | Back / cancel        |

Debounce: 20 ms software debounce in `InputSwitch` FSM.
Hold time: UP/DOWN 1000 ms, SELECT 1500 ms, BACK hold disabled.

---

## Battery ADC

```
  VBAT (3.0–4.2 V)
       │
     100 kΩ
       │
       ├──── GPIO 1 (ADC1_CH0)
       │
      47 kΩ
       │
      GND
```

Divider ratio: 47k / (100k + 47k) = **0.32**
At VBAT = 4.2 V → ADC input = 1.34 V (within ESP32-S3 ADC range 0–3.3 V)
At VBAT = 3.0 V → ADC input = 0.96 V

---

## Power

| Net   | Voltage   | Source                      |
|-------|-----------|-----------------------------|
| VBAT  | 3.0–4.2 V | Li-Ion cell                 |
| VDD   | 3.3 V     | LDO regulator from VBAT     |
| GND   | 0 V       | Common                      |

I2C pull-up resistors (4.7 kΩ) connect SDA and SCL to VDD (3.3 V).
