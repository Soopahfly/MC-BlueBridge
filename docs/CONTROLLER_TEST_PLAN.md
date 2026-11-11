# DS4 and original Steam Controller contribution

The owner authorized backing up this branch to Soopahfly/MC-BlueBridge.
Do not create an upstream GitHub PR or publish firmware until hardware testing
is complete and the owner is ready to submit.
Base: upstream 1.1.0, e3896aaf4a00e4e1063d4ccfceba08d8c231a8cb.
Branch: contrib/ds4-steam-controllers.

## Included

- DS4 Bluetooth touchpad to relative USB mouse and pad-press left-click/drag.
- Original 2015 Steam Controller BLE transport, bounded report assembly,
  gamepad inputs, left-pad D-pad, remappable grips and right-pad mouse/stick.
- Optional Steam gyro-to-mouse aiming with sensitivity, inversion, activation,
  pause/reposition and right-trigger mouse-fire settings.
- Separate controller-only Web Serial config page, saved settings and migrations.
- Controller-focused native/GUI tests and USB descriptor audit.
- Minimal prerequisites: missing controller-state declaration include in the
  platform wrapper, static serial reply buffer, and static import/reset scratch
  configuration objects to avoid large stack allocations on those paths.

## Excluded

No OLED/display driver, PEW animation, encoder, trackball, physical menu or panel
configuration. No general gamepad queue rewrite, turbo timer, input caching,
flash-save coalescing, broad stack/latency hardening or release publication.
Existing upstream output modes, normal gamepad queue and input scheduling remain.

## Local software checks

- [x] DS4 pointer/CRC/history/contact and mouse queue regression tests.
- [x] Steam GATT setup, per-client setup commands, failure/timeout/cleanup tests.
- [x] Steam fragmented/delta decoder and gyro mapping/timing tests.
- [x] Configuration defaults/atomic validation, flash round-trip, schema-13 migration/import/reset.
- [x] Actual controller config page with simulated serial, restart and old-capability fallback.
- [x] Clean standalone Pico 2 W Release build, UF2 manifest and actual ELF USB descriptor audit.
- [x] Review the upstream-relative file list and added source for controller-only scope.

## Required hardware tests before publication

- [ ] Back up settings from the firmware currently flashed.
- [ ] Flash the controller-only build and confirm gamepad-only USB operation with mouse off.
- [ ] DS4: pair, all ordinary controls, pointer movement, pad press/release/drag,
  contact lift/change, capture release, sleep/reconnect and radio-loss release.
- [ ] Steam: confirm BLE-capable firmware; pair with Steam + Y without the dongle.
- [ ] Steam: test buttons, stick, both triggers, left-pad directions/click and both grips.
- [ ] Steam: right pad as right stick with mouse off, mouse with it on, held drag during idle,
  lift/re-touch, capture, reconnect and disconnect release.
- [ ] Steam gyro: stationary drift, axes/inversion, sensitivity, all activation options,
  pause/reposition, pad fine adjustment, trigger-fire press/release/drag and disabled behavior.
- [ ] Config app: save/restart/reconnect, restore old upstream export, reset, and malformed-value rejection.
- [ ] MiSTer: verify pointer/click in an actual mouse-capable core and assess gyro aiming.
- [ ] Other output modes and two-controller mode: ordinary gamepads, no extra mouse interface.
- [ ] Measure latency separately; no hardware timing result is claimed by software tests.
- [x] Owner authorizes pushing the branch to their own fork for backup.
- [ ] Hardware tests complete and owner is ready to submit the upstream PR.

## Firmware/settings separation

This standalone branch uses upstream schema 13 plus a six-byte controller-only
schema-14 extension (10,986-byte payload). Its schema 14 is independent of the
personal OLED fork's schema 14-18 and has a different validated payload length.
It cannot import the personal fork's exports or automatically retain those flash
records. Keep that firmware and its export before testing; restoring the personal
fork requires reflashing it and restoring its own settings export. Never assume
matching schema numbers imply matching layouts. Upstream schema 7-13 flash records
migrate; schema 8-13 exports remain accepted. New mouse and gyro options default off.

## Known limitations

Hardware support is experimental. Relative gyro aiming does not know the screen
position and cannot automatically centre a crosshair. No accelerometer tilt,
DS4 gyro, gyro calibration, scroll/right-click gestures or haptics are included.
Core mouse support, firmware compatibility, drift, axes and sensitivity need real
validation. General Bluetooth keyboards/mice and the Valve USB receiver are out
of scope. Unrelated existing upstream stack/queue behavior is retained rather
than claimed fixed by this contribution. Existing hid_feedback.c excess-initializer
compiler warnings are outside this controller contribution.
