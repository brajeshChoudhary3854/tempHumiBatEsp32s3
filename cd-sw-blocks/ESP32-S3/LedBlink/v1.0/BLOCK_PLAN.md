# BLOCK PLAN: ESP32-S3 / LedBlink / v1.0

Generated: 2026-09-06
Platform:  ESP32-S3
Interface: GPIO (Output)
Reference: None (new block)
Standard:  Follow cd-sw-blocks/CLAUDE.md

## Block Description
Simple LED blink driver for ESP32-S3. Used as first bring-up test to verify
GPIO output, power supply, and FreeRTOS task scheduling are working correctly.

## Hardware
- 1× LED (any color)
- 1× 330Ω resistor
- ESP32-S3 (any variant)

## Pin Reference
| Signal | ESP32-S3 Pin | Notes                  |
|--------|--------------|------------------------|
| LED    | GPIO48       | Default — change in config |
| GND    | GND          | LED cathode side       |

## Development Steps

### Step 1: GPIO Init + Basic Blink
- [x] `led_blink_init()` — gpio_config, set output mode
- [x] `led_blink_set(bool on)` — active_high / active_low support
- [x] `led_blink_toggle()` — toggle state
- [x] `led_blink_run(on_ms, off_ms)` — one blink cycle with delay
- **Test**: Flash, LED should blink at configured rate

### Step 2: FreeRTOS Task (if needed)
- [ ] `led_blink_task_start(on_ms, off_ms)` — create blink task
- [ ] `led_blink_task_stop()` — delete blink task
- **Test**: LED blinks in background while main task runs

## Usage Example
```c
led_blink_config_t led = {
    .pin = 48,
    .active_high = true,
};
led_blink_init(&led);

// In a loop or task:
while (1) {
    led_blink_run(500, 500); // 500ms ON, 500ms OFF
}
```

## Notes
- Step 2 only if background blinking is required by project
