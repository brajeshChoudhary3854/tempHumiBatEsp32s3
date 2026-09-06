# Wiring Guide — tempHumiBatEsp32s3_v1

## Complete Pin Assignment (ESP32-S3)

| GPIO | Function      | Block           | Notes                        |
|------|---------------|-----------------|------------------------------|
| 1    | BAT_ADC       | BatteryAdc      | 100k+47k divider from LiIon  |
| 2    | SW_UP         | InputSwitch     | Active low, internal pull-up |
| 3    | SW_DOWN       | InputSwitch     | Active low, internal pull-up |
| 4    | SW_SELECT     | InputSwitch     | Active low, internal pull-up |
| 5    | I2C_SDA       | Mcp3421 (both)  | 4.7kΩ pull-up to 3.3V       |
| 6    | I2C_SCL       | Mcp3421 (both)  | 4.7kΩ pull-up to 3.3V       |
| 7    | SW_BACK       | InputSwitch     | Active low, internal pull-up |
| 8    | GLCD_RST      | GlcdCog128x64   | Active low reset             |
| 9    | GLCD_DC       | GlcdCog128x64   | Data/Command select          |
| 10   | GLCD_CS       | GlcdCog128x64   | SPI chip select              |
| 11   | SPI_MOSI      | GlcdCog128x64   | SPI data out                 |
| 12   | SPI_SCK       | GlcdCog128x64   | SPI clock                    |
| 48   | LED           | LedBlink        | Active high, 330Ω to LED     |

## Power
| Net      | Voltage | Source        |
|----------|---------|---------------|
| VDD      | 3.3V    | LDO from LiIon|
| VBAT     | 3.0–4.2V| Li-Ion cell   |
| GND      | 0V      | Common        |

## MCP3421 Addresses
| IC  | ADR0 | I2C Address |
|-----|------|-------------|
| IC1 | GND  | 0x68        |
| IC2 | VDD  | 0x69        |
