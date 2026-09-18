#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ── Step 1 (active) ───────────────────────────────────────────────────
#include "led_blink.h"

// ── Step 2+ (uncomment as each step is done) ──────────────────────────
#include "glcd_cog.h"
#include "home_screen.h"
// #include "hc2a.h"          // Step 5: HC2A-S3 temp+humi sensor
// #include "battery_adc.h"
#include "input_switch.h"
// #include "user_menu.h"

// ── Hardware config ────────────────────────────────────────────────────
static led_blink_config_t s_led = {
    .pin         = 48,
    .active_high = true,
};

static glcd_cog_config_t s_disp = {
    .spi_host   = SPI2_HOST,
    .sck_pin    = 13,
    .mosi_pin   = 14,
    .cs_pin     = 10,
    .dc_pin     = 12,
    .rst_pin    = 11,
    .bl_pin     = 40,
    .clk_hz     = 8000000,
    .col_offset = 4,    // ST7565R: 132 drivers, 128 visible — shift by 4
};

// ── Serial display control ─────────────────────────────────────────────
static uint8_t s_contrast   = 1;    // matches glcd_cog.c init value
static uint8_t s_backlight  = 100;  // 0–100 %

static void serial_cmd_task(void *arg)
{
    (void)arg;
    char buf[32];
    int  pos = 0;
    printf("\n--- display control ---\n");
    printf("  CNT<0-63>    e.g. CNT8   set contrast\n");
    printf("  BKL<0-100>   e.g. BKL50  set backlight %%\n");
    printf("  DISH                     0deg   landscape normal\n");
    printf("  DISV                     180deg landscape flipped\n");
    printf("  DIS90                    90deg  CW portrait\n");
    printf("  DIS270                   90deg  CCW portrait\n");
    printf("  +  /  -      contrast up / down by 1\n");
    printf("CNT=%d  BKL=%d\n\n", s_contrast, s_backlight);

    while (1) {
        int c = getchar();
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (c == '+') {
            if (s_contrast < 63) s_contrast++;
            glcd_cog_set_contrast(s_contrast);
            printf("CNT=%d\n", s_contrast);
            continue;
        }
        if (c == '-') {
            if (s_contrast > 0) s_contrast--;
            glcd_cog_set_contrast(s_contrast);
            printf("CNT=%d\n", s_contrast);
            continue;
        }

        if (c == '\r' || c == '\n') {
            buf[pos] = '\0';
            if (pos > 0) {
                int val;
                if (sscanf(buf, "CNT%d", &val) == 1) {
                    if (val < 0) val = 0;
                    if (val > 63) val = 63;
                    s_contrast = (uint8_t)val;
                    glcd_cog_set_contrast(s_contrast);
                    printf("CNT=%d\n", s_contrast);
                } else if (sscanf(buf, "BKL%d", &val) == 1) {
                    if (val < 0) val = 0;
                    if (val > 100) val = 100;
                    s_backlight = (uint8_t)val;
                    glcd_cog_set_backlight(s_backlight);
                    printf("BKL=%d%%\n", s_backlight);
                } else if (strcmp(buf, "DISH") == 0) {
                    glcd_cog_set_orientation(GLCD_ORIENT_H);
                    glcd_cog_clear();
                    glcd_cog_draw_rect(0, 0, glcd_cog_log_width(), glcd_cog_log_height(), true);
                    glcd_cog_draw_string(4, 10, "DISH 0deg");
                    glcd_cog_draw_string(4, 24, "128x64");
                    glcd_cog_update();
                    printf("orientation: DISH 0deg\n");
                } else if (strcmp(buf, "DISV") == 0) {
                    glcd_cog_set_orientation(GLCD_ORIENT_V);
                    glcd_cog_clear();
                    glcd_cog_draw_rect(0, 0, glcd_cog_log_width(), glcd_cog_log_height(), true);
                    glcd_cog_draw_string(4, 10, "DISV 180deg");
                    glcd_cog_draw_string(4, 24, "128x64");
                    glcd_cog_update();
                    printf("orientation: DISV 180deg\n");
                } else if (strcmp(buf, "DIS90") == 0) {
                    glcd_cog_set_orientation(GLCD_ORIENT_CW);
                    glcd_cog_clear();
                    glcd_cog_draw_rect(0, 0, glcd_cog_log_width(), glcd_cog_log_height(), true);
                    glcd_cog_draw_string(2, 10, "DIS90");
                    glcd_cog_draw_string(2, 24, "90 CW");
                    glcd_cog_draw_string(2, 38, "64x128");
                    glcd_cog_update();
                    printf("orientation: DIS90 CW  canvas=64x128\n");
                } else if (strcmp(buf, "DIS270") == 0) {
                    glcd_cog_set_orientation(GLCD_ORIENT_CCW);
                    glcd_cog_clear();
                    glcd_cog_draw_rect(0, 0, glcd_cog_log_width(), glcd_cog_log_height(), true);
                    glcd_cog_draw_string(2, 10, "DIS270");
                    glcd_cog_draw_string(2, 24, "90 CCW");
                    glcd_cog_draw_string(2, 38, "64x128");
                    glcd_cog_update();
                    printf("orientation: DIS270 CCW canvas=64x128\n");
                } else {
                    printf("? CNT<0-63>  BKL<0-100>  DISH DISV DIS90 DIS270  + -\n");
                }
                pos = 0;
            }
        } else if (pos < (int)sizeof(buf) - 1) {
            buf[pos++] = (char)c;
        }
    }
}

// ── Step 3: switch config + callback ──────────────────────────────────
static input_switch_config_t s_sw_cfg = {
    .pins = {
        {.pin=2, .id=SW_UP,     .active_low=true, .hold_ms=1000, .double_click_ms=300},
        {.pin=3, .id=SW_DOWN,   .active_low=true, .hold_ms=1000, .double_click_ms=300},
        {.pin=4, .id=SW_SELECT, .active_low=true, .hold_ms=1500, .double_click_ms=300},
        {.pin=7, .id=SW_BACK,   .active_low=true, .hold_ms=0,    .double_click_ms=0},
    },
    .count       = 4,
    .debounce_ms = 20,
};

static const char *sw_name(switch_id_t id)
{
    switch (id) {
        case SW_UP:     return "UP";
        case SW_DOWN:   return "DOWN";
        case SW_SELECT: return "SEL";
        case SW_BACK:   return "BACK";
        default:        return "?";
    }
}

static void sw_event_cb(switch_id_t id, switch_event_t event)
{
    const char *ev_str = NULL;
    switch (event) {
        case SW_EVENT_CLICK:        ev_str = "CLICK";        break;
        case SW_EVENT_DOUBLE_CLICK: ev_str = "DOUBLE CLICK"; break;
        case SW_EVENT_HOLD:         ev_str = "HOLD";         break;
        default: return;
    }

    printf("SW %-4s  %s\n", sw_name(id), ev_str);

    // Briefly show event on GLCD then restore home screen
    glcd_cog_clear();
    glcd_cog_draw_rect(0, 0, glcd_cog_log_width(), glcd_cog_log_height(), true);
    glcd_cog_draw_string(4, 10, "Switch:");
    glcd_cog_draw_string(4, 24, sw_name(id));
    glcd_cog_draw_string(4, 38, ev_str);
    glcd_cog_update();
    vTaskDelay(pdMS_TO_TICKS(1000));
    home_screen_draw(25.3f, 68.0f);

    // Step 4: hand off to menu
    // user_menu_handle_event(id, event);
}

/*
// ── Step 5-8 placeholders (uncomment as steps are enabled) ────────────

// HC2A-S3: one handle wraps both MCP3421 ADCs + both AdcSensor tasks
static int32_t     s_temp_buf[HC2A_BUF_SIZE_DEFAULT];
static int32_t     s_humi_buf[HC2A_BUF_SIZE_DEFAULT];
static hc2a_t      s_hc2a;

static const hc2a_config_t s_hc2a_cfg = {
    // IC1 — HC2A pin 6 (temp output) → I2C_NUM_0  SDA=GPIO8  SCL=GPIO9
    .temp_bus = { .i2c_port=I2C_NUM_0, .sda_pin=8, .scl_pin=9,
                  .clk_hz=MCP3421_CLK_DEFAULT },
    // IC2 — HC2A pin 5 (humi output) → I2C_NUM_1  SDA=GPIO4  SCL=GPIO5
    .humi_bus = { .i2c_port=I2C_NUM_1, .sda_pin=4, .scl_pin=5,
                  .clk_hz=MCP3421_CLK_DEFAULT },
    .temp_buf        = s_temp_buf,
    .humi_buf        = s_humi_buf,
    .buf_size        = HC2A_BUF_SIZE_DEFAULT,
    .scan_ms         = HC2A_SCAN_MS_DEFAULT,
    .error_threshold = ADC_SENSOR_ERROR_THRESHOLD_DEFAULT,
};

static battery_adc_config_t s_bat = {
    .adc_channel   = ADC1_CHANNEL_0,
    .adc_gpio      = 1,
    .sense_en_gpio = 39,
    .r1_ohm        = 100000,
    .r2_ohm        = 47000,
    .vbat_full_mv  = 4200,
    .vbat_empty_mv = 3000,
};

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

    // ── Step 2: GLCD init — default DIS270 portrait home screen ────
    glcd_cog_init(&s_disp);
    glcd_cog_set_orientation(GLCD_ORIENT_CCW);   // DIS270: 64×128 portrait
    home_screen_draw(25.3f, 68.0f);              // demo values until ADC ready

    // ── Serial contrast tuning task ────────────────────────────────
    xTaskCreate(serial_cmd_task, "serial_cmd", 4096, NULL, 3, NULL);

    // ── Step 3: Input switches test ────────────────────────────────
    input_switch_init(&s_sw_cfg, sw_event_cb);

    // ── Step 4: User menu (uncomment when ready) ────────────────────
    /*
    user_menu_init(&s_main_menu);
    */

    // ── Step 5: HC2A-S3 sensor (temp + humi via two MCP3421 ADCs) ──
    /*
    hc2a_init(&s_hc2a, &s_hc2a_cfg);
    */

    // ── Step 6: Battery ADC ─────────────────────────────────────────
    /*
    battery_adc_init(&s_bat);
    */

    while (1) {
        // Step 1: blink 1Hz
        led_blink_run(500, 500);

        // Step 5+: read HC2A-S3 and update home screen
        /*
        float temp_c = 0.0f, humi_pct = 0.0f;
        hc2a_get_temp_avg(&s_hc2a, &temp_c);
        hc2a_get_humi_avg(&s_hc2a, &humi_pct);
        home_screen_draw(temp_c, humi_pct);
        */
    }
}
