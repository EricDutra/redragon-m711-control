# Redragon Control

**English** | [Português](README.pt-BR.md)

A lightweight, portable alternative to the official software for the **Redragon M711 (internal model 2710-H)** mouse on Windows.

A single `.exe`, no installer, no dependencies beyond Windows itself.

> Personal, unofficial project with no affiliation with Redragon. The protocol was obtained by reverse engineering the official software. Use at your own risk.

![Redragon Control](docs/screenshot.png)

## Features

- **Profiles:** edit all 5 profiles and choose which one is active on the mouse.
- **Lighting:** effect (breathing, rainbow, static, wave, reactive, etc.), color, speed and brightness.
- **DPI:** 5 stages from 200 to 11400 DPI. Each stage can be enabled or disabled, and you choose the active one.
- **Buttons:** each of the 8 buttons can be assigned:
  - a function (clicks, forward/back, DPI+/−, disabled);
  - a key or key combination (e.g. Ctrl+C);
  - a media key;
  - a macro;
  - a custom hexadecimal code.
- **Polling rate:** 125, 250, 500 or 1000 Hz.
- **Macros:**
  - 20 slots in the mouse memory, up to 66 events each;
  - record straight from the keyboard (with or without the delays between keys) or edit by hand;
  - they work without the program running.
- **Language:** English (default) or Portuguese interface, selectable in the top-right corner. The choice is remembered.

Settings are written to the mouse's internal memory, so the program does not need to keep running.

## Compatibility

| Item | Value |
|---|---|
| Mouse | Redragon M711 / 2710-H (P3327 sensor) |
| USB VID:PID | `04D9:FC30` (Holtek chip), revision `0x0710` |
| System | Windows 10/11 x64 |

The program only looks for this VID:PID. Other Redragon mice may use the same protocol but with different memory maps, and **are not supported**.

## Usage

1. Close the official Redragon software, since both may compete for the device.
2. Run `RedragonControl.exe`. To change the language, use the selector in the top-right corner.
3. Change what you want and click the tab's "Apply" button.

The language choice is stored in the Windows registry under `HKCU\Software\RedragonControl`. No other files are created.

Diagnostic tool (read-only):

```
redragon-cli state
redragon-cli raw <address-hex> <length>
redragon-cli macro <index>
```

## Building

Requirement: Visual Studio 2022 or newer, with the **Desktop development with C++** workload.

Dear ImGui is a git submodule, so clone with `--recursive`:

```
git clone --recursive https://github.com/EricDutra/redragon-m711-control.git
cd redragon-m711-control
build.bat
```

If you already cloned without `--recursive`, run `git submodule update --init` before building.

The script finds Visual Studio on its own and produces, in `build\`:

- `RedragonControl.exe`, the graphical interface;
- `redragon-cli.exe`, the diagnostic tool.

The build uses C++20 and the static runtime (`/MT`), so the executables do not depend on the Visual C++ Redistributable.

## Structure

```
src/
  hid_device.*       HID access through SetupAPI/hid.dll (no hidapi)
  mouse_protocol.*   mouse protocol: memory, LED, DPI, buttons, macros
  keys.*             HID / virtual-key / media key tables
  i18n.*             English and Portuguese strings
  app_ui.*           interface (Dear ImGui)
  app_main.cpp       Win32 window + Direct3D 11
  cli_main.cpp       command-line tool
third_party/imgui/   Dear ImGui 1.91.9 (MIT), git submodule
```

## Protocol

Communication uses **HID feature reports** on the configuration interface (MI_02, Usage Page `0xFFA0`).

### Reports

| ID | Size | Use |
|---|---|---|
| `0x02` | 16 bytes | short commands, writes of up to 8 bytes |
| `0x03` | 64 bytes | reads and writes of up to 56 bytes |
| `0x04` | 1024 bytes | macro writes |

Header: `[ID, cmd, addrLo, addrHi, len, 0, 0, 0]`, with data starting at byte 8.

| Command | Meaning |
|---|---|
| `F3` | write memory |
| `F2` | read memory (SetFeature with the request, then GetFeature) |
| `F1` | apply: `[02, F1, 02, flag]` |

Apply flags: `01` profile, `02` DPI, `04` LED, `08` polling, `10` active DPI stage.

### Memory map

| Address | Contents |
|---|---|
| `0x2C` | active profile `[p, 0]` |
| `0x32 + 2p` | polling rate: `1`=1000, `2`=500, `4`=250, `8`=125 Hz |
| `0x446` | LED prefix `[0, 0]` |
| `0x449 + 8p` | LED: `[R, G, B, effect, 9−speed, variant, brightness 1–3]` |
| `0x40`, `0x100`, `0x1B0`, `0x260`, `0x310` | base of profiles 1–5 |
| `base + 2` | active DPI stage |
| `base + 4 + 6i` | DPI stage `[enabled, codeX, flagX, codeY, flagY]` |
| `base + 0x42 + 4b` | button `[code, a1, a2, a3]` |
| `0x478 + 200i` | macro `i` (written with report 4) |

DPI: `dpi = (index + 1) × 100`, with index from 1 to 113. The code sent comes from a sensor table. The flag is `0` below index 62, `1` below 93 and `4` from there on.

### Button codes

| Code | Action |
|---|---|
| `81` / `82` / `83` | left / right / middle click |
| `84` / `85` | back / forward |
| `8A` / `89` | DPI+ / DPI− |
| `00` | disabled |
| `8F mods hid` | key or combination |
| `8E 01 lo hi` | media key |
| `91 idx\|mode n` | macro (`n` repetitions, `0x80` while held, `0x40` toggle) |

### Macro events

Each event is 3 bytes:

| Code | Event |
|---|---|
| `84` / `04` | key down / up (HID code) |
| `81` / `01` | mouse button down / up (mask) |
| `06` | delay, in 10 ms units |

## Status

- **Reading:** verified on the real mouse.
- **Writing:** implemented by reproducing the official software's sequences, still being validated.
- **Still inferred, not tested:**
  - the button codes marked with `*` in the interface;
  - the exact semantics of the macro repeat modes;
  - the effect speed range.

## Credits

- [Dear ImGui](https://github.com/ocornut/imgui) by Omar Cornut (MIT license).
