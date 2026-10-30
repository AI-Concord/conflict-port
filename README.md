# Conflict: Middle East Political Simulator — native Mac port

A port of the 1990 DOS game (Virgin Mastertronic) to a native macOS app, made by
reverse engineering `Conflict.exe`. The game logic is C translated function by function
from the disassembly; the front end is Swift with a Metal renderer. No emulator and no
original code runs in the app.

## Run

    open build/Conflict.app

Keys are the original's: Up/Down/Home/End move the highlight, Return selects.
On the monthly news menu `B` (capital) shows the stability/relations chart; on the
"Good morning" newspaper menu `S` (capital) saves the game; `E` (capital) abandons the
current game. Saved games live in `~/Library/Application Support/Conflict/` and use the
DOS file format (`conf1.sav` ... `conf5.sav`), so they are interchangeable with the original.

## Build

    app/build.sh            # needs only the Xcode command line tools

## Windows

    win/build.sh            # cross-compiles with MinGW-w64 (brew install mingw-w64)

Produces `build/win/Conflict/Conflict.exe` plus its `data` folder: the same game core with
a Win32 window and a Direct3D 12 presenter (`win/main.cpp`, `win/engine_win.c`). It falls
back to GDI if Direct3D 12 cannot start (`-gdi` forces that). Built on the Mac and not yet
run on Windows.

## Layout

- `port/` — the game. `ega.c` is a small model of the EGA adapter the game draws on;
  `gfx.c` is the original's assembly graphics library translated onto it; the other files
  follow the original's modules (`ui`, `paper`, `ai`, `maps`, `briefing`, `war`, `diplomacy`,
  `fronts`, `border`, `arms`, `policy`, `strikes`, `warsim`, `news`). Game state lives in an
  image of the original data segment (`conflict.dat`, extracted from the EXE), addressed
  through the macros in `conflict.h`; original addresses are noted on every function.
- `port/platform.c`, `port/engine_native.c` — platform layer for the app;
  `port/headless.c` — script-driven build used for testing.
- `app/` — AppKit + Metal shell (`App/main.swift`), bundle build script.
- `original/` — the DOS files (fonts, maps, unit pictures are loaded from here).
- `re/` — reverse-engineering workspace: Ghidra project and scripts, a small 8086/DOS/EGA
  emulator (`re/emu/emu.c`) used as the reference, and the comparison tools.

## How it was verified

`re/emu/cmp2.sh <script>` plays the same key script through the original (in the reference
emulator) and through the port, and compares every captured frame pixel for pixel and the
game's memory byte for byte. `re/emu/gen.py` / `gen2.py` generate random sessions, with
wars and other states forced so that rarely seen code runs. Around sixty such sessions
match exactly, and saved games are byte-identical.

## Differences from the DOS game

- **Newspaper freeze fixed.** The original stores each month's headlines and country
  stories in fixed buffers (25 entries; 1000 and 2500 bytes) with no bounds checks, and the
  news counters sit directly after the headline text. A month with enough news — typically
  several wars at once — overwrote the counters and the game hung. The port drops the
  excess items instead (`add_headline` / `add_story` in `port/briefing.c`).
- **Pauses can be skipped with any key**, and `E` works during the month-end calculations.
  The original's key-polling routine swallowed ordinary keys (see `gfx_getkey`).
- Timed pauses use real time instead of calibrated busy loops.

Everything else, including the original's quirks, is kept: for example aid to an ally
costs nothing, and one AI decision depends on uninitialised memory, which the port
reproduces from what tracing showed that memory to contain (`ai_month_begins` in `port/ai.c`).

## Picture (2026-10-17)

Both apps draw the 640x350 screen in one of two looks: **Smooth Picture** (the default), an xBR-style edge-smoothing
filter with four samples per screen pixel that redraws diagonal edges and the stair-steps of the lettering as clean
lines, or **Sharp Pixels**, the earlier sharp-bilinear filter. Mac: View menu. Windows: the window menu (right-click
the title bar or Alt+Space); kept in `HKEY_CURRENT_USER\Software\Conflict`. The shader is the one written for the
Pool of Radiance port ([AI-Concord/pool-of-radiance-port](https://github.com/AI-Concord/pool-of-radiance-port), documented there), in Metal (`app/App/main.swift`) and HLSL (`win/main.cpp`).
If a PC cannot compile the smooth HLSL, the Windows app uses sharp pixels and writes `shader-error.txt` next to the
program. Package: `dist/Conflict for Windows (smooth picture).zip`.

## Game files

This repository holds only this project's own work. The original game (Conflict: Middle East Political Simulator,
1990) is not included: no game files, no data segment, no decompilation or disassembly listings of its code. To build
or test, put the data files from your own copy of the game (the original DOS release) in `original/` and extract `port/conflict.dat` from it.
`tools_export_public.sh` makes the published copy from the full working repository.

## Credits

**Conflict: Middle East Political Simulator** (1990) was designed by David J. Eastman, developed by PSI Software and
published by Virgin Mastertronic for MS-DOS, Amiga and Atari ST. The game, its design, text and artwork belong to
their creators and rights holders. All credit for the game itself goes to them.

This native port to macOS (Metal) and Windows (Direct3D 12) was made by Claude, an AI model by Anthropic, working
with Nick Behnken. The port is an unofficial fan project, not affiliated with or endorsed by the game's creators,
publisher or rights holders. The port is fully native: it needs no DOS and no emulator. It does use the original game's data files
(pictures, maps, fonts, text), which are not included here; copy them from your own copy of the game into
`original/` before building.
