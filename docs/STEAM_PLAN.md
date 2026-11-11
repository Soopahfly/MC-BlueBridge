# Original Steam Controller BLE preview

Target: the 2015 Valve Steam Controller with two round pads and BLE firmware.
Publication is restricted to **Soopahfly/MC-BlueBridge**. Upstream is read-only.

- [x] Discover Valve BLE service/input/feature characteristics independently of generic HID.
- [x] Add bounded asynchronous setup, subscription, raw-pad commands and disconnect cleanup.
- [x] Add per-controller fragmented/delta report decoding and malformed-input checks.
- [x] Preserve buttons, triggers, left stick, left-pad D-pad/click and back grip inputs.
- [x] Feed the right pad/click to USB mouse when enabled; use right stick when disabled.
- [x] Keep event-driven held mouse buttons through idle and prevent stale-motion replay.
- [x] Reuse the optional schema-17 mouse setting and update the web GUI/demo.
- [x] Add synthetic transport/decoder regression tests and Steam setup diagnostics.
- [x] Complete both firmware builds, full regression tests and static audits.
- [x] Publish preview firmware/checksums to the owner's fork: [v1.1.1-oled-preview](https://github.com/Soopahfly/MC-BlueBridge/releases/tag/v1.1.1-oled-preview).
- [ ] Hardware: confirm BLE firmware and pair with Steam + Y, without the USB dongle.
- [ ] Hardware: check each gamepad control, left pad directions and both grip mappings.
- [ ] Hardware: check right-pad pointer, click/drag, pause during drag, lift/re-touch and capture release.
- [ ] Hardware: check mouse off/right stick, reconnect after controller sleep, radio-loss release and two-slot gamepad mode.
- [ ] Hardware: collect STEAM_GET/HID_DIAGNOSTICS reports if setup or decoding fails; measure latency separately.

The new transport is checked with fake GATT events and the decoder with synthetic
packets. These do not prove the exact behavior of the owner's controller firmware.
No physical pairing, MiSTer enumeration, sensitivity or end-to-end timing is
claimed yet. All existing DS4/OLED/encoder/trackball features remain in scope.


## Optional gyro configuration (1.1.2)

- [x] Add boot-applied schema-18 settings, defaults, strict validation and migration preserving mouse choice.
- [x] Expose enable, sensitivity, inversion, activation, pause button and trigger-fire in the config app/demo.
- [x] Save gyro/master pointer settings atomically and keep older firmware capability fallback.
- [x] Request raw gyro only when enabled in eligible mouse mode; keep per-client BLE command buffers.
- [x] Integrate fresh gyro chunks, fractional motion and wrap-safe bounded timing; rebase after pauses/gaps.
- [x] Test sensitivity, inversion, activation, pause, capture/mode gating, trigger release and schema migration.
- [x] Complete both firmware builds and static audits.
- [ ] Publish 1.1.2 preview firmware to the owner's fork.
- [ ] Hardware: verify gyro axes, rest drift, activation, pause/reposition and trigger click/drag/release.
- [ ] Hardware: tune sensitivity in a mouse-capable MiSTer core and measure end-to-end latency.
