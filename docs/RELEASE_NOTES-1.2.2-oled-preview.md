# MC BlueBridge 1.2.2 OLED/encoder fork preview

Touchpad right-click is now optional and defaults OFF. With it off, a physical
click anywhere on the DS4 touchpad sends left-click. Enable the checkbox in
Companion Remote: BlueBridge -> select DS4 -> Touchpad & Gyro -> Enable
right-click on the right half of the DS4 touchpad -> Save right-click option.
The setting is adapter-wide, saved and applied without restarting. A button
already held keeps its original choice until released; the next press uses
the new option. Mouse output must also be enabled in single-controller MiSTer
mode, outside Controller Tester. Gyro pause on touchpad click consumes either
mouse button. R2 mouse fire can still combine with right-click.

Both OLED/encoder/trackball and no-OLED variants include this setting, retain
independent DS4 motors, and report version 1.2.2-oled. The OLED capability
distinguishes them. Steam and other controller behaviour are unchanged.

## Configuration and GUI

Schema 21 appends one boolean byte to schema 20 (11110 -> 11111 bytes). Existing
saved settings, controller records, palettes and gyro choices are retained;
right-click defaults off on migration, import and reset. Existing stock schema
14 conversion remains available. Export a backup before updating and select
Preserve Configuration. Older firmware cannot read schema 21: use the earlier
backup when downgrading.

Apply add_ds4_right_click.py on MiSTer with --install, then restart Companion
Remote. It supports the native Companion Remote 4.3.x page, prepares/validates
both files before writing, creates timestamped backups, and supports --restore
TIMESTAMP. A later upstream update may replace this local GUI patch. The old
firmware hides the checkbox until an adapter advertising ds4_right_click is
connected. The setting uses DS4_MOUSE_GET/SET and the /api/bluebridge/ds4-mouse
POST route. Values are strictly boolean; older firmware is rejected safely.

## Validation

Full native regression suite passes, including schema-20 byte preservation,
default off, live updates, persistence, reset and malformed import rejection.
Pointer tests exercise both option states and ensure toggles never change a
held button. Stock14 migration, Steam, rumble/busy-stop, lightbar, OLED and
panel tests pass. GUI tests cover actual selected-adapter HTTP routes, method
and capability checks, invalid values, read failures, dirty edits, save failure
and switching adapters during a save. Full embedded Python/JavaScript syntax
checks pass. Both Release builds pass stack-frame checks and UF2 checks.
Physical left/right-click behaviour remains for the user to confirm.

Live check (18 November 2026): Companion Remote GUI patch was installed and
its marker verified over HTTP. No-OLED firmware 1.2.2/schema 21 was installed
with Preserve Configuration. Every pre-existing configuration byte after the
schema field matches the pre-update export; one zero byte was appended.
The real API successfully enabled/disabled right-click and rejected a numeric
value with HTTP 400. Right-click is left OFF. Physical clicks remain for the
user to verify. Independent DS4 rumble remains included.
