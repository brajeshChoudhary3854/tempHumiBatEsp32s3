#include "led_blink.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static led_blink_config_t s_cfg;

void led_blink_init(const led_blink_config_t *cfg)
{
    s_cfg = *cfg;
    gpio_config_t io = {
        .pin_bit_mask  = (1ULL << cfg->pin),
        .mode          = GPIO_MODE_OUTPUT,
        .pull_up_en    = GPIO_PULLUP_DISABLE,
        .pull_down_en  = GPIO_PULLDOWN_DISABLE,
        .intr_type     = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(cfg->pin, cfg->active_high ? 0 : 1); // LED off at start
}

void led_blink_set(bool on)
{
    int level = on ? (s_cfg.active_high ? 1 : 0) : (s_cfg.active_high ? 0 : 1);
    gpio_set_level(s_cfg.pin, level);
}

void led_blink_toggle(void)
{
    static bool state = false;
    state = !state;
    led_blink_set(state);
}

void led_blink_run(uint32_t on_ms, uint32_t off_ms)
{
    led_blink_set(true);
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    led_blink_set(false);
    vTaskDelay(pdMS_TO_TICKS(off_ms));
}
