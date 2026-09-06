# BLOCK PLAN: ESP32-S3 / InputSwitch / v1.0

Generated: 2026-09-06
Platform:  ESP32-S3
Interface: GPIO Input
Reference: None (new block)
Standard:  Follow cd-sw-blocks/CLAUDE.md

## Block Description
Multi-button input driver with software debounce and hold detection.
Callback-based: fires PRESS, RELEASE, HOLD events per switch.
Background FreeRTOS task scans all pins every 10ms.
Supports up to 8 switches (SW_UP, SW_DOWN, SW_SELECT, SW_BACK, ...).

## Hardware
- 1–8 tactile push buttons (SPST NO)
- Active-low with internal pull-up (no external resistors)

## Pin Reference
| Switch    | ESP32-S3 Pin | Notes              |
|-----------|--------------|--------------------|
| SW_UP     | GPIO2        | Active low (pull-up) |
| SW_DOWN   | GPIO3        | Active low (pull-up) |
| SW_SELECT | GPIO4        | Active low (pull-up) |
| SW_BACK   | GPIO7        | Active low (pull-up) |

## Development Steps

### Step 1: GPIO Init
- [x] gpio_config() for all switch pins: input mode, pull-up, no interrupt
- **Test**: gpio_get_level() returns 1 when unpressed, 0 when pressed

### Step 2: Scan Task + Debounce
- [x] FreeRTOS task, 10ms period
- [x] Per-pin stable time counter
- [x] Debounce: state changes only after stable_ms >= debounce_ms (20ms)
- **Test**: Short glitches ignored, clean press registered

### Step 3: Event Callbacks
- [x] PRESS event on rising edge of debounced state
- [x] RELEASE event on falling edge
- [x] HOLD event after hold_ms of continuous press
- [x] `switch_event_cb_t` callback registered at init
- **Test**: Press → PRESS fires once; hold 1s → HOLD fires; release → RELEASE fires

### Step 4: Poll API
- [x] `input_switch_is_pressed(id)` — instant non-blocking state query
- **Test**: Returns true while button held

## Usage Example
```c
void my_switch_cb(switch_id_t id, switch_event_t event) {
    if (id == SW_UP && event == SW_EVENT_PRESS) {
        // navigate up
    }
    if (id == SW_SELECT && event == SW_EVENT_HOLD) {
        // long-press action
    }
}

input_switch_config_t sw_cfg = {
    .pins = {
        {.pin=2, .id=SW_UP,     .active_low=true, .hold_ms=1000},
        {.pin=3, .id=SW_DOWN,   .active_low=true, .hold_ms=1000},
        {.pin=4, .id=SW_SELECT, .active_low=true, .hold_ms=1500},
    },
    .count       = 3,
    .debounce_ms = 20,
};
input_switch_init(&sw_cfg, my_switch_cb);
```
