#pragma once
#include <stdint.h>
#include <stdbool.h>

#define INPUT_SWITCH_MAX  8

typedef enum {
    SW_UP = 0,
    SW_DOWN,
    SW_SELECT,
    SW_BACK,
} switch_id_t;

typedef enum {
    SW_EVENT_PRESS        = 1,  // button pressed (debounced leading edge)
    SW_EVENT_RELEASE      = 2,  // button released (debounced trailing edge)
    SW_EVENT_CLICK        = 3,  // single click: press+release, no hold, no 2nd click follows
    SW_EVENT_DOUBLE_CLICK = 4,  // two clicks within double_click_ms window
    SW_EVENT_HOLD         = 5,  // press held >= hold_ms
} switch_event_t;

typedef struct {
    int          pin;
    switch_id_t  id;
    bool         active_low;        // true = pressed when GPIO LOW (pull-up wiring)
    uint32_t     hold_ms;           // ms before HOLD fires (0 = hold disabled)
    uint32_t     double_click_ms;   // window for 2nd click (0 = double-click disabled)
} switch_pin_config_t;

typedef struct {
    switch_pin_config_t pins[INPUT_SWITCH_MAX];
    uint8_t             count;
    uint32_t            debounce_ms;  // typically 20 ms
} input_switch_config_t;

typedef void (*switch_event_cb_t)(switch_id_t id, switch_event_t event);

void input_switch_init(const input_switch_config_t *cfg, switch_event_cb_t cb);
bool input_switch_is_pressed(switch_id_t id);
