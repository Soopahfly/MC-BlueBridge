"""Validate actual USB configuration bytes in a built ARM ELF (no mock macros)."""
import struct
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else root / "build")
elf = build / "mc_bluebridge.elf"
data = elf.read_bytes()
assert data[:6] == b"\x7fELF\x01\x01", "Expected little-endian ELF32"
section_offset = struct.unpack_from("<I", data, 32)[0]
section_size, count = struct.unpack_from("<HH", data, 46)
sections = [struct.unpack_from("<10I", data, section_offset + i * section_size) for i in range(count)]
nm = root / ".build-tools/bin/arm-none-eabi-nm.exe"
if not nm.exists():
    nm = "arm-none-eabi-nm"
symbols = subprocess.check_output([str(nm), "-S", "--defined-only", str(elf)], text=True)
def blob(name):
    line = next(line for line in symbols.splitlines() if line.split()[-1] == name)
    address, size = (int(v, 16) for v in line.split()[:2])
    for section in sections:
        _, kind, _, start, offset, length, *_ = section
        if kind != 8 and start <= address and address + size <= start + length:
            return data[offset + address - start:offset + address - start + size]
    raise AssertionError(f"Missing data for {name}")

for name, interfaces, hid_count in [
    ("hid_config_desc", 3, 1), ("mister_config_desc", 3, 1),
    ("touch_config_desc", 4, 2), ("multi_config_desc", 4, 2),
    ("dualsense_config_desc", 3, 1), ("switch_config_desc", 3, 1),
    ("xinput_config_desc", 3, 0),
]:
    config = blob(name)
    assert config[:2] == bytes([9, 2])
    assert struct.unpack_from("<H", config, 2)[0] == len(config), name
    assert config[4] == interfaces, name
    offset = 0
    found_interfaces, endpoints = set(), set()
    hids = 0
    while offset < len(config):
        length, kind = config[offset:offset + 2]
        assert length >= 2 and offset + length <= len(config), name
        descriptor = config[offset:offset + length]
        if kind == 4:
            found_interfaces.add(descriptor[2])
            if descriptor[5] == 3:
                hids += 1
        if kind == 5:
            assert descriptor[2] not in endpoints, (name, "duplicate endpoint")
            endpoints.add(descriptor[2])
        offset += length
    assert found_interfaces == set(range(interfaces)), name
    assert hids == hid_count and hids <= 2, name
    if name == "touch_config_desc":
        assert 0x83 in endpoints and 0x84 in endpoints
    print(f"USB descriptor: {name}: {interfaces} interfaces, {hids} HID")
assert b"\x05\x01\x09\x02" in blob("touch_mouse_report_desc"), "Mouse usage missing"
print("USB descriptor audit passed; physical host enumeration still needs testing.")
