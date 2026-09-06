#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ── Step 1 (active) ───────────────────────────────────────────────────
#include "led_blink.h"

// ── Step 2+ (uncomment as each step is done) ──────────────────────────
// #include "glcd_cog.h"
// #include "mcp3421.h"
// #include "battery_adc.h"
// #include "input_switch.h"
// #include "user_menu.h"

// ── Hardware config ────────────────────────────────────────────────────
static led_blink_config_t s_led = {
    .pin         = 48,
    .active_high = true,
};

/*
static glcd_cog_config_t s_disp = {
    .spi_host = SPI2_HOST,
    .sck_pin  = 12,
    .mosi_pin = 11,
    .cs_pin   = 10,
    .dc_pin   = 9,
    .rst_pin  = 8,
    .clk_hz   = 8000000,
};

static mcp3421_dev_t s_ic1, s_ic2;

static battery_adc_config_t s_bat = {
    .adc_channel   = ADC1_CHANNEL_0,
    .adc_gpio      = 1,
    .r1_ohm        = 100000,
    .r2_ohm        = 47000,
    .vbat_full_mv  = 4200,
    .vbat_empty_mv = 3000,
};

static input_switch_config_t s_sw_cfg = {
    .pins = {
        {.pin=2, .id=SW_UP,     .active_low=true, .hold_ms=1000},
        {.pin=3, .id=SW_DOWN,   .active_low=true, .hold_ms=1000},
        {.pin=4, .id=SW_SELECT, .active_low=true, .hold_ms=1500},
        {.pin=7, .id=SW_BACK,   .active_low=true, .hold_ms=0},
    },
    .count       = 4,
    .debounce_ms = 20,
};

static void sw_event_cb(switch_id_t id, switch_event_t event) {
    user_menu_handle_event(id, event);
}

void action_measure(void) { }
void action_battery(void) { }

static const menu_t s_main_menu = {
    .title = "MAIN MENU",
    .items = {
        {"Measure",  action_measure},
        {"Battery",  action_battery},
        {"Settings", NULL},
        {"About",    NULL},
    },
    .count = 4,
};
*/

void app_main(void)
{
    // ── Step 1: LED blink test ──────────────────────────────────────
    led_blink_init(&s_led);
    printf("tempHumiBatEsp32s3 v1.0 starting\n");

    // ── Step 2: GLCD init + splash screen ──────────────────────────
    /*
    glcd_cog_init(&s_disp);
    glcd_cog_clear();
    glcd_cog_draw_string(10, 20, "CD Industrial");
    glcd_cog_draw_string(30, 32, "v1.0");
    glcd_cog_draw_rect(0, 0, 128, 64, true);
    glcd_cog_update();
    vTaskDelay(pdMS_TO_TICKS(2000));
    */

    // ── Step 3+4: Switches + Menu ───────────────────────────────────
    /*
    input_switch_init(&s_sw_cfg, sw_event_cb);
    user_menu_init(&s_main_menu);
    */

    // ── Step 5: MCP3421 ADC ─────────────────────────────────────────
    /*
    mcp3421_bus_config_t bus = {
        .i2c_port   = I2C_NUM_0,
        .sda_pin    = 5,
        .scl_pin    = 6,
        .i2c_clk_hz = 400000,
    };
    mcp3421_bus_init(&bus);
    mcp3421_dev_config_t ic1_cfg = {
        .addr = MCP3421_ADDR_GND,
        .rate = MCP3421_RATE_15SPS,
        .gain = MCP3421_GAIN_1,
    };
    mcp3421_dev_config_t ic2_cfg = {
        .addr = MCP3421_ADDR_VDD,
        .rate = MCP3421_RATE_15SPS,
        .gain = MCP3421_GAIN_1,
    };
    mcp3421_init(&s_ic1, &ic1_cfg, I2C_NUM_0);
    mcp3421_init(&s_ic2, &ic2_cfg, I2C_NUM_0);
    */

    // ── Step 6: Battery ADC ─────────────────────────────────────────
    /*
    battery_adc_init(&s_bat);
    */

    while (1) {
        // Step 1: blink 1Hz
        led_blink_run(500, 500);

        // Step 5+: read ADC and show on display
        /*
        float mv1, mv2;
        mcp3421_read_mv(&s_ic1, &mv1);
        mcp3421_read_mv(&s_ic2, &mv2);

        uint32_t vbat = battery_adc_read_mv();
        uint8_t  pct  = battery_adc_read_percent();

        char buf[22];
        glcd_cog_clear();
        snprintf(buf, sizeof(buf), "IC1:%.1fmV", mv1);
        glcd_cog_draw_string(0, 0, buf);
        snprintf(buf, sizeof(buf), "IC2:%.1fmV", mv2);
        glcd_cog_draw_string(0, 10, buf);
        snprintf(buf, sizeof(buf), "Bat:%u%% %umV", pct, vbat);
        glcd_cog_draw_string(0, 20, buf);
        glcd_cog_update();
        */
    }
}
