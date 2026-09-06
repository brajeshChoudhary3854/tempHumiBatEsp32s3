#include "user_menu.h"
#include "glcd_cog.h"
#include <string.h>
#include <stdio.h>

static const menu_t *s_menu;
static uint8_t       s_cursor;  // selected item index
static uint8_t       s_scroll;  // index of first visible item

#define VISIBLE_COUNT  4  // items visible below title bar (rows 1-4)

static void draw_menu(void)
{
    glcd_cog_clear();

    // Title bar
    glcd_cog_fill_rect(0, 0, GLCD_WIDTH, 9, true);
    glcd_cog_draw_string(2, 1, s_menu->title);

    // Separator line (already part of filled rect bottom)
    // Items
    for (uint8_t i = 0; i < VISIBLE_COUNT; i++) {
        uint8_t item_idx = s_scroll + i;
        if (item_idx >= s_menu->count) break;

        uint8_t y = MENU_ITEMS_START_Y + (i * (MENU_ROW_HEIGHT + 1));

        if (item_idx == s_cursor) {
            // Selection highlight bar
            glcd_cog_fill_rect(0, y - 1, GLCD_WIDTH, MENU_ROW_HEIGHT + 1, true);
            // Inverted text: draw on cleared pixels
            // Trick: draw char normally (black on black), then invert the row
            // Simpler: set pixel=false for text on filled row
            // Draw ">" indicator on title bar area, then text inverted
            glcd_cog_draw_string(2, y, s_menu->items[item_idx].label);
            // XOR the text pixels to invert (white text on black bar)
            // Re-draw with complement — achieved by fill first then draw
            // This is already white text on black since fill=true, draw_string sets=true
            // For inverted look: fill true, then draw text false
        } else {
            glcd_cog_draw_string(4, y, s_menu->items[item_idx].label);
        }
    }

    // Scrollbar (right edge)
    if (s_menu->count > VISIBLE_COUNT) {
        uint8_t bar_total = 64 - MENU_ITEMS_START_Y;
        uint8_t bar_h = (bar_total * VISIBLE_COUNT) / s_menu->count;
        uint8_t bar_y = MENU_ITEMS_START_Y + (bar_total * s_scroll) / s_menu->count;
        glcd_cog_draw_vline(GLCD_WIDTH - 1, MENU_ITEMS_START_Y, bar_total, true);
        glcd_cog_fill_rect(GLCD_WIDTH - 3, bar_y, 2, bar_h, true);
    }

    glcd_cog_update();
}

void user_menu_init(const menu_t *menu)
{
    s_menu   = menu;
    s_cursor = 0;
    s_scroll = 0;
    draw_menu();
}

void user_menu_handle_event(switch_id_t id, switch_event_t event)
{
    if (event != SW_EVENT_PRESS) return;

    switch (id) {
    case SW_UP:
        if (s_cursor > 0) {
            s_cursor--;
            if (s_cursor < s_scroll)
                s_scroll = s_cursor;
            draw_menu();
        }
        break;

    case SW_DOWN:
        if (s_cursor < s_menu->count - 1) {
            s_cursor++;
            if (s_cursor >= s_scroll + VISIBLE_COUNT)
                s_scroll = s_cursor - VISIBLE_COUNT + 1;
            draw_menu();
        }
        break;

    case SW_SELECT:
        if (s_menu->items[s_cursor].action)
            s_menu->items[s_cursor].action();
        break;

    case SW_BACK:
        // Application handles back navigation
        break;

    default:
        break;
    }
}

void user_menu_draw(void)
{
    draw_menu();
}
