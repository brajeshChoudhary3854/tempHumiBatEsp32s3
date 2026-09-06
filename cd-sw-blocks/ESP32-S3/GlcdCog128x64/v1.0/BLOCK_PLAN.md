# BLOCK PLAN: ESP32-S3 / GlcdCog128x64 / v1.0

Generated: 2026-09-06
Platform:  ESP32-S3
Interface: SPI (4-wire)
Reference: None (new block)
Standard:  Follow cd-sw-blocks/CLAUDE.md

## Block Description
COG (Chip On Glass) 128×64 monochrome graphic LCD driver.
Controller: ST7565R. Uses framebuffer — draw in RAM, flush to display with update().
Supports pixel ops, lines, rectangles, 5×7 ASCII text.

## Hardware
- 128×64 COG LCD module (ST7565R controller)
- 3.3V supply
- SPI connection (SCK, MOSI, CS, DC, RST)

## Pin Reference
| Signal | ESP32-S3 Pin | Notes                     |
|--------|--------------|---------------------------|
| SCK    | GPIO12       | SPI clock                 |
| MOSI   | GPIO11       | SPI data (SI)             |
| CS     | GPIO10       | Chip select, active low   |
| DC     | GPIO9        | Data=HIGH, Command=LOW    |
| RST    | GPIO8        | Hardware reset, active low |

## Development Steps

### Step 1: SPI Init + Reset + Basic Commands
- [x] SPI bus + device init
- [x] DC and RST GPIO config
- [x] `glcd_cog_init()` — ST7565 init sequence
- [x] send_cmd() / send_data() helpers
- **Test**: No crash on init, display turns on (may show garbage)

### Step 2: Framebuffer + Update
- [x] 1024-byte framebuffer (8 pages × 128 cols)
- [x] `glcd_cog_update()` — flush all pages to display
- [x] `glcd_cog_clear()` — zero framebuffer
- **Test**: Clear then update → blank screen

### Step 3: Pixel + Primitives
- [x] `glcd_cog_set_pixel(x, y, on)`
- [x] `glcd_cog_draw_hline()`, `draw_vline()`
- [x] `glcd_cog_draw_rect()`, `fill_rect()`
- [x] `glcd_cog_draw_line()` — Bresenham
- **Test**: Draw rect at corner, diagonal line

### Step 4: Text
- [x] 5×7 ASCII font (0x20–0x7E) in glcd_cog_font.c
- [x] `glcd_cog_draw_char(x, y, c)`
- [x] `glcd_cog_draw_string(x, y, str)`
- **Test**: Print "Hello World" on first line

### Step 5: Contrast + Invert
- [x] `glcd_cog_set_contrast(0–63)`
- [x] `glcd_cog_invert(bool on)`
- **Test**: Adjust contrast, invert display

## Usage Example
```c
glcd_cog_config_t disp = {
    .spi_host = SPI2_HOST,
    .sck_pin  = 12,
    .mosi_pin = 11,
    .cs_pin   = 10,
    .dc_pin   = 9,
    .rst_pin  = 8,
    .clk_hz   = 8000000,
};
glcd_cog_init(&disp);
glcd_cog_clear();
glcd_cog_draw_string(0, 0, "Hello World");
glcd_cog_draw_rect(0, 10, 128, 54, true);
glcd_cog_update();
```

## Notes
- All draw functions operate on RAM framebuffer only
- Call glcd_cog_update() after all draws to push to LCD
- For partial updates (performance), send individual pages only
