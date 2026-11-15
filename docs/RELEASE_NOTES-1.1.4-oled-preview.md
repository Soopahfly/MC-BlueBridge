# 1.1.4 DS4 horizontal inversion default

DS4 gyro mouse now defaults to horizontal inversion enabled. This follows the
user's successful physical test of gyro mouse in 1.1.3 with inversion selected.
Vertical inversion remains off, sensitivity is 100%, activation is touchpad
contact, and L3 is the pause/zero button. Gyro itself remains off by default.

Applies to both OLED/encoder and no-OLED/encoder builds. The configuration schema
remains 19. Existing saved DS4 choices are preserved, including users who have
explicitly disabled horizontal inversion. The new default applies to fresh/reset
configurations and imports/migration from before DS4 gyro settings existed.
Steam Controller gyro defaults and touchpad movement are unchanged.

The user's running no-OLED adapter already has horizontal inversion saved and
active, so it does not need to be restarted or reflashed for the requested
behaviour. Both 1.1.4 Release images are built and packaged for future installs.

Validation: native gyro and configuration/migration tests, including preservation
of a schema-19 non-inverted choice; both firmware builds and UF2 checksums,
version manifest, family and settings-flash exclusion checks. No new physical
flash is performed for this default-only change.
