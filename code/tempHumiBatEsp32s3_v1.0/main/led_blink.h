#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int pin;       // GPIO pin number
    bool active_high; // true = HIGH is ON, false = LOW is ON
} led_blink_config_t;

void led_blink_init(const led_blink_config_t *cfg);
void led_blink_set(bool on);
void led_blink_toggle(void);
void led_blink_run(uint32_t on_ms, uint32_t off_ms); // one blink cycle
