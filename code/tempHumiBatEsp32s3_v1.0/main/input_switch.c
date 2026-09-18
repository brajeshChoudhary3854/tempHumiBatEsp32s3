#include "input_switch.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define SCAN_MS  10u   // scan period — must match vTaskDelay below

typedef enum {
    FSM_IDLE = 0,
    FSM_PRESSED,       // first press down, hold timer running
    FSM_WAIT,          // first release, watching for 2nd click
    FSM_PRESSED_2ND,   // 2nd press within double_click_ms window
} fsm_t;

typedef struct {
    bool     raw;           // last raw GPIO reading
    uint32_t stable_ms;     // how long raw has been stable
    bool     debounced;     // last accepted (stable) state
    fsm_t    fsm;
    uint32_t hold_timer;    // time spent in FSM_PRESSED (ms)
    bool     hold_fired;    // HOLD event already sent this press
    uint32_t dbl_timer;     // time spent in FSM_WAIT (ms)
} sw_rt_t;

static input_switch_config_t s_cfg;
static switch_event_cb_t     s_cb;
static sw_rt_t               s_rt[INPUT_SWITCH_MAX];

static bool pin_is_pressed(uint8_t i)
{
    int lv = gpio_get_level(s_cfg.pins[i].pin);
    return s_cfg.pins[i].active_low ? (lv == 0) : (lv != 0);
}

static void emit(uint8_t i, switch_event_t ev)
{
    if (s_cb) s_cb(s_cfg.pins[i].id, ev);
}

static void scan_task(void *arg)
{
    (void)arg;
    while (1) {
        for (uint8_t i = 0; i < s_cfg.count; i++) {
            bool raw = pin_is_pressed(i);
            sw_rt_t *rt = &s_rt[i];
            const switch_pin_config_t *pc = &s_cfg.pins[i];

            // --- debounce ------------------------------------------------
            if (raw != rt->raw) {
                rt->raw       = raw;
                rt->stable_ms = 0;
            } else {
                rt->stable_ms += SCAN_MS;
            }
            // cur changes only after debounce_ms of stable reading
            bool cur       = (rt->stable_ms >= s_cfg.debounce_ms) ? raw : rt->debounced;
            bool j_press   =  cur && !rt->debounced;
            bool j_release = !cur &&  rt->debounced;
            rt->debounced  = cur;

            // --- state machine -------------------------------------------
            switch (rt->fsm) {

            case FSM_IDLE:
                if (j_press) {
                    rt->fsm        = FSM_PRESSED;
                    rt->hold_timer = 0;
                    rt->hold_fired = false;
                    emit(i, SW_EVENT_PRESS);
                }
                break;

            case FSM_PRESSED:
                // advance hold timer while button stays down
                if (cur && !rt->hold_fired) {
                    rt->hold_timer += SCAN_MS;
                    if (pc->hold_ms > 0 && rt->hold_timer >= pc->hold_ms) {
                        rt->hold_fired = true;
                        emit(i, SW_EVENT_HOLD);
                    }
                }
                if (j_release) {
                    emit(i, SW_EVENT_RELEASE);
                    if (rt->hold_fired) {
                        // HOLD already fired — suppress click on this release
                        rt->fsm = FSM_IDLE;
                    } else if (pc->double_click_ms == 0) {
                        // double-click detection disabled — fire CLICK immediately
                        emit(i, SW_EVENT_CLICK);
                        rt->fsm = FSM_IDLE;
                    } else {
                        // wait to see if a 2nd press follows
                        rt->fsm       = FSM_WAIT;
                        rt->dbl_timer = 0;
                    }
                }
                break;

            case FSM_WAIT:
                rt->dbl_timer += SCAN_MS;
                if (j_press) {
                    // 2nd press within window → double-click path
                    rt->fsm = FSM_PRESSED_2ND;
                    emit(i, SW_EVENT_PRESS);
                } else if (rt->dbl_timer >= pc->double_click_ms) {
                    // window expired → single click confirmed
                    emit(i, SW_EVENT_CLICK);
                    rt->fsm = FSM_IDLE;
                }
                break;

            case FSM_PRESSED_2ND:
                if (j_release) {
                    emit(i, SW_EVENT_RELEASE);
                    emit(i, SW_EVENT_DOUBLE_CLICK);
                    rt->fsm = FSM_IDLE;
                }
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SCAN_MS));
    }
}

void input_switch_init(const input_switch_config_t *cfg, switch_event_cb_t cb)
{
    s_cfg = *cfg;
    s_cb  = cb;
    memset(s_rt, 0, sizeof(s_rt));

    for (uint8_t i = 0; i < cfg->count; i++) {
        gpio_config_t io = {
            .pin_bit_mask = (1ULL << cfg->pins[i].pin),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = cfg->pins[i].active_low
                                ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = cfg->pins[i].active_low
                                ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io);
    }

    xTaskCreate(scan_task, "sw_scan", 2048, NULL, 5, NULL);
}

bool input_switch_is_pressed(switch_id_t id)
{
    for (uint8_t i = 0; i < s_cfg.count; i++) {
        if (s_cfg.pins[i].id == id)
            return s_rt[i].debounced;
    }
    return false;
}
