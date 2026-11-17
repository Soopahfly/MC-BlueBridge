# DS4 right-click and two-finger scrolling

Implemented and software-tested; physical DS4 and MiSTer testing is pending.
This contribution starts from upstream 1.2.1, commit 323b057, and retains the
merged gyro aiming/pause calibration, lightbar, rumble and gamepad behavior.
No display, PEW, rotary encoder or trackball features are included.

## Settings

Serve this checkout on localhost (`python -m http.server 8000`) and open
`http://localhost:8000/tools/ds4-mouse-config.html` in desktop Chrome or Edge.
Connect the adapter by USB, close other serial clients, and click Connect.
Use single-controller MiSTer output mode with USB mouse output enabled.
The standard Companion app needs to adopt the new capability/commands before
these controls appear there; the included page configures them directly now.

Right-click choices:

- Disabled: preserve the existing physical-pad left-click behavior.
- Two-finger press: press the pad while both contacts are active for right-click;
  a one-finger press remains left-click.
- Right-half press: primary contact on the right half selects right-click;
  the left half selects left-click.
- Controller button: choose any canonical DS4 button (0-13); that input is reserved
  from normal gamepad output while mouse mode is active. Choosing L2/R2 also
  reserves its analog output. Physical-pad press remains left-click unless the
  pad press itself is the chosen right-click button.

The chosen physical-click type is latched until release, preventing a held drag
from changing buttons when fingers move, lift or cross the centre. Both left and
right buttons can be held together when using a dedicated right-click button or
gyro trigger fire. Avoid assigning the same controller button to right-click and
gyro pause unless that combined behavior is intended.

Enable two-finger vertical scrolling to send standard HID wheel notches. Single
finger movement still controls the pointer. Two contacts stop pointer/gyro motion
and move the wheel; lifting/replacing contacts rebases the gesture so no catch-up
or pointer jump occurs. Contact array order changes are handled by contact IDs.
Scroll step is raw touchpad distance per notch (16-255): smaller is faster;
32 fast, 64 normal, 128 slow. Reverse direction is optional. Partial notches are
retained between fresh reports. Horizontal scrolling and tap-to-click are not
part of this change.

Save applies the entire mouse/gesture selection together and restarts when needed.
Existing gyro/lightbar choices are preserved. Old firmware hides/rejects these
controls through HELLO capability detection rather than receiving unknown commands.

Both right-click and scrolling default off, including after upgrading. Schema 15
appends five gesture bytes to current upstream schema 14 (11,044-byte payload).
Upstream schema-14 settings retain mouse enable, gyro settings and lightbar colors;
older supported upstream flash/export formats continue to migrate. Older firmware
cannot read schema 15. The personal display fork and earlier experimental controller
branch used independent layouts: their exports are not interchangeable. Back up
settings before flashing and retain the old firmware/export for rollback.

## Protocol

HELLO adds `ds4_mouse_gestures: true`. Existing DS4_AIM_GET/SET and TOUCH_MOUSE_GET/SET
remain compatible and preserve gesture choices. New commands:

```
BB1|1|DS4_MOUSE_GET
BB1|2|DS4_MOUSE_SET|1|5|1|64|0|1
```

SET fields: right-click mode (0 disabled/1 two-finger/2 right-half/3 button),
canonical right-click button (0-13), scroll enabled (0/1), scroll step (16-255),
reverse scrolling (0/1), master mouse enabled (0/1). GET returns these saved values
and restart_required. Strict field/count/range validation occurs before saving.

Wheel deltas use the existing mouse HID descriptor and endpoint; no extra USB
interface is added. Queueing preserves wheel-only events, button edges and unsent
movement until successful endpoint acknowledgement, bounds accumulated motion,
and discards stale wheel motion. Capture/disconnect releases all buttons, including
when a press was already in flight before the release.

## Validation and hardware checklist

- [x] Original DS4 pointer CRC/contact/history and mouse queue regressions.
- [x] Right-click classification/latching and chosen-button reservation tests.
- [x] Two-finger movement, fractions, duplicate history, contact replacement/order and stale-gap tests.
- [x] Wheel-only/backpressure/expiry and in-flight press/release queue tests.
- [x] Schema-14 migration preserves gyro/lightbar, strict validation, saved settings/restart/reset.
- [x] Actual settings-page simulated serial tests, including save errors and old-firmware fallback.
- [x] Final Pico 2 W firmware build, preview manifest and actual USB descriptor/wheel audit.
- [x] Back up source to the owner's fork on contrib/ds4-mouse-gestures (implementation ec83a5f); upstream PR remains unopened.
- [ ] Hardware: all click modes, right/left drags, crossing/lifting fingers while pressed, and simultaneous buttons.
- [ ] Hardware: scrolling direction/speed, one/two-finger transitions, scrolling with gyro enabled, contact changes.
- [ ] Hardware: capture, sleep/reconnect, radio loss and USB backpressure release all buttons correctly.
- [ ] Hardware: settings migration/save/restart plus existing gyro zeroing, rumble and lightbar remain usable.
- [ ] Hardware: MiSTer menu and actual mouse-capable cores recognize right-click/wheel; core support varies.

No physical compatibility, gesture feel or end-to-end latency result is claimed
by native tests. Keep new upstream submission on hold until hardware tests pass.
