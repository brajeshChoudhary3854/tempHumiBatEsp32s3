#include "home_screen.h"
#include "glcd_cog.h"
#include <stdint.h>

// ─── 7-segment table ─────────────────────────────────────────────────────────
// bit: 0=a(top) 1=b(top-R) 2=c(bot-R) 3=d(bot) 4=e(bot-L) 5=f(top-L) 6=g(mid)
static const uint8_t SEG[10] = {
    0x3F, // 0  a b c d e f
    0x06, // 1      b c
    0x5B, // 2  a b   d e   g
    0x4F, // 3  a b c d     g
    0x66, // 4      b c   f g
    0x6D, // 5  a   c d   f g
    0x7D, // 6  a   c d e f g
    0x07, // 7  a b c
    0x7F, // 8  a b c d e f g
    0x6F, // 9  a b c d   f g
};

// Draw one 7-segment digit.
// x,y = top-left corner; dw=digit width; dh=digit height; sw=stroke width.
static void seg_digit(uint8_t x, uint8_t y, uint8_t d,
                      uint8_t dw, uint8_t dh, uint8_t sw)
{
    if (d > 9) return;
    uint8_t m  = SEG[d];
    uint8_t hw = dw - 2 * sw;           // inner horizontal segment length
    uint8_t vh = (uint8_t)(dh / 2 - sw); // height of each vertical half
    uint8_t my = y + dh / 2;            // y of the mid-rail

    if (m & 0x01) glcd_cog_fill_rect(x + sw,    y,           hw, sw, true); // a top
    if (m & 0x02) glcd_cog_fill_rect(x + dw-sw, y + sw,      sw, vh, true); // b top-R
    if (m & 0x04) glcd_cog_fill_rect(x + dw-sw, my,          sw, vh, true); // c bot-R
    if (m & 0x08) glcd_cog_fill_rect(x + sw,    y + dh - sw, hw, sw, true); // d bot
    if (m & 0x10) glcd_cog_fill_rect(x,         my,          sw, vh, true); // e bot-L
    if (m & 0x20) glcd_cog_fill_rect(x,         y + sw,      sw, vh, true); // f top-L
    if (m & 0x40) glcd_cog_fill_rect(x + sw,    my - sw/2,   hw, sw, true); // g mid
}

// ─── Icons ───────────────────────────────────────────────────────────────────

// 4×4 degree circle outline
static void draw_degree(uint8_t x, uint8_t y)
{
    glcd_cog_set_pixel(x+1, y,   true);
    glcd_cog_set_pixel(x+2, y,   true);
    glcd_cog_set_pixel(x,   y+1, true);
    glcd_cog_set_pixel(x+3, y+1, true);
    glcd_cog_set_pixel(x,   y+2, true);
    glcd_cog_set_pixel(x+3, y+2, true);
    glcd_cog_set_pixel(x+1, y+3, true);
    glcd_cog_set_pixel(x+2, y+3, true);
}

// 11×11 snowflake: cross + X arms
static void draw_snowflake(uint8_t x, uint8_t y)
{
    uint8_t cx = x + 5, cy = y + 5;
    glcd_cog_draw_hline(x,  cy, 11, true);
    glcd_cog_draw_vline(cx, y,  11, true);
    glcd_cog_draw_line(x, y, x+10, y+10, true);
    glcd_cog_draw_line(x+10, y, x, y+10, true);
    // small tick marks at arm mid-points for detail
    glcd_cog_set_pixel(cx-2, cy-1, true); glcd_cog_set_pixel(cx-2, cy+1, true);
    glcd_cog_set_pixel(cx+2, cy-1, true); glcd_cog_set_pixel(cx+2, cy+1, true);
}

// 8×11 water drop outline (hollow)
static void draw_drop(uint8_t x, uint8_t y)
{
    // tip
    glcd_cog_set_pixel(x+3, y,    true);
    glcd_cog_set_pixel(x+4, y,    true);
    // widening sides
    glcd_cog_set_pixel(x+2, y+1,  true);
    glcd_cog_set_pixel(x+5, y+1,  true);
    glcd_cog_set_pixel(x+1, y+2,  true);
    glcd_cog_set_pixel(x+6, y+2,  true);
    glcd_cog_set_pixel(x+1, y+3,  true);
    glcd_cog_set_pixel(x+6, y+3,  true);
    // full-width body sides
    glcd_cog_set_pixel(x,   y+4,  true);
    glcd_cog_set_pixel(x+7, y+4,  true);
    glcd_cog_set_pixel(x,   y+5,  true);
    glcd_cog_set_pixel(x+7, y+5,  true);
    glcd_cog_set_pixel(x,   y+6,  true);
    glcd_cog_set_pixel(x+7, y+6,  true);
    glcd_cog_set_pixel(x,   y+7,  true);
    glcd_cog_set_pixel(x+7, y+7,  true);
    // narrowing round bottom
    glcd_cog_set_pixel(x+1, y+8,  true);
    glcd_cog_set_pixel(x+6, y+8,  true);
    glcd_cog_set_pixel(x+2, y+9,  true);
    glcd_cog_set_pixel(x+5, y+9,  true);
    glcd_cog_set_pixel(x+3, y+10, true);
    glcd_cog_set_pixel(x+4, y+10, true);
}

// WiFi symbol: 3 concentric arcs + dot (11×8)
static void draw_wifi(uint8_t x, uint8_t y)
{
    // outer arc
    glcd_cog_draw_hline(x+1, y+0, 9, true);
    glcd_cog_set_pixel(x+0,  y+1, true);
    glcd_cog_set_pixel(x+10, y+1, true);
    // middle arc
    glcd_cog_draw_hline(x+3, y+2, 5, true);
    glcd_cog_set_pixel(x+2,  y+3, true);
    glcd_cog_set_pixel(x+8,  y+3, true);
    // inner arc
    glcd_cog_draw_hline(x+4, y+4, 3, true);
    glcd_cog_set_pixel(x+3,  y+5, true);
    glcd_cog_set_pixel(x+7,  y+5, true);
    // dot
    glcd_cog_draw_hline(x+4, y+7, 3, true);
}

// Battery outline + fill (16×7)
static void draw_battery(uint8_t x, uint8_t y)
{
    glcd_cog_draw_rect(x, y, 14, 7, true);
    glcd_cog_fill_rect(x+14, y+2, 2, 3, true);  // positive terminal
    glcd_cog_fill_rect(x+1,  y+1, 9, 5, true);  // charge level (~70%)
}

// ─── Main draw ───────────────────────────────────────────────────────────────

void home_screen_draw(float temp_c, float humi_pct)
{
    glcd_cog_clear();

    // ── status bar (top) ───────────────────────────────────────────────
    draw_wifi(2, 1);
    draw_battery(44, 2);
    // no separator line — cleaner look

    // ── centre divider — equal split: status bar y=0..9, each zone 59px ──
    glcd_cog_draw_hline(0, 69, 64, true);

    // ── TEMPERATURE (y = 10 .. 68) ─────────────────────────────────────
    {
        int ti = (int)temp_c;
        float frac = temp_c - (float)ti;
        if (frac < 0.0f) frac = -frac;
        int tfrac   = (int)(frac * 100.0f);
        uint8_t td1 = (uint8_t)((tfrac / 10) % 10);
        uint8_t td2 = (uint8_t)(tfrac % 10);

        uint8_t tens  = (uint8_t)(ti / 10);
        uint8_t units = (uint8_t)(ti % 10);

        // large digits — bottom 2px above divider
        uint8_t base_y = 43;
        if (tens > 0) {
            seg_digit(10, base_y, tens,  14, 24, 2);
            seg_digit(26, base_y, units, 14, 24, 2);
        } else {
            seg_digit(18, base_y, units, 14, 24, 2);
        }

        // decimal point (3×3) at digit baseline
        glcd_cog_fill_rect(42, base_y + 21, 3, 3, true);

        // two small decimal digits: 8px wide × 14px tall, stroke 2
        seg_digit(46, base_y + 5, td1, 8, 14, 2);
        seg_digit(55, base_y + 5, td2, 8, 14, 2);

        // snowflake — top-right, 3px below status bar
        draw_snowflake(47, 13);

        // °C — same relative offset as snowflake bottom +4px
        draw_degree(47, 27);
        glcd_cog_draw_string(52, 27, "C");
    }

    // ── HUMIDITY (y = 70 .. 127) ────────────────────────────────────────
    {
        int hi = (int)humi_pct;
        float frac = humi_pct - (float)hi;
        if (frac < 0.0f) frac = -frac;
        int hfrac   = (int)(frac * 100.0f);
        uint8_t hd1 = (uint8_t)((hfrac / 10) % 10);
        uint8_t hd2 = (uint8_t)(hfrac % 10);

        uint8_t tens  = (uint8_t)(hi / 10);
        uint8_t units = (uint8_t)(hi % 10);

        // drop mirrors snowflake: same x, 3px below humidity zone start
        draw_drop(47, 73);

        // % mirrors °C: same x as C, same offset within zone as C in temp
        glcd_cog_draw_string(52, 87, "%");

        // large digits — bottom 2px above canvas edge
        uint8_t base_y = 101;
        if (tens > 0) {
            seg_digit(10, base_y, tens,  14, 24, 2);
            seg_digit(26, base_y, units, 14, 24, 2);
        } else {
            seg_digit(18, base_y, units, 14, 24, 2);
        }

        // decimal point (3×3) at digit baseline
        glcd_cog_fill_rect(42, base_y + 21, 3, 3, true);

        // two small decimal digits: 8px wide × 14px tall, stroke 2
        seg_digit(46, base_y + 5, hd1, 8, 14, 2);
        seg_digit(55, base_y + 5, hd2, 8, 14, 2);
    }

    glcd_cog_update();
}
