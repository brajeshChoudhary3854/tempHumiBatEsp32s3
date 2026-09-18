# PROJECT PLAN: tempHumiBatEsp32s3_v1

Generated: 2026-09-06
MCU:       ESP32-S3
Version:   v1.0

## Project Description
Temperature + Humidity datalogger with 128×64 GLCD display, battery level monitoring,
MCP3421 external ADC (×2), and user menu navigation via tactile switches.

## Hardware Summary
| Component      | Interface | Pins                        |
|----------------|-----------|-----------------------------|
| LED (status)   | GPIO48    | active_high                 |
| GLCD ST7565    | SPI2      | SCK=13, MOSI=14, CS=10, DC=12, RST=11, BL=40 |
| MCP3421 IC1    | I2C0      | SDA=5, SCL=6, addr=0x68     |
| MCP3421 IC2    | I2C0      | SDA=5, SCL=6, addr=0x69     |
| Battery ADC    | ADC1_CH0  | GPIO1 (via 100k+47k divider)|
| SW_UP          | GPIO2     | active_low, hold=1000ms     |
| SW_DOWN        | GPIO3     | active_low, hold=1000ms     |
| SW_SELECT      | GPIO4     | active_low, hold=1500ms     |
| SW_BACK        | GPIO7     | active_low                  |

## Development Steps

### Step 1: LED Blink Test (bring-up) ✓ DONE
- [x] Include LedBlink block
- [x] Init LED on GPIO48
- [x] Blink 500ms on / 500ms off in main loop
- **Test**: LED blinks → power, GPIO, FreeRTOS all working ✓
- File: code/main/main.c

### Step 2: GLCD Test
- [ ] Include GlcdCog128x64 block
- [ ] Init GLCD (SPI2, pins as above)
- [ ] Display splash screen: "CD Industrial\nv1.0"
- [ ] Draw border rect
- **Test**: Text visible on display
- File: code/main/main.c

### Step 3: Input Switch Test
- [ ] Include InputSwitch block
- [ ] Init 4 switches (UP/DOWN/SELECT/BACK)
- [ ] On press: show switch name on GLCD
- **Test**: Each button press updates display text

### Step 4: User Menu
- [ ] Include UserMenu block
- [ ] Define main_menu with items: Measure, Battery, Settings, About
- [ ] Connect switch callback to user_menu_handle_event()
- [ ] Display menu on GLCD
- **Test**: Navigate UP/DOWN, SELECT fires item action

### Step 5: MCP3421 (ADC IC1 + IC2)
- [ ] Include Mcp3421 block
- [ ] Init I2C bus (I2C0, 400kHz)
- [ ] Init IC1 (0x68, 15SPS, gain=1) and IC2 (0x69, 15SPS, gain=1)
- [ ] Read both channels in loop, display on GLCD
- **Test**: Values change with input signal variation

### Step 6: Battery ADC
- [ ] Include BatteryAdc block
- [ ] Init with R1=100k, R2=47k, GPIO1, Li-Ion 3.0–4.2V
- [ ] Display battery % in status bar on GLCD (top right)
- **Test**: Battery % reads 0–100%, stable reading

### Step 7: Screens + Menu Integration
- [ ] Measure screen: show IC1 + IC2 ADC readings (mV)
- [ ] Battery screen: show voltage + % + simple bar graph
- [ ] Status bar (always visible): time or batt % in corner
- [ ] Menu actions navigate between screens
- **Test**: Full menu navigation, all screens display correctly

### Step 8: LED Status Logic
- [ ] Blink slow (1Hz) = idle/menu
- [ ] Blink fast (5Hz) = measuring
- [ ] Solid ON = error
- **Test**: LED state matches application state

## Notes
- Complete each step and test before moving to next
- Blocks are in code/blocks/{BlockName}/
- Main app logic in code/main/main.c
