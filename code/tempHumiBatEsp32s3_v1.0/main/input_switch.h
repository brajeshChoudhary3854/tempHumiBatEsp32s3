#pragma once
#include <stdint.h>
#include <stdbool.h>

#define INPUT_SWITCH_MAX  8  // max number of switches

typedef enum {
    SW_UP = 0,
    SW_DOWN,
    SW_SELECT,
    SW_BACK,
    // Add more as needed, up to INPUT_SWITCH_MAX
} switch_id_t;

typedef enum {
    SW_EVENT_PRESS   = 1,
    SW_EVENT_RELEASE = 2,
    SW_EVENT_HOLD    = 3,  // fired after hold_ms continuous press
} switch_event_t;

typedef struct {
    int            pin;
    switch_id_t    id;
    bool           active_low;  // true = pressed when GPIO LOW (pull-up)
    uint32_t       hold_ms;     // ms to fire HOLD event (0 = disabled)
} switch_pin_config_t;

typedef struct {
    switch_pin_config_t pins[INPUT_SWITCH_MAX];
    uint8_t             count;
    uint32_t            debounce_ms; // typically 20ms
} input_switch_config_t;

typedef void (*switch_event_cb_t)(switch_id_t id, switch_event_t event);

void input_switch_init(const input_switch_config_t *cfg, switch_event_cb_t cb);
bool input_switch_is_pressed(switch_id_t id); // poll current state
