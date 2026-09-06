# tempHumiBatEsp32s3 — Claude Read This First

## Project
ESP32-S3 based temperature/humidity datalogger with GLCD display, battery monitoring,
external ADC, and user menu navigation.

## Folder Structure
```
tempHumiBatEsp32s3/
├── code/tempHumiBatEsp32s3_v1.0/   ← ESP-IDF project (idf.py build here)
│   ├── CMakeLists.txt
│   └── main/
│       ├── CMakeLists.txt
│       ├── main.c                  ← main application
│       ├── led_blink.c / .h        ← LedBlink block
│       ├── glcd_cog.c / .h / font  ← GlcdCog128x64 block (ST7565R)
│       ├── mcp3421.c / .h          ← Mcp3421 block (I2C ADC)
│       ├── battery_adc.c / .h      ← BatteryAdc block (internal ADC1)
│       ├── input_switch.c / .h     ← InputSwitch block (debounce, events)
│       └── user_menu.c / .h        ← UserMenu block (GLCD menu)
├── cd-sw-blocks/ESP32-S3/          ← local reference copies of blocks
│   └── {BlockName}/v1.0/           ← full block with doc, BLOCK_PLAN, wiring
└── doc/wiring_guide.md             ← complete pin table
```

## Block Library (upstream)
Library repo: `f:\oneDriveBkup\ProjectGit\cdProduct\cd-sw-blocks`
GitHub: `https://github.com/brajeshChoudhary3854/cd-sw-blocks`

Project repo: `f:\oneDriveBkup\ProjectGit\cdProduct\cdProduct-blocks`
GitHub: `https://github.com/brajeshChoudhary3854/cdProduct-blocks`

## Workflow Rules

### Working on firmware (main.c, driver files):
1. Read PROJECT_PLAN.md → work on current step only
2. One step at a time → test → commit → next step
3. Build: `cd code/tempHumiBatEsp32s3_v1.0 && idf.py build`
4. Flash: `idf.py -p COM## flash monitor`

### When a block file is improved or fixed:
1. Copy the updated .c / .h back to cd-sw-blocks:
   - `f:\oneDriveBkup\ProjectGit\cdProduct\cd-sw-blocks\ESP32-S3\{BlockName}\v1.0\sw\src\`
   - `f:\oneDriveBkup\ProjectGit\cdProduct\cd-sw-blocks\ESP32-S3\{BlockName}\v1.0\sw\include\`
2. Commit in cd-sw-blocks repo
3. Commit in this project repo (same change)

### Commit message style:
```
feat: step 2 - GLCD init and splash screen working
fix: mcp3421 sign-extend bug for 18-bit reading
```

## Pin Table
| GPIO | Function      | Block           |
|------|---------------|-----------------|
| 1    | BAT_ADC       | BatteryAdc      |
| 2    | SW_UP         | InputSwitch     |
| 3    | SW_DOWN       | InputSwitch     |
| 4    | SW_SELECT     | InputSwitch     |
| 5    | I2C_SDA       | Mcp3421         |
| 6    | I2C_SCL       | Mcp3421         |
| 7    | SW_BACK       | InputSwitch     |
| 8    | GLCD_RST      | GlcdCog128x64   |
| 9    | GLCD_DC       | GlcdCog128x64   |
| 10   | GLCD_CS       | GlcdCog128x64   |
| 11   | SPI_MOSI      | GlcdCog128x64   |
| 12   | SPI_SCK       | GlcdCog128x64   |
| 48   | LED           | LedBlink        |

## Development Steps (see PROJECT_PLAN.md for details)
1. LED Blink test ← START HERE
2. GLCD init + splash
3. Input switches test
4. User menu navigation
5. MCP3421 ADC (IC1 + IC2)
6. Battery ADC
7. Screens + menu integration
8. LED status logic
