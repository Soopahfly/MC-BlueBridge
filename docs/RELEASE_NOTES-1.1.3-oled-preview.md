# 1.1.3 DS4 gyro mouse preview

Adds optional DualShock 4 gyro-to-mouse movement to both the OLED/encoder and
no-OLED/encoder firmware builds. The upstream base remains 1.1.0. Steam
Controller support remains separate and its saved gyro settings are preserved.

Configure through the existing MiSTer Companion Remote 4.2.0 BlueBridge page
after applying `tools/companion_remote_mouse.py`. This upgrades the earlier
touchpad-only patch or installs both sets of controls onto an unmodified page.
The optional USB configuration page still exposes the existing Steam settings;
DS4 gyro configuration is in the integrated MiSTer page.

DS4 gyro defaults off. Its independent settings are sensitivity (1–200%),
activation (touchpad contact, L2 held, or always), horizontal/vertical inversion,
pause button, and optional R2 mouse left-click. The master touchpad/mouse output
must also be on, with single-controller MiSTer output selected. Settings apply
after adapter restart.

Initial activation choice is touchpad contact and the pause button is L3.
Touchpad relative movement and click/drag remain available alongside gyro.
The pause button is reserved only while gyro is enabled. Optional R2 fire
replaces gamepad R2 and does not fire while paused. Other controls remain intact.

To reduce resting drift, hold the configured pause button while keeping the
controller still for at least half a second. The firmware averages at least
32 fresh samples. Movement or a radio gap restarts collection. This is a manual
resting-bias estimate, not factory sensor calibration; no automatic bias
adaptation occurs while aiming. Start with 100% sensitivity and use inversion
if an axis moves the wrong way on your hardware.

The gyro path only receives full CRC-validated 78-byte DS4 Bluetooth reports.
It rejects duplicate/old sensor timestamps, rebases on activation and radio
gaps, and retains fractional movement. Capture, mouse-off and controller-reset
paths clear gyro state. This is a relative air mouse with a visible cursor;
it does not provide absolute light-gun position or automatic screen centring.

Schema 19 appends DS4 settings and preserves schema-18 profiles, pairing data,
panel settings, mouse choice and Steam aiming. Older schema imports default DS4
gyro off. **Export configuration before flashing.** Firmware 1.1.2 cannot read
schema 19; restore the pre-upgrade export if reverting to it.

Validation: complete native C regression suite, USB configuration simulated-
serial tests, Companion Remote patch/upgrade and JavaScript tests, both Release
UF2 builds, and UF2 manifest/address checks. Physical DS4 axis direction, resting
drift, sensitivity and MiSTer/core behaviour require hardware testing. Steam
Controller and OLED/encoder physical tests have not been performed in this run.

Live smoke check: the no-OLED build was flashed through Companion Remote with
Preserve Configuration on 14 November 2026. Firmware 1.1.3/schema 19, USB ready,
mouse output active and DS4 settings/defaults were verified through the API and
existing browser GUI. Comparing the before/after exports confirmed every old
configuration byte was preserved apart from the schema field; the five DS4
default bytes were appended. The controller needed reconnection after flashing.

Protocol additions: `DS4_AIM_GET` and `DS4_AIM_SET|enabled|sensitivity|activation|
flags|pause_button|master_mouse` use the same field encoding as Steam aiming but
save independent values. `HELLO.capabilities.ds4_gyro` advertises support.

DS4 motion layout and nominal scale were checked against the Linux
`hid-playstation` driver; implementation here is original:
https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux-stable/+/master/drivers/hid/hid-playstation.c
