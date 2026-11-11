# 1.1.1 original Steam Controller BLE preview

Pico 2 W firmware based on upstream MC-BlueBridge 1.1.0 plus our fork changes.
**Experimental: original Steam Controller hardware testing remains outstanding.**

## Added

- Dedicated BLE discovery/subscription/configuration for the original 2015
  Valve Steam Controller; no proprietary receiver or USB controller input.
- Per-slot fragmented/delta report parser with bounded buffers, atomic updates,
  report-length/flag checks and fragment timeouts.
- Standard gamepad buttons/triggers, left stick, left-pad directions and click,
  back grips, and right pad as a right stick when mouse mode is off.
- Optional right-pad USB mouse plus pad-press left-click/drag in single-controller
  MiSTer mode. The existing DS4 mouse checkbox now describes both controllers.
- Event-driven mouse button retention, idle motion expiry, and STEAM_GET diagnostics.

All prior OLED/encoder/PIM447/PEW, DS4 mouse, DualSense USB output and performance
fixes are retained. Configuration schema 17 is unchanged from the 1.1.0 fork;
the mouse option is the same saved setting, off by default on fresh settings.
Older 1.0.6/schema-16 configurations still migrate. Export settings before flashing.

## Test on hardware

1. Ensure the original Steam Controller has Valve's BLE-capable firmware.
2. Flash `mc-bluebridge-1.1.1-oled-preview.uf2`. The optional no-OLED binary has
   the same Steam/DS4 support without physical panel hardware.
3. Start pairing in BlueBridge; hold Steam + Y while turning on the controller.
4. In the included USB web GUI, use single-controller MiSTer mode and enable
   the DS4/original Steam Controller mouse option, save and reconnect.
5. Slide/press the right pad in a mouse-capable core. Test lift/re-touch, held
   click/drag, sleep/reconnect, and button release on capture/disconnect.
6. If it fails, obtain STEAM_GET and HID_DIAGNOSTICS data before disconnecting.

Left pad stays a gamepad D-pad; grips are remappable inputs 16/17 and default
unmapped. No scroll/right-click gestures, tap-to-click, inertia, configurable
sensitivity, haptic or gyro output in this initial version. Newer Steam models,
Steam Deck, general Bluetooth keyboards/mice and the Valve USB dongle are not
covered. Mouse is not exposed in multi-controller or non-MiSTer USB output modes.

Validation uses synthetic input/GATT events, native C regression tests, web GUI
tests, both Release builds, and stack/USB descriptor audits. Physical pairing,
exact report compatibility, pointer behavior, MiSTer enumeration and latency
still require testing on the owner's controller and MiSTer.

[Setup guide](https://github.com/Soopahfly/MC-BlueBridge/blob/main/docs/OLED.md)
[Progress and hardware checklist](https://github.com/Soopahfly/MC-BlueBridge/blob/main/docs/STEAM_PLAN.md)
