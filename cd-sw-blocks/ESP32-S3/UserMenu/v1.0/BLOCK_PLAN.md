# BLOCK PLAN: ESP32-S3 / UserMenu / v1.0

Generated: 2026-09-06
Platform:  ESP32-S3
Interface: Software (uses GlcdCog128x64 + InputSwitch)
Reference: GlcdCog128x64/v1.0, InputSwitch/v1.0
Standard:  Follow cd-sw-blocks/CLAUDE.md

## Block Description
Scrollable user menu for 128×64 GLCD display.
Title bar (inverted) + up to 4 visible items + scrollbar.
Cursor navigation with UP/DOWN; SELECT triggers item action callback.
Supports up to 16 items with automatic scroll.

## Hardware
- Requires: GlcdCog128x64 block (display)
- Requires: InputSwitch block (UP, DOWN, SELECT, BACK buttons)

## Display Layout
- Row 0–8  : Title bar (filled, inverted text)
- Row 10–53: 4 visible menu items (9px each)
- Scrollbar : right edge (2px wide indicator)

## Development Steps

### Step 1: Draw Function
- [x] `draw_menu()` — clear, draw title bar, draw items, draw scrollbar
- [x] Selected item: filled bar (inverted)
- [x] Non-selected: plain text with 4px indent
- [x] Scrollbar: proportional indicator on right edge
- **Test**: Static menu renders correctly on display

### Step 2: Navigation
- [x] UP: cursor--, scroll if needed
- [x] DOWN: cursor++, scroll if needed
- [x] Wrap: stops at first/last item (no wrap-around)
- **Test**: Navigate through more items than visible, scrollbar moves

### Step 3: Action on SELECT
- [x] SELECT: call items[cursor].action() if not NULL
- **Test**: Select item → action callback fires

### Step 4: Integration with InputSwitch
- [x] `user_menu_handle_event(id, event)` — call from switch callback
- **Test**: Pass SW_UP/SW_DOWN/SW_SELECT events → menu responds

## Usage Example
```c
void action_show_temp(void) { /* switch to temperature screen */ }
void action_show_bat(void)  { /* switch to battery screen */ }
void action_settings(void)  { /* open settings */ }

const menu_t main_menu = {
    .title = "MAIN MENU",
    .items = {
        {"Temperature",  action_show_temp},
        {"Humidity",     NULL},
        {"Battery",      action_show_bat},
        {"Settings",     action_settings},
        {"About",        NULL},
    },
    .count = 5,
};

user_menu_init(&main_menu);

// In switch callback:
void sw_cb(switch_id_t id, switch_event_t ev) {
    user_menu_handle_event(id, ev);
}
```

## Notes
- Call glcd_cog_init() BEFORE user_menu_init()
- Call input_switch_init() with same sw_cb before or after
- For submenus: action callback calls user_menu_init() with new menu
- To return from submenu: SW_BACK handler calls user_menu_init() with parent
