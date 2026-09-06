idf_component_register(
    SRCS "src/glcd_cog.c"
         "src/glcd_cog_font.c"
    INCLUDE_DIRS "include"
    REQUIRES driver spi_flash freertos
)
