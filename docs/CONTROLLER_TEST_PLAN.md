# DS4 and original Steam Controller contribution

The owner authorized backing up this branch to Soopahfly/MC-BlueBridge.
Do not create an upstream GitHub PR or publish firmware until hardware testing
is complete and the owner is ready to submit.
Base: upstream 1.1.0, e3896aaf4a00e4e1063d4ccfceba08d8c231a8cb.
Branch: contrib/ds4-steam-controllers.

## Included

- DS4 Bluetooth touchpad mouse plus optional calibrated gyro aiming and gyro/accelerometer diagnostics.
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

This standalone branch uses upstream schema 13 plus controller-only mouse/Steam
settings (schema 14), then independent DS4 aiming settings in schema 15
(10,991-byte payload). Its schemas 14/15 are independent of the
personal OLED fork's schema 14-18 and has a different validated payload length.
It cannot import the personal fork's exports or automatically retain those flash
records. Keep that firmware and its export before testing; restoring the personal
fork requires reflashing it and restoring its own settings export. Never assume
matching schema numbers imply matching layouts. Upstream schema 7-13 flash records
migrate; schema 8-14 exports remain accepted. Controller-only schema 14 preserves
its mouse/Steam choices and defaults DS4 gyro off. New mouse and gyro options default off.

## Known limitations

Hardware support is experimental. Relative gyro aiming does not know the screen
position and cannot automatically centre a crosshair. No accelerometer tilt,
user stationary calibration, scroll/right-click gestures or haptics are included.
DS4 factory calibration and gyro/accelerometer diagnostics are implemented.
Core mouse support, firmware compatibility, drift, axes and sensitivity need real
validation. General Bluetooth keyboards/mice and the Valve USB receiver are out
of scope. Unrelated existing upstream stack/queue behavior is retained rather
than claimed fixed by this contribution. Existing hid_feedback.c excess-initializer
compiler warnings are outside this controller contribution.


## DS4 motion follow-up

- [x] Decode signed three-axis gyro/accelerometer and sensor timestamps from validated full reports.
- [x] Validate/consume Bluetooth factory calibration, with bounded setup retry/response handling.
- [x] Independent default-off DS4 aiming settings, atomic master/Steam/DS4 save and schema-14 migration.
- [x] Gyro mouse, sensitivity/inversion, touch/L2 activation, pause/reposition and optional R2 fire.
- [x] Config app controls, calibration/sensor diagnostics and old-firmware capability fallback.
- [x] Native tests for calibration CRC/ranges, units, settings, stale/duplicate/wrapped timestamps and isolation.
- [x] Actual config-page simulated serial tests, including independent DS4 and Steam options.
- [x] Build final preview.2 UF2, audit USB descriptors and verify controller-only scope.
- [x] Back up tested source changes to the owner's GitHub branch (implementation commit 80cc9b4); upstream PR remains unopened.
- [ ] Hardware: Sony DS4 v1/v2 factory calibration, full reports and sensor readout while moving/still.
- [ ] Hardware: gyro directions, sensitivity, stationary drift, touch/L2 activation, pause/R2 fire and releases.
- [ ] Hardware: invalid/missing calibration keeps motion inactive; buttons/touchpad/reconnect remain usable.
- [ ] Hardware: verify old settings migration and mouse-capable MiSTer core behavior.

Accelerometer tilt mapping remains a separate follow-up; sensor diagnostics do
not imply native six-axis output or tested motion-control compatibility.
