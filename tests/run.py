"""Run hardware-independent checks with a native C compiler (CC or gcc)."""
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
cc = os.environ.get("CC") or shutil.which("gcc")
if not cc and Path("C:/msys64/mingw64/bin/gcc.exe").exists():
    cc = "C:/msys64/mingw64/bin/gcc.exe"
if not cc:
    raise SystemExit("Set CC to a native C compiler, or put gcc on PATH.")
env = dict(os.environ)
env["PATH"] = str(Path(cc).parent) + os.pathsep + env.get("PATH", "")
out = root / "build-host"
out.mkdir(exist_ok=True)
for name, sources in {
    "touch-mouse": ["tests/test_touch_mouse.c", "src/touch_mouse.c"],
    "dualsense": ["tests/test_dualsense.c", "src/dualsense_device.c"],
    "input": ["tests/test_input.c", "src/controller_input.c", "src/bluebridge_state.c", "src/config_store.c"],
    "usb-queue": ["tests/test_usb_queue.c", "src/usb_report_queue.c"],
    "controls": ["tests/test_controls.c", "src/panel_controls.c"],
    "state": ["tests/test_state.c", "src/bluebridge_state.c", "src/config_store.c"],
    "oled": ["tests/test_oled.c"],
    "panel": ["tests/test_panel.c", "src/panel_controls.c", "src/oled_gamepad.c", "src/bluebridge_state.c", "src/config_store.c"],
    "gamepad": ["tests/test_gamepad.c", "src/oled_gamepad.c"],
    "turbo-display": ["tests/test_turbo_display.c", "src/controller_input.c", "src/bluebridge_state.c", "src/config_store.c"],
    "trackball-motion": ["tests/test_trackball_motion.c", "src/trackball_motion.c"],
    "trackball": ["tests/test_trackball.c", "src/trackball_motion.c"],
    "trackball-panel": ["tests/test_trackball_panel.c", "src/panel_controls.c", "src/oled_gamepad.c", "src/bluebridge_state.c", "src/config_store.c"],
}.items():
    exe = out / (name + (".exe" if os.name == "nt" else ""))
    subprocess.run([cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-Itests/stubs", "-Isrc",
                    *sources, "-o", str(exe)], cwd=root, env=env, check=True)
    subprocess.run([str(exe)], cwd=root, env=env, check=True)
