idf_component_register(
    SRCS
        "main.c"
        "led_blink.c"
        "glcd_cog.c"
        "glcd_cog_font.c"
        "mcp3421.c"
        "battery_adc.c"
        "input_switch.c"
        "user_menu.c"
        "home_screen.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc freertos
)
