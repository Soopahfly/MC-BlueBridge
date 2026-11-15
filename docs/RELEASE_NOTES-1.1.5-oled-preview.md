# 1.1.5 DS4 light-bar themes preview

Adds saved light-bar colours to both OLED/encoder and no-OLED/encoder builds,
configured in the existing Companion Remote BlueBridge GUI. The default theme
is **Nintendo: P1 red, P2 green**, as requested. **Sega** uses P1 blue and P2
yellow/orange. **Custom** permits individual RGB colours. Themes change P1/P2
and retain the user's P3/P4 colours. Factory P3/P4 are green/pink and editable.

Automatic colour selection follows **BlueBridge connection slots**, not MiSTer's
independently assigned player numbers. This adapter supports two simultaneous
controllers. P3/P4 presets remain selectable as explicit overrides, including
when using multiple adapters. Each saved DS4 can follow Auto, use a fixed P1–P4
colour, or use its own custom RGB. Colours and preferences persist across restarts.

MiSTer Main updates LEDs on some native supported controllers, but the current
BlueBridge MiSTer-mode interface does not receive those player assignments.
Following MiSTer automatically would require additional host integration; this
release deliberately uses the BlueBridge-slot fallback the user approved.

Connected DS4 colour changes are applied live without a firmware restart. The
Bluetooth worker retries busy colour reports at a bounded rate. RGB and current
rumble strength are always encoded together so changing one retains the other.
Reconnect/activation sends the current saved colour. Output preserves the old
DS4 polling header, report length and CRC scheme. Other controller families are
not given DS4 light-bar commands.

Schema 20 appends the light-bar settings. Existing controller records/profiles,
OLED/physical controls, mouse choice and separate DS4/Steam gyro settings are
preserved. New/reset/migrated light-bar settings use Nintendo with automatic
slot colours. Controller overrides move with records when another controller
is forgotten; reused record slots start with Auto. Configuration writes use
controller ID checks to reject stale selections.

Apply `tools/companion_remote_mouse.py` to Companion Remote 4.2.0 and restart
Companion Remote. It upgrades either earlier mouse/gyro patch, or patches an
unmodified install. Then flash the appropriate 1.1.5 UF2 with **Preserve
Configuration**. Export a backup first: older firmware cannot read schema 20.
Use the pre-upgrade export if downgrading.

Protocol: `LIGHTBAR_GET`; `LIGHTBAR_PALETTE_SET|theme|12 RGB bytes`;
`LIGHTBAR_SET|controller_index|controller_id|player|custom_enabled|r|g|b`.
Theme is 0 Custom, 1 Sega or 2 Nintendo; player 0 means Auto and 1–4 chooses a
preset. `HELLO.capabilities.ds4_lightbar` advertises support. These settings
save and take effect without setting the adapter-restart flag.

Validation: full native C suite (theme/preset/override resolution, CRC and rumble
encoding, persistence, record deletion/reuse, schema-19 migration and strict
validation), Companion Remote upgrade/API/UI tests, unchanged USB config tests,
both Release builds and UF2 family/manifest/address checks. Physical RGB,
reconnect and simultaneous colour/rumble behaviour still require checking on
the user's controller. Prior DS4 touchpad and gyro functionality was verified
by the user before this release.

Live smoke check (14 November 2026): no-OLED firmware 1.1.5/schema 20 was
installed with Preserve Configuration. The Nintendo theme, palette save API,
controller Auto preference and rendered browser controls were verified. Comparing
exports confirmed all pre-existing configuration bytes were retained, apart
from the schema field; 53 bytes of Nintendo light-bar defaults were appended.
Physical colour after DS4 reconnection remains unverified at delivery.

DS4 report layout/flags checked against the Linux hid-playstation driver:
https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux-stable/+/master/drivers/hid/hid-playstation.c
