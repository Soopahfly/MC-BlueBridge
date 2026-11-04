# SSD1306 OLED and rotary encoder

This fork adds a 128x64 SSD1306 display over I2C and a rotary encoder with a
push switch. The display shows USB output mode, connection/pairing state,
the selected controller and profile, turbo configuration and battery percentage
where available. It uses a compact uppercase ASCII font.

## Default wiring

Disconnect USB while wiring. Power the OLED and any encoder module at **3.3 V**;
all signal lines must remain at 3.3 V. A bare mechanical encoder needs no VCC:
connect its common terminal and the other push-switch terminal to GND. Firmware
enables internal pull-ups on A, B and the switch.

| Device connection | Pico GPIO / supply | Physical Pico pin |
| --- | --- | --- |
| OLED SDA | GP4 (I2C0 SDA) | 6 |
| OLED SCL | GP5 (I2C0 SCL) | 7 |
| OLED VCC | 3V3 OUT | 36 |
| OLED GND | GND | 8 (or another GND) |
| Encoder A / CLK | GP6 | 9 |
| Encoder B / DT | GP7 | 10 |
| Encoder SW | GP8 | 11 |
| Encoder common / GND | GND | 8 (or another GND) |
| Encoder module VCC, if present | 3V3 OUT | 36 |

The default OLED address is **0x3c**. Some modules use **0x3d**. Most SSD1306
breakouts include I2C pull-up resistors; if yours does not, add suitable pull-ups
from SDA/SCL to 3.3 V (typically 4.7 kohm for short wiring). This implementation
targets common four-pin modules with onboard reset circuitry and charge-pump
power. A bare panel or module needing a separate reset line needs additional work.

Use a quadrature rotary encoder with a push switch. Four signal transitions per
detent is the default; two-transition encoders are configurable. Swap A and B
if rotation feels reversed. No buzzer or audio hardware is used.

## Navigation

- Press or turn from the status screen to open the menu.
- Turn to choose a setting; press to edit it.
- Turn to preview a value; press again to apply/save it.
- Hold for 0.8 seconds to cancel an edit, or return from the menu to status.
- Select Back to return to status.

Menu settings:

| Item | Action |
| --- | --- |
| USB mode | Choose one of the five existing output modes; confirmation re-enumerates USB. |
| Controller | Choose which remembered controller to configure/view. Does not disconnect the others. |
| Profile | Choose the selected controller's active profile. MiSTer identity changes re-enumerate USB as before. |
| Turbo | Enable/disable the selected profile's turbo configuration. |
| Turbo rate | Set 1-30 Hz. |
| Turbo control | Choose modifier, toggle or hold control. |
| Turbo button | Choose the modifier/control input button for that control method. |
| Turbo output | Choose an output button; press to toggle its membership in the saved turbo mask. Repeat for more buttons. |
| PEW PEW | Enable/disable the optional OLED-only animation for the selected profile. |
| Fire button | Choose which canonical input button triggers the animation. |
| Pairing | Start or stop pairing. Starting is refused if both slots are occupied. |
| Controller mode | Choose single/two controllers. Applying saves and restarts the adapter. |

Existing turbo semantics are preserved: modifier mode selects turbo outputs
using modifier+button presses at runtime. Toggle/hold modes use the saved turbo
output mask. Setting turbo output buttons does not remap the controller.

Two-controller mode remains MiSTer-only. USB mode and turbo items are unavailable
in that mode. Select Controller to view/configure either controller; the status
screen and cosmetic animation follow the selected controller. Settings can also
be edited for remembered controllers while disconnected.

## PEW PEW checkbox

The animation is **off by default**, stored per controller/profile, and has no
effect on gamepad output. It watches the chosen input button before remapping,
not a game-specific fire action. The default trigger is South (canonical input 0).
Actual press edges trigger a four-frame, 400 ms `PEW PEW!`/laser animation in the
bottom two display rows. Held buttons and generated turbo pulses do not retrigger
it. New effects start at most once every 750 ms; the menu takes priority. Button
capture/tester mode suppresses new effects.

To use the configuration checkbox on a computer:

```powershell
cd tools
python -m http.server 8765 --bind 127.0.0.1
```

Open **http://localhost:8765/oled-config.html** in desktop Chrome or Edge, click
Connect adapter and select its USB serial port. Choose a controller/profile,
tick **Enable PEW PEW animation**, choose Fire button and save. Selecting a
profile in this page edits its animation settings without activating it.
Close other software using the adapter's serial port first.

This repository does not contain the external MiSTer Companion/BlueBridge web
app. Their existing UI will not acquire the checkbox automatically. This local
page supplies it; the protocol below allows those apps to integrate it later.
The local page cannot reach the management interface while native Xbox 360
console mode is active. The encoder menu remains available in that mode.

## Build configuration

Validated with **Pico SDK 2.3.1**, its pinned BTstack, Arm GNU Toolchain
14.3.Rel1, CMake and Ninja. The original firmware requires the newer BTstack
`hids_host` API; SDK 2.2.0 does not provide it. Ensure SDK submodules `lib/btstack`,
`lib/tinyusb`, `lib/cyw43-driver` and `lib/mbedtls` are initialized. The libxsm3
dependency follows the existing CMake configuration.

OLED/encoder support is enabled by default in this fork. Override any wiring
at configure time:

```sh
cmake -S . -B build -G Ninja -DPICO_BOARD=pico2_w \
  -DBB_ENABLE_OLED=ON -DBB_OLED_I2C=0 \
  -DBB_OLED_SDA=4 -DBB_OLED_SCL=5 -DBB_OLED_ADDRESS=0x3c \
  -DBB_ENCODER_A=6 -DBB_ENCODER_B=7 -DBB_ENCODER_SW=8 \
  -DBB_ENCODER_STEPS=4
cmake --build build
```

Use `-DBB_ENABLE_OLED=OFF` for a build with no panel GPIO/I2C initialization.
The configuration/protocol fields remain supported so saved settings survive
switching between builds. GPIOs must be distinct exposed pins, with a valid
SDA/SCL pair for the selected I2C peripheral. The panel owns that peripheral.

On this Windows checkout, `tools/build.ps1` uses the project-local build tools
under `.build-tools` when present, otherwise tools on PATH and `PICO_SDK_PATH`.
It does not download or install tools. `tools/build.ps1 -WithoutOled` creates
the corresponding `build-no-oled` build. Extra options can be supplied through
`-CmakeArgs @('-DBB_OLED_ADDRESS=0x3d')`.

## Management protocol and persistence

The BB1 wire format and protocol version 2 are retained. `HELLO` advertises
`profile_oled: true` and `oled_panel` indicating build-time panel support.
`PROFILE_GET` adds JSON fields `oled_pew_enabled` and `oled_fire_button`.

```text
BB1|request-id|PROFILE_OLED|profile-index|enabled-0-or-1|input-button-0-to-25
BB1|request-id|OK|saved
```

The command acts on the selected management controller. Invalid indices,
boolean values and button numbers are rejected.

Configuration schema **14** appends a 64-byte per-controller/profile setting
table to schema 13, preserving the old profile/controller binary layout. Existing
schema 7-13 flash records migrate, and schema 8-13 exports remain importable.
Old profiles start with the animation disabled. Duplicating copies the setting;
deleting profiles/controllers shifts it with its owner, and new slots start off.
Current configuration payload size is 11,044 bytes, within the reserved 16 KiB.
Export a backup before flashing; older firmware will not understand schema 14.

## Runtime overhead and failure handling

- No periodic framebuffer transmission. Status is sampled in RAM at most every
  20 ms; unchanged text is not rasterized and unchanged pages are not transmitted.
- The SSD1306 owns the displayed image between updates. Changed 128-byte pages
  are sent at 400 kHz over hardware I2C.
- FIFO service queues at most 16 bytes per USB-loop call and never waits for
  bus progress, so a screen transfer does not block for its wire time.
- Display storage uses two 1,024-byte image buffers, a 135-byte in-flight packet
  and small text/state buffers. The full panel additionally holds a controller
  snapshot and menu state.
- Encoder IRQs only decode quadrature and accumulate movement; menu actions and
  persistence happen on core 1. Turning the knob does not save flash until a
  value is confirmed.
- A NACK or 100 ms without transfer progress disables the display for that boot,
  avoiding repeated traffic when absent or miswired. Reconnect power after fixing
  wiring. Encoder operation remains independent of display detection.
- Bluetooth callbacks never draw or perform I2C transfers.

In the validated Release builds, enabling the panel adds **6,560 bytes of code/
read-only data and 3,996 bytes of static RAM** compared with the same fork built
with `BB_ENABLE_OLED=OFF`. These figures exclude transient stack use and are not
a latency measurement.

## Validation and hardware bring-up

```sh
python tests/run.py
node tests/test_config.mjs
```

Native C tests cover encoder direction/bounce/hold/wrap, fire edges/capture,
profile/controller lifecycle, flash checksums and migration, menu confirmation,
turbo controls, animation timing, multi-controller restrictions, idle I2C
suppression, changed pages, in-flight changes, NACK and stuck-bus timeouts.
The JavaScript test exercises the actual checkbox page with simulated Web Serial,
including split replies, saved values and stale-profile protection.

Builds and simulations do **not** validate physical wiring or controller latency.
On hardware, first verify the status screen and encoder direction, then mode
changes/pairing, per-profile persistence after unplugging, and the optional effect.
Finally compare Bluetooth/USB report and dropped-report diagnostics with the panel
enabled/disabled, including turbo and two-controller operation.
