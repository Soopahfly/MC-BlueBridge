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
    "steam-ble": ["tests/test_steam_ble.c", "src/steam_ble.c"],
    "steam-controller": ["tests/test_steam_controller.c", "src/steam_controller.c"],
    "touch-mouse": ["tests/test_touch_mouse.c", "src/touch_mouse.c"],
    "controller-settings": ["tests/test_controller_settings.c", "src/bluebridge_state.c", "src/config_store.c", "src/controller_input.c"],
}.items():
    exe = out / (name + (".exe" if os.name == "nt" else ""))
    extra = ["-Itests/stubs/steam"] if name == "steam-ble" else []
    subprocess.run([cc, "-std=c11", *extra, "-Wall", "-Wextra", "-Werror", "-Itests/stubs", "-Isrc",
                    *sources, "-o", str(exe)], cwd=root, env=env, check=True)
    subprocess.run([str(exe)], cwd=root, env=env, check=True)
