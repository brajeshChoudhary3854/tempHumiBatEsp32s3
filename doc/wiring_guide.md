# Wiring Guide — tempHumiBatEsp32s3_v1

## Complete Pin Assignment (ESP32-S3)

| GPIO | Function        | Block           | Notes                              |
|------|-----------------|-----------------|------------------------------------|
| 1    | BAT_ADC         | BatteryAdc      | 100k+47k divider from LiIon        |
| 2    | SW_UP           | InputSwitch     | Active low, internal pull-up       |
| 3    | SW_DOWN         | InputSwitch     | Active low, internal pull-up       |
| 4    | ADC2_SDA        | Mcp3421 IC2     | I2C bus 2, 4.7kΩ pull-up to 3.3V  |
| 5    | ADC2_SCL        | Mcp3421 IC2     | I2C bus 2, 4.7kΩ pull-up to 3.3V  |
| 7    | SW_BACK         | InputSwitch     | Active low, internal pull-up       |
| 8    | ADC1_SDA        | Mcp3421 IC1     | I2C bus 1, 4.7kΩ pull-up to 3.3V  |
| 9    | ADC1_SCL        | Mcp3421 IC1     | I2C bus 1, 4.7kΩ pull-up to 3.3V  |
| 10   | GLCD_CS         | GlcdCog128x64   | SPI chip select                    |
| 11   | GLCD_RST        | GlcdCog128x64   | Active low reset                   |
| 12   | GLCD_DC (A0)    | GlcdCog128x64   | Data/Command select                |
| 13   | SPI_SCK         | GlcdCog128x64   | SPI clock                          |
| 14   | SPI_MOSI        | GlcdCog128x64   | SPI data out                       |
| 39   | BAT_SENSE_EN    | BatteryAdc      | HIGH = enable sense divider        |
| 40   | BL_CTRL         | GlcdCog128x64   | LCD backlight, active low (LOW=ON) |
| 48   | LED             | LedBlink        | Active high, 330Ω to LED           |

## Power
| Net      | Voltage | Source        |
|----------|---------|---------------|
| VDD      | 3.3V    | LDO from LiIon|
| VBAT     | 3.0–4.2V| Li-Ion cell   |
| GND      | 0V      | Common        |

## MCP3421 I2C Buses

| IC  | I2C Bus   | SDA    | SCL    | Address |
|-----|-----------|--------|--------|---------|
| IC1 | I2C_NUM_0 | GPIO 8 | GPIO 9 | 0x68    |
| IC2 | I2C_NUM_1 | GPIO 4 | GPIO 5 | 0x68    |

Each device is on its own bus — both use fixed address 0x68.
