#pragma once

// Draw the main temperature + humidity screen.
// Canvas: DIS270 orientation — logical 64 wide × 128 tall.
// Top half (y 0-62): temperature.  Bottom half (y 64-127): humidity.
void home_screen_draw(float temp_c, float humi_pct);
