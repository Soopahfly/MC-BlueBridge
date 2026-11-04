# Bluetooth controller latency and reliability plan

Controllers connect over Bluetooth; USB is the output to MiSTer. Software checks do not imply physical testing.

- [ ] 1. Remove oversized command/input stack allocations, measure stack usage and reserve appropriate core stacks.
- [ ] 2. Replace whole-controller input copies with compact snapshots and reduce repeated identity work.
- [ ] 3. Reduce OLED copying/rendering while keeping USB service bounded and prioritized.
- [ ] 4. Avoid unchanged flash writes, coalesce saves and retain failed saves for controlled retry.
- [ ] 5. Add USB queue age/depth diagnostics and backpressure tests, preserving button edges.
- [ ] 6. Schedule turbo independently of Bluetooth reports, preserving immediate releases and profile behavior.
- [ ] 7. Run regression tests and both firmware builds; publish updated documentation and firmware.
- [ ] Hardware: pairing, configuration import/export, simultaneous controllers, OLED/controls and turbo.
- [ ] Hardware: measure Bluetooth-to-USB latency and queue behavior before claiming improvements.

Implementation commits update this checklist with validation evidence. Hardware checks await flashing by the owner.
