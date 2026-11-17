# Proposed PR: Add optional DS4 right-click and two-finger vertical scrolling

Local description draft; no upstream PR opened. Base is upstream 1.2.1 at 323b057.
Hardware validation is pending; source backup to the owner's fork is authorized.

DS4 touchpad mouse currently provides pointer movement and left-click only.
This adds selectable two-finger press, right-half press or a chosen controller
button for right-click, plus optional two-finger vertical wheel scrolling with
step/direction controls. Existing settings preserve legacy behavior after migration.

A physical click keeps its original button until released. Contact transitions
rebase motion, scrolling suppresses cursor/gyro movement, and wheel deltas survive
USB backpressure. An in-flight press/release race is covered by an explicit neutral
queue entry. The existing mouse descriptor/interface is reused.

New persistent settings/serial commands and a standalone Web Serial config page
are included. Upstream gyro calibration and lightbar features are retained. The
pointer parser and mouse queue are extracted into native-testable modules without
changing unrelated gamepad queue, turbo, output modes or physical peripherals.

Validation: native legacy pointer and new gesture/queue/settings tests, actual-page
simulated serial tests, final Release build and USB descriptor audit. See
DS4_MOUSE_GESTURES.md for exact software results and open hardware checks.
Before submission, add real controller/core results and review the new schema
number against any intervening upstream changes.
