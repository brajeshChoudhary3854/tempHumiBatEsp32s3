#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "input_switch.h"

#define MENU_MAX_ITEMS     16
#define MENU_VISIBLE_ROWS   5  // rows visible on 128×64 display (row 0 = title)
#define MENU_ROW_HEIGHT     8  // pixels per row (5×7 font + 1px gap)
#define MENU_TITLE_Y        0
#define MENU_ITEMS_START_Y  10 // below title + separator line

typedef void (*menu_action_t)(void);

typedef struct {
    const char    *label;
    menu_action_t  action;   // called on SW_SELECT press (NULL = submenu placeholder)
} menu_item_t;

typedef struct {
    const char       *title;
    const menu_item_t items[MENU_MAX_ITEMS];
    uint8_t           count;
} menu_t;

// Init — pass pointers to GLCD (already inited) and attach to switch events
void user_menu_init(const menu_t *menu);

// Call from main loop OR switch callback
void user_menu_handle_event(switch_id_t id, switch_event_t event);

// Force redraw (e.g. after display cleared by another screen)
void user_menu_draw(void);
