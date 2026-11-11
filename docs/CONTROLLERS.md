# Optional controller mouse and Steam BLE support

Experimental controller-only contribution based on upstream 1.1.0.

## Setup

1. Export a backup from the firmware currently flashed; see the incompatible
   personal-fork settings warning in CONTROLLER_TEST_PLAN.md before switching.
2. Build for pico2_w with the normal upstream CMake instructions and flash the
   locally built firmware. This branch has no published firmware or pull request.
3. Pair a DS4 normally, or an original Steam Controller with BLE firmware using
   Steam + Y. The Steam USB dongle is not used.
4. Serve the repository on localhost (for example `python -m http.server 8000`)
   and open `http://localhost:8000/tools/controller-config.html` in desktop Chrome
   or Edge. Close other apps using the adapter's serial port.
5. Connect, enable mouse output in single-controller MiSTer mode, save/restart,
   then reconnect. Slide/press the DS4 touchpad or Steam right pad to point/click.

Mouse off preserves normal DS4 touchpad click and maps the Steam right pad to the
right stick. Steam left pad supplies D-pad directions; grip inputs 16/17 default
unmapped but are available in normal controller remapping.

## Optional Steam gyro

Enable gyro aiming and choose 1-200% sensitivity, X/Y inversion, always/right-pad
contact/left-trigger activation, and a hold-to-pause button (default left grip).
Use pause to reposition, then release to resume without catching up paused motion.
Right pad still provides fine pointer adjustment. Optional right-trigger left-click
consumes the normal gamepad right-trigger input; the pause button is also reserved.
Pause affects gyro movement only. All choices are global and apply after restart;
master mouse off disables pointer output while preserving the saved gyro choices.

This is relative air-mouse aiming, not absolute screen pointing or automatic
recentering. No DS4 gyro or accelerometer tilt mode. Hardware drift, axes, radio
behavior, core compatibility and sensitivity still need validation. Mouse is
absent in other output modes and two-controller mode.

## Protocol and implementation

HELLO advertises touch_mouse, steam_ble and steam_gyro. Existing clients can use
TOUCH_MOUSE_GET/SET. STEAM_AIM_GET returns saved aiming fields. Atomic save:

```
BB1|1|STEAM_AIM_SET|1|100|0|0|16|1
```

Fields: gyro enabled, sensitivity, activation (0 always/1 right-pad contact/2 left
trigger), flags (1 invert X/2 invert Y/4 right-trigger fire), canonical pause button
(0-17 or 255=None), master mouse enabled. Bad fields are rejected before saving.
STEAM_GET and HID_DIAGNOSTICS expose setup/decoding information for failed tests.

DS4 mouse parsing validates full Bluetooth report length, CRC, history and contacts.
Steam input uses bounded per-slot fragmentation/delta state and asynchronous GATT
setup with stage/total timeouts. Only enabled eligible mouse configurations request
raw gyro. Fresh gyro chunks integrate receive-time deltas with fractional retention;
first samples, inactive periods and gaps over 50 ms rebase rather than catch up.

References: [Valve BLE setup](https://help.steampowered.com/en/faqs/view/1796-5FC3-88B3-C85F),
[SDL Steam protocol](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_steam.c),
[Valve IMU definitions in SDL](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/steam/controller_constants.h),
[Bluepad32 Steam parser](https://github.com/ricardoquesada/bluepad32/blob/main/src/components/bluepad32/parser/uni_hid_parser_steam.c).
The transport, parser and pointer integration are original implementation code.

Software checks: `python tests/run.py`, `node tests/test_controller_config.mjs`,
and after an ARM build `python tools/check_usb_descriptors.py build`.
Hardware checklist and contribution boundaries: CONTROLLER_TEST_PLAN.md.
