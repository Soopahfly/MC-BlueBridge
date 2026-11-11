# 1.1.2 optional Steam gyro aiming preview

Experimental original Steam Controller (model 1001) BLE firmware for Pico 2 W.
Upstream base remains 1.1.0. Published only to Soopahfly/MC-BlueBridge.

The config app now includes optional gyro-to-mouse aiming, sensitivity (1-200%),
horizontal/vertical inversion, activation (always/right-pad touch/left trigger),
a hold-to-pause button and optional right-trigger mouse left-click/fire.
Gyro and trigger-fire default off. Right-pad pointer fine adjustment is retained.
The pause button is reserved; trigger-fire replaces gamepad right-trigger input.
All pointer settings save together and take effect after restart.

Requires single-controller MiSTer mode with the master mouse checkbox enabled,
original Steam Controller BLE firmware and a core accepting mouse input.
This is relative air-mouse aiming: no automatic screen recenter, absolute
light-gun positioning, accelerometer tilt mapping or DS4 gyro support.

Schema 18 migrates old flash/settings exports, including preserving schema-17
mouse choices. Export a backup before flashing; older firmware cannot read 18.
Both OLED and no-OLED UF2s are included, along with SHA-256 checksums.

Software validation: native regression tests (including integration timing,
activation, sensitivity/inversion, pause, click release and migration), config
app simulated-serial tests, both Release builds and stack/USB descriptor audits.
Physical BLE pairing, gyro axis direction, stationary drift, sensitivity,
MiSTer/core behavior and end-to-end latency remain untested on hardware.

Use the updated tools/oled-config.html page; docs/demos/oled-config-demo.html is
an interactive simulation only. See docs/OLED.md for setup and serial commands.
