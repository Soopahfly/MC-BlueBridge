# 1.0.5 OLED/trackball preview

Pico 2 W firmware preview for hardware testing. **Not yet flashed or validated
on physical OLED/encoder/PIM447 hardware; controller latency is unmeasured.**

## Included

- SSD1306 128x64 I2C display with live PlayStation, Xbox and Nintendo/8BitDo
  layouts, pressed-input highlights, stick/trigger movement and turbo rings.
- A simpler status screen and the per-profile optional three-word PEW animation.
- An independent saved OLED on/off setting in the included USB web GUI.
- Saved physical-control selection: None, Encoder or Pimoroni PIM447 trackball.
- Non-blocking trackball I2C driver with filtered movement, short/held clicks,
  identity checks, timeouts and RGBW status feedback.
- Automatic migration to configuration schema 16, preserving existing profiles.
- Documentation screenshots, GIF demonstrations and a simulated config GUI.

One panel-enabled firmware supports all control choices. No trackball-specific
binary is needed. Fresh settings use OLED on and control None. Earlier settings
retain OLED on and migrate to Encoder to preserve the previous behavior.

## Install and configure

1. Export your existing configuration before upgrading. Older firmware cannot
   read schema 16; keep the old export if you plan to downgrade.
2. Hold BOOTSEL while connecting the Pico 2 W to USB, then copy the supplied UF2
   to its bootloader drive. It restarts automatically.
3. From the repository root, run
   `python -m http.server 8765 --bind 127.0.0.1 --directory tools`.
4. Open `http://localhost:8765/oled-config.html` in desktop Chrome/Edge and connect
   to the adapter. Close other applications using its serial port first.
5. Set Enable OLED display and Physical menu control, then Save and restart.
   Reconnect from the web GUI after the adapter returns.

Default wiring: OLED SDA GP4 / SCL GP5; encoder A GP6 / B GP7 / switch GP8;
PIM447 SDA GP2 / SCL GP3. Use 3.3 V and shared GND. See [the full guide](OLED.md)
for physical pin numbers and bare-encoder wiring.

The included web page supports these settings. The external MiSTer Companion
website/app has not been changed. Physical controls/display do not work in a
build made with BB_ENABLE_OLED=OFF.

## Verification completed

- Pico SDK 2.3.1 / Arm GCC 14.3 builds with panel support enabled and disabled.
- Native C tests for controls, configuration migration/persistence, display
  traffic suppression, input graphics, turbo assignments and trackball handling.
- Simulated Web Serial tests for all control choices, OLED on/off, profile
  settings, restart/reconnect and configuration before pairing.
- PNG/GIF frames generated from actual firmware rendering functions; web-GUI
  screenshot captured with a simulated adapter.

## Hardware checks still needed

- OLED address, cold/warm start, off/on and encoder direction/click/hold.
- PIM447 detection, movement sensitivity, quick taps, long holds and LEDs.
- Persistence after power removal and switching between physical controls.
- Bluetooth/USB report diagnostics with the panel enabled/disabled, including
  turbo, PEW and two-controller operation.

The trackball driver is specific to the Pimoroni PIM447 I2C breakout. Bare
BlackBerry trackball boards with individual direction outputs are not supported.
