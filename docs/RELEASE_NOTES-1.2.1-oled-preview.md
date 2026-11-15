# MC BlueBridge 1.2.1 OLED/encoder fork preview

DS4 mouse mode now maps a physical touchpad click by the active finger's
position: left half = USB left button, right half = USB right button. The
midpoint is X=960 of 1920. The choice is latched until the physical button is
released, including while dragging across the middle or lifting the finger.
A click with no known active contact falls back to left. Invalid reports do
not alter the click latch. Tap-to-click and two-finger gestures are not added.
The existing mouse-output setting gates both buttons and still defaults off.
No additional GUI patch or saved setting is required.

DS4 output now drives its large/left and small/right motors independently when
the host supplies separate values. This extends the upstream DS3 motor path.
For MiSTer PSX-style effects (variable large motor plus small motor on/off),
the imported PID decoder recovers the two values. Other effects fall back to
combined strength sent to both motors. No promise is made for exact recovery
of arbitrary independent variable-strength effects through HID PID.

Colour and low-battery updates retain the last accepted motor pair. Rumble
updates keep the selected colour and blink phase. Busy DS4 starts/stops retry;
explicit timeout stops clear both motors, including effects whose combined
strength rounds to zero. Reconnect starts with a cleared motor cache.
The existing rumble tester still drives both motors together. In MiSTer's
Menu core, hold L and press X or Y (SNES-style labels) to test the two motors.

Both OLED/encoder/trackball and no-OLED variants include the changes and all
1.2.0 upstream connectivity improvements. Both identify as 1.2.1-oled; the
OLED capability distinguishes the build. Schema remains 20 / 11110 bytes.
DS4/Steam gyro, colour themes, panel controls and existing preferences remain.
Cross/Circle swap is available through existing MiSTer or BlueBridge mappings;
no separate swap toggle is added.

## Installation and checks

Export configuration, select the correct UF2, and choose Preserve Configuration.
Press PS to reconnect after the restart. Existing patched Companion Remote or
Remote 4.3.0 DS4 controls work with this firmware. Stock upstream schema 14
now migrates to this fork's schema 20 by exact payload size, retaining its
controller/profile prefix, mouse/gyro and colours. The older OLED schema 14
uses a different size and retains its existing migration path. Both flash
load and CDC import handle stock v1.2.0 exports. Invalid data/checksums are
rejected. Tests cover saved controller profiles and the real adapter export.

First check right-click in a mouse-capable core, outside Controller Tester.
In Amiga, hold a right-half click to check the menu button; left-half clicks
and dragging should continue to work. If touchpad click is your gyro pause
button, it is reserved for pause and suppresses both mouse buttons. R2 mouse
fire remains a left-click and can be held together with touchpad right-click.

Validation: full native regression suite; right/left split boundary, drag
latching, no-contact fallback, invalid-report state retention, release queue,
controller isolation and gyro interactions; independent motor packet order,
CRC, colour/blink and explicit-stop handling; real output/service functions
with a mocked transport/clock covering busy start/stop, activation, tiny effects,
combined fallback and per-slot caches. Both Release builds pass project stack
checks. UF2 family/manifest/settings-flash checks are recorded separately.
Physical right-click and independent DS4 rumble remain to be checked on the
user's controller. The firmware has not yet been physically validated.

MiSTer input and motor test reference:
https://mister-devel.github.io/MkDocs_MiSTer/basics/input/

Live installation (15 November 2026): no-OLED 1.2.1 installed successfully with
Preserve Configuration. The prior schema-20 settings were still in flash and
were recovered, including enabled mouse/gyro and two controller records. Stock
upstream had shown empty runtime defaults because it could not load that fork
layout. The earlier backed-up controller record and all global settings match
byte-for-byte; an additional saved controller record is retained. Stock schema
14 conversion is tested separately using the pre-update export and synthetic
profiles. Right-click and separate rumble still await the user's hardware check.
