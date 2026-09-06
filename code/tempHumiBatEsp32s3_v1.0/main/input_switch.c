#include "input_switch.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define SCAN_INTERVAL_MS  10

typedef struct {
    bool     raw;          // last read raw state
    bool     debounced;    // debounced state
    uint32_t stable_ms;    // how long current raw has been stable
    uint32_t hold_counter; // ms held
    bool     hold_fired;
} switch_state_t;

static input_switch_config_t s_cfg;
static switch_event_cb_t     s_cb;
static switch_state_t        s_state[INPUT_SWITCH_MAX];

static bool read_pin(uint8_t idx)
{
    int level = gpio_get_level(s_cfg.pins[idx].pin);
    return s_cfg.pins[idx].active_low ? (level == 0) : (level == 1);
}

static void switch_scan_task(void *arg)
{
    (void)arg;
    while (1) {
        for (uint8_t i = 0; i < s_cfg.count; i++) {
            bool pressed = read_pin(i);
            switch_state_t *st = &s_state[i];

            if (pressed == st->raw) {
                st->stable_ms += SCAN_INTERVAL_MS;
            } else {
                st->raw       = pressed;
                st->stable_ms = 0;
            }

            if (st->stable_ms >= s_cfg.debounce_ms) {
                bool was = st->debounced;
                st->debounced = pressed;

                if (!was && pressed) {
                    st->hold_counter = 0;
                    st->hold_fired   = false;
                    if (s_cb) s_cb(s_cfg.pins[i].id, SW_EVENT_PRESS);
                } else if (was && !pressed) {
                    st->hold_counter = 0;
                    if (s_cb) s_cb(s_cfg.pins[i].id, SW_EVENT_RELEASE);
                }

                // Hold event
                if (pressed && s_cfg.pins[i].hold_ms > 0 && !st->hold_fired) {
                    st->hold_counter += SCAN_INTERVAL_MS;
                    if (st->hold_counter >= s_cfg.pins[i].hold_ms) {
                        st->hold_fired = true;
                        if (s_cb) s_cb(s_cfg.pins[i].id, SW_EVENT_HOLD);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}

void input_switch_init(const input_switch_config_t *cfg, switch_event_cb_t cb)
{
    s_cfg = *cfg;
    s_cb  = cb;
    memset(s_state, 0, sizeof(s_state));

    for (uint8_t i = 0; i < cfg->count; i++) {
        gpio_config_t io = {
            .pin_bit_mask = (1ULL << cfg->pins[i].pin),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = cfg->pins[i].active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = cfg->pins[i].active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io);
    }

    xTaskCreate(switch_scan_task, "sw_scan", 2048, NULL, 5, NULL);
}

bool input_switch_is_pressed(switch_id_t id)
{
    for (uint8_t i = 0; i < s_cfg.count; i++) {
        if (s_cfg.pins[i].id == id)
            return s_state[i].debounced;
    }
    return false;
}
