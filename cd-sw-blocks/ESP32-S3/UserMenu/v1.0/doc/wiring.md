# Wiring — UserMenu / ESP32-S3

## No additional wiring — uses GlcdCog128x64 + InputSwitch

See wiring in:
- [../../../GlcdCog128x64/v1.0/doc/wiring.md](../../../GlcdCog128x64/v1.0/doc/wiring.md)
- [../../../InputSwitch/v1.0/doc/wiring.md](../../../InputSwitch/v1.0/doc/wiring.md)

## Required: Display + 3 Buttons

| Block         | Pins Used          |
|---------------|--------------------|
| GlcdCog128x64 | GPIO8-12 (SPI)     |
| InputSwitch   | GPIO2,3,4 (UP/DOWN/SELECT) |

## Display Layout

```
┌────────────────────────────┐  Y=0
│  MENU TITLE (inverted bar) │  9px
├────────────────────────────┤  Y=10
│► Item 1 (selected)         │  9px
│  Item 2                    │  9px
│  Item 3                    │  9px
│  Item 4                    │  9px ──── Y=46
│                 │  scrollbar (right edge)
└────────────────────────────┘  Y=63
```
