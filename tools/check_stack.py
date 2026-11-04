"""Check ARM project frame reports and linker-reserved stacks after a Release build.

This is a static regression check, not a runtime high-water measurement or a
complete bound on SDK/libc/interrupt nesting.
"""
import re
import sys
from pathlib import Path

build = Path(sys.argv[1] if len(sys.argv) > 1 else "build")
reports = build / "CMakeFiles/mc_bluebridge.dir/src"
frames = []
for report in reports.glob("*.su"):
    for line in report.read_text().splitlines():
        location, size, kind = line.split("\t")
        frames.append((int(size), location.rsplit(":", 1)[-1], kind))
if len(frames) < 50:
    raise SystemExit("Missing project ARM stack-usage reports; rebuild with the current CMake configuration.")
for size, name, kind in frames:
    if size > 1100 or "dynamic" in kind:
        raise SystemExit(f"Unexpected stack frame: {name}: {size} bytes ({kind})")
mapping = (build / "mc_bluebridge.elf.map").read_text()
for section in (".stack_dummy", ".stack1_dummy"):
    match = re.search(r"^" + re.escape(section) + r"\s+0x[0-9a-f]+\s+(0x[0-9a-f]+)", mapping, re.M)
    if not match or int(match[1], 16) != 4096:
        raise SystemExit(f"{section}: expected a dedicated 4096-byte stack")
print(f"Stack audit: {len(frames)} project frames; two 4096-byte linker-reserved stacks")
for size, name, _ in sorted(frames, reverse=True)[:8]:
    print(f"  {name}: {size} bytes")
print("SDK/libc nesting and runtime high-water remain hardware validation items.")
