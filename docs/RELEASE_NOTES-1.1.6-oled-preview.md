# 1.1.6 DS4 low-battery light-bar warning preview

The DS4 light bar automatically blinks in its selected player/custom colour
when the reported battery is below 20% and discharging. Timing is one second
on, one second off. It returns to solid while charging, when full, or at 25%
and above. The recovery threshold avoids repeated toggling near the boundary.
Unknown/unsupported battery data does not trigger a warning.

The controller handles the blink timing. Normal rumble updates retain the
current colour and do not restart the blink phase. Reconnect sends the current
colour and warning state. No GUI update or new setting is required.

Both OLED/encoder and no-OLED builds include this feature. Schema remains 20;
existing saved configuration, mouse/gyro settings and colour themes are retained.
Flash the appropriate UF2 with Preserve Configuration. The shared version string
is 1.1.6-oled; the no-OLED build reports oled_panel:false.

Validation: the full native C test suite passes, including threshold and recovery
boundaries, charging/full/unknown suppression, RGB/rumble/blink encoding, CRC,
and avoiding blink-timer updates on unchanged rumble reports. Both Release
builds pass, including project stack-frame checks. UF2 family, manifest and
settings-flash boundaries are checked separately in validation.json.
Physical low-battery blinking still needs checking on a depleted DS4.

Report flags and 10 ms blink units follow the Linux hid-playstation driver:
https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux-stable/+/master/drivers/hid/hid-playstation.c

Live installation (14 November 2026): no-OLED adapter updated successfully from
1.1.5 to 1.1.6 with Preserve Configuration. Live firmware/schema and no-OLED
capability verified. All 11110 saved configuration bytes match the pre-update
backup exactly. DS4 needs PS-button reconnection after the firmware restart.
The pre-update battery was 55%; physical low-battery blinking remains untested.
