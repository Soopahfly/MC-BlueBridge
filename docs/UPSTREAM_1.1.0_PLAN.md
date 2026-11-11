# Upstream 1.1.0 and touchpad work

Publication target: **Soopahfly/MC-BlueBridge only**. Upstream is read-only.

- [x] Merge upstream v1.1.0 (e3896aa) and retain fork features/fixes.
- [x] Cover new DualSense output encoding and saved output-mode bounds in tests.
- [x] Add optional DS4 relative mouse, full-report validation and bounded output queue.
- [x] Add independent saved mouse checkbox to the included USB web GUI.
- [x] Migrate schema 13/16 configurations to schema 17 without losing panel/profile data.
- [x] Pass native/Web Serial tests, both Release builds, stack and USB descriptor audits.
- [x] Publish the new preview UF2s/checksums and final documentation to the owner's fork.
- [ ] Hardware: DS4 Bluetooth pairing, mouse/gamepad enumeration, pointer movement,
      lift/re-touch, click/drag, capture/disconnect releases and reconnect.
- [ ] Hardware: panel regression and end-to-end latency comparison.
- [ ] Follow-up: original Steam Controller BLE initialization/report reassembly,
      right-pad relative pointer and click mapping, then physical verification.

The DS4 implementation is original code based on protocol-layout facts. Steam
support cannot be obtained simply by enabling the same DS4 parser: the original
Valve controller sends proprietary BLE reports and needs its own initialization.
Current generic controller identification is not a claim of complete Steam support.
Reference: https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_steam.c

Published preview: https://github.com/Soopahfly/MC-BlueBridge/releases/tag/v1.1.0-oled-preview
Includes both UF2 variants and verified SHA-256 checksums. General Bluetooth
keyboard/mouse input and USB keyboard output are not implemented by this change.
