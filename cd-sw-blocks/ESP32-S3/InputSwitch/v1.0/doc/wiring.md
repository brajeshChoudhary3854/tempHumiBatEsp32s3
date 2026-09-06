# Wiring — InputSwitch / ESP32-S3

## Active-Low Connection (recommended — uses internal pull-up)

```
ESP32-S3                Switch
---------              -------
GPIO2  ─────────────── One leg
GND    ─────────────── Other leg
(internal pull-up enabled in driver)
```

## Three-Switch Layout (UP / DOWN / SELECT)

| Switch   | ESP32-S3 Pin | active_low | Function        |
|----------|--------------|------------|-----------------|
| SW_UP    | GPIO2        | true       | Menu up         |
| SW_DOWN  | GPIO3        | true       | Menu down       |
| SW_SELECT| GPIO4        | true       | Confirm/Enter   |
| SW_BACK  | GPIO7        | true       | Back/Cancel     |

## Notes
- active_low = true: Press = GPIO LOW (with internal pull-up) ← recommended
- active_low = false: Press = GPIO HIGH (external pull-down needed)
- Driver uses 10ms scan, 20ms debounce by default
- HOLD event fires after hold_ms (e.g. 1000ms) of continuous press
- Tactile switches: any SPST normally-open button
- No external resistors needed with active_low=true (internal pull-up ~45kΩ)
