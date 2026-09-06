# BLOCK PLAN: ESP32-S3 / Mcp3421 / v1.0

Generated: 2026-09-06
Platform:  ESP32-S3
Interface: I2C
Reference: None (new block)
Standard:  Follow cd-sw-blocks/CLAUDE.md

## Block Description
MCP3421 18-bit I2C ADC driver. Supports 2 ICs on same bus (addresses 0x68, 0x69).
Programmable gain (1/2/4/8×) and sample rate (240/60/15/3.75 SPS).
One-shot read mode — trigger, wait, read result in millivolts.

## Hardware
- 2× MCP3421 ICs
- 4.7kΩ pull-up on SDA and SCL (shared)
- 3.3V supply

## Pin Reference
| Signal | ESP32-S3 Pin | Notes                   |
|--------|--------------|-------------------------|
| SDA    | GPIO5        | I2C data, 4.7kΩ pull-up |
| SCL    | GPIO6        | I2C clock, 4.7kΩ pull-up |
| ADR0_1 | GND (IC1)   | Sets address 0x68       |
| ADR0_2 | 3.3V (IC2)  | Sets address 0x69       |

## Development Steps

### Step 1: I2C Bus Init
- [x] `mcp3421_bus_init()` — i2c_param_config + i2c_driver_install
- **Test**: No error on init, I2C bus ready

### Step 2: Device Init + Config Write
- [x] `mcp3421_init()` — write config byte (rate, gain, continuous mode)
- **Test**: No I2C NACK (device responds)

### Step 3: Raw ADC Read
- [x] Trigger one-shot, wait for conversion time
- [x] Read 2–4 bytes based on resolution
- [x] Sign-extend to int32_t
- [x] `mcp3421_read_raw()` function
- **Test**: Read non-zero value, verify changes with input signal

### Step 4: Millivolt Conversion
- [x] LSB size = (VREF × 2) / (full_scale_counts × gain)
- [x] `mcp3421_read_mv()` function
- **Test**: Apply 1.0V input → reading ≈ 1000mV

### Step 5: Two-IC Test
- [ ] Init IC1 (0x68) and IC2 (0x69) separately
- [ ] Read both channels in sequence
- **Test**: Both ICs read independently without interference

## Usage Example
```c
// Bus init (once)
mcp3421_bus_config_t bus = {
    .i2c_port  = I2C_NUM_0,
    .sda_pin   = 5,
    .scl_pin   = 6,
    .i2c_clk_hz = 400000,
};
mcp3421_bus_init(&bus);

// IC1 init
mcp3421_dev_t ic1;
mcp3421_dev_config_t ic1_cfg = {
    .addr = MCP3421_ADDR_GND, // 0x68
    .rate = MCP3421_RATE_15SPS,
    .gain = MCP3421_GAIN_1,
};
mcp3421_init(&ic1, &ic1_cfg, I2C_NUM_0);

// IC2 init
mcp3421_dev_t ic2;
mcp3421_dev_config_t ic2_cfg = {
    .addr = MCP3421_ADDR_VDD, // 0x69
    .rate = MCP3421_RATE_15SPS,
    .gain = MCP3421_GAIN_1,
};
mcp3421_init(&ic2, &ic2_cfg, I2C_NUM_0);

// Read
float mv1, mv2;
mcp3421_read_mv(&ic1, &mv1);
mcp3421_read_mv(&ic2, &mv2);
printf("IC1: %.2f mV, IC2: %.2f mV\n", mv1, mv2);
```

## Notes
- Use 15SPS (16-bit) for best noise performance in temperature sensing
- Gain=1 for inputs near 2V full scale; Gain=8 for small signals (<250mV)
