# MC BlueBridge 1.2.0 OLED/encoder fork preview

Integrates all six Stu1983 controller improvements released in upstream v1.2.0:

- Faster Classic Bluetooth reconnect listening: 11.25 ms every 160 ms,
  interlaced, instead of the default 1.28-second interval.
- DualShock 3 Bluetooth authentication/activation, steady player LED, separate
  motor feedback with retrying stop reports, and readable adapter BT address.
  DS3 still requires its adapter address to be written over USB for pairing.
- 8BitDo Bluetooth X-input Home, Xbox button order, sticks, triggers and rumble.
- Select+Start held for 600 ms opens the MiSTer menu on pads without Home.
- NSO SNES steady player LED without changing its input report mode.
- L3+R3 held for five seconds sends the MiSTer PSX analog/digital shortcut,
  with progress LEDs on DS3. The mode alternates; the core does not report it.

Two separate UF2s are included:

- mc-bluebridge-1.2.0-oled-preview.uf2: OLED and encoder/trackball support.
- mc-bluebridge-1.2.0-oled-preview-no-oled.uf2: display/physical controls excluded.

Both retain our existing DS4 touchpad/gyro, horizontal-inversion default,
light-bar themes and low-battery blink, original Steam Controller BLE/gyro,
USB input queue, save and stack fixes. Physical panel and PEW settings remain
available in the OLED build. Both use the shared version 1.2.0-oled; oled_panel
capability identifies the build. Two connected-controller slots are retained.

## Updating

Export the adapter configuration first. Select the correct UF2 in the
BlueBridge web firmware updater and use Preserve Configuration. Press PS to
reconnect the DS4 after the adapter restarts. Schema remains 20 (11110 bytes),
with the exact 1.1.6 configuration layout; no new saved fields are appended.
The upstream stock v1.2.0 schema 14 has a different layout and cannot read this
fork's schema-20 configuration. These files are the maintained fork builds.

Existing patched Companion Remote settings remain compatible. The released
Companion Remote 4.3.0 also contains the upstream DS4 web controls. No additional
GUI patch is needed for the faster reconnect schedule, NSO LEDs or shortcuts.
DS3 RUMBLE_GAIN is a runtime command and is not a new persisted GUI preference.

If DS4 gyro reserves L3 or R3 as its pause button, change that pause setting or
disable gyro before using the L3+R3 analog shortcut. The pause button is
consumed by mouse mode before the gamepad shortcut sees it.

## Validation and limits

Both Pico 2 W Release builds pass with the project's stack-frame checks.
The full existing native suite passes: DS4 touchpad/gyro/lightbar, Steam BLE
and gyro, input, queue, save/migration, OLED/encoder/trackball and panel tests.
A new feedback test passes all 512 big/small-motor combinations through actual
PID report handling, plus fallback, stop, motor-only changes and slot isolation.
The USB configuration-page tests pass. Eighteen imported connection, DS3,
shortcut and feedback functions match the released upstream implementations.
The fork retains its earlier two-slot feedback-context initializer fix.
UF2 family/manifest/address boundaries and archive hashes are checked separately.

These UF2s have not been flashed during this update. Faster DS4 reconnects,
new controller paths and both physical build variants require hardware checks.
The contributor reported successful NSO/DS3/8BitDo hardware tests upstream;
those results do not establish performance on the user's adapter. The existing
DS4 mouse/gyro and red light bar were physically tested on the prior 1.1.6 fork.
Low-battery blinking still needs a depleted-controller test.

Upstream release and source:
https://github.com/Anime0t4ku/MC-BlueBridge/releases/tag/v1.2.0
