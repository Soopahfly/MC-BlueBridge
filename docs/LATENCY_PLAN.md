# Bluetooth controller latency and reliability plan

Controllers connect over Bluetooth; USB is the output to MiSTer. Software checks do not imply physical testing.

- [x] 1. Remove oversized command/input stack allocations, measure stack usage and reserve appropriate core stacks.
- [x] 2. Replace whole-controller input copies with compact snapshots and reduce repeated identity work.
- [x] 3. Reduce OLED copying/rendering while keeping USB service bounded and prioritized.
- [x] 4. Avoid unchanged flash writes, coalesce saves and retain failed saves for controlled retry.
- [ ] 5. Add USB queue age/depth diagnostics and backpressure tests, preserving button edges.
- [ ] 6. Schedule turbo independently of Bluetooth reports, preserving immediate releases and profile behavior.
- [ ] 7. Run regression tests and both firmware builds; publish updated documentation and firmware.
- [ ] Hardware: pairing, configuration import/export, simultaneous controllers, OLED/controls and turbo.
- [ ] Hardware: measure Bluetooth-to-USB latency and queue behavior before claiming improvements.

Implementation commits update this checklist with validation evidence. Hardware checks await flashing by the owner.

## Stack and input snapshot validation

Release ARM compiler reports: command handler 480 bytes (previously about 15,496), input submission 472 bytes (previously about 1,552), import 112 bytes (previously over 11 KB). CDC scratch storage and import/reset staging are static and owned by serialized core-1 management operations. Both linker-reserved stacks are 4 KiB, within the RP2350 scratch banks. A 1 KiB frame limit guards the affected project files; this is a per-frame guard, not proof of all nested/interrupt stack use. Native regression tests and the OLED firmware build pass. Physical stack high-water and latency checks remain pending.

The input snapshot copies only the active profile and identifying fields under one lock; controller kind is cached until identifying fields change. Bluetooth reports still traverse the input pipeline even when identical, preserving capture and turbo bookkeeping.

## Background and save validation

Normal OLED snapshots copy controller metadata plus the active profile; profile browsing still obtains the full store. Quantized picture/label comparison skips unchanged rendering and invalidates on menus/PEW/status transitions. With display off and input set to None, panel polling only drains the display-off command. USB gets another service opportunity after panel work.

Saves coalesce for 250 ms (maximum batch window 1 s); identical stored images skip erase/program. Failed saves remain pending and retry no faster than once per second. Restart waits for a successful save. `SAVE_STATUS` exposes pending state and failure count. Flash operations still pause execution; batching does not eliminate that hardware behavior.

Native tests cover idle rendering, visible input changes, unchanged flash images, save failure, retry pacing and persistence. All native tests and the OLED firmware build pass.
