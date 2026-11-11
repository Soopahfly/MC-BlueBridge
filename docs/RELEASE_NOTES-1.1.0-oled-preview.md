# 1.1.0 OLED fork preview

For Raspberry Pi Pico 2 W. Software-tested; physical DS4 mouse/MiSTer, panel
hardware and end-to-end latency checks remain pending.

## Included

- Upstream v1.1.0 at e3896aa: DualSense USB output for macOS/SDL, 8BitDo BLE
  rumble report discovery, and 8BitDo stick/trigger mapping fixes.
- All fork features retained: SSD1306 OLED, live controller display, turbo rings,
  PEW, None/Encoder/PIM447 selection, OLED on/off, bounded I2C, compact snapshots,
  4 KiB core stacks, independent turbo, reliable/coalesced saves and USB queues.
- New optional DS4 Bluetooth touchpad-to-USB-mouse output in single-controller
  MiSTer mode. Off by default. Finger movement moves the pointer; mechanical
  touchpad press is left-click/drag. Other gamepad controls remain available.
- The included web GUI has a separate saved DS4 mouse checkbox and restart button.
  No change has been made to the external Companion app/website.

## Install

1. Export configuration before upgrading, especially if you may downgrade.
2. Flash `mc-bluebridge-1.1.0-oled-preview.uf2` for the usual panel-enabled build.
   The `no-oled` UF2 excludes physical panel hardware and supports the DS4 mouse too.
3. Open the included `tools/oled-config.html` via localhost in desktop Chrome/Edge.
4. For DS4 mouse use, select single-controller MiSTer output, enable the DS4
   touchpad mouse checkbox, save, then reconnect after restart.

Schema 17 appends one mouse-enable byte to schema 16 (11,047-byte payload).
Schema 7-16 flash records migrate; schema 8-16 exports remain importable.
Existing OLED/control/PEW/profile settings survive; mouse defaults off.
Earlier firmware cannot read schema 17. Retain your pre-upgrade export.

## DS4 mouse limits

Full 78-byte Bluetooth report 0x11 required; minimal compatibility reports do
not provide touch position. This is a relative one-finger mouse with fixed
sensitivity, left-click and drag. No scrolling, right-click gestures, tap-to-click
or inertia. Mouse is a separate HID interface, not joystick emulation. A MiSTer
core must support a mouse for it to respond. Multi-controller and other USB output
modes do not expose the mouse. PIM447 remains a physical menu control.

## Original Steam Controller

Not implemented in this preview. It needs proprietary BLE initialization and
report decoding before its right pad can feed the mouse path. The USB dongle is
outside this project's Bluetooth-only controller scope. See the follow-up plan.

## Validation

- 13 native C regression executables plus Web Serial GUI tests.
- Pico 2 W Release builds with OLED enabled and disabled.
- ARM stack-usage/linker audit: two 4096-byte stacks, largest project frame 600 bytes.
- Actual ELF USB descriptor audit: interface counts, lengths, endpoint uniqueness,
  HID instance budget and mouse report usage across seven configurations.
- DS4 tests cover CRC/length/count validation, history replay, contact/lift changes,
  clicks, timestamp wrap, movement queue retention and button-state edges.
- DualSense tests cover report encoding, feature lengths and rumble forwarding.

Physical pairing, pointer direction/sensitivity, click/drag, USB enumeration,
mouse/gamepad coexistence, controller reconnect, capture/disconnect release,
OLED/control behavior and actual latency still require the owner's hardware.

[Full guide](https://github.com/Soopahfly/MC-BlueBridge/blob/main/docs/OLED.md)
