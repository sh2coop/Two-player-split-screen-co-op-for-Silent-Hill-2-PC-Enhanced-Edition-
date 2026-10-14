# Silent Hill 2 (PC) – two-player co-op mod

A plugin for the PC version of Silent Hill 2 (with the fan-made
[Enhanced Edition]([https://enhanced.townofsilenthill.com/](https://enhanced.townofsilenthill.com/SH2/)) installed) that puts a second James into the
game for a second player: keyboard + controller or two controllers, split screen (side by side or top
and bottom), one shared story. Player 2 joins and leaves at any time; while he is not in, the game is
unchanged.

It is a work in progress. The whole game has been played through with it, but expect rough edges.

**This repository contains no files from the game or from the Enhanced Edition** – only the mod's own
source code. You need your own copy of Silent Hill 2 (PC) with the Enhanced Edition installed.

## What you need

- Silent Hill 2 (PC) with the Enhanced Edition installed (`SH2EEsetup.exe` from the Enhanced Edition
  project), and nothing else added to it. The mod was made for the Enhanced Edition's exe 1.0 and its
  "SH2 Enhancements" module 3.0.2218.0; it checks the exe when the game starts and does nothing on any
  other exe.
- A second input device. Controllers are read as XInput (Xbox-style) pads; the keyboard counts as a
  device too, so keyboard + one pad works.

Please use a clean install of the Enhanced Edition. Other plugins (`.asi`) and wrapper DLLs hook the
same game code the co-op hooks; the installer refuses to install over them, and bug reports are only
useful from a clean install.

## Install (players)

Download the zip from the Releases page, unzip it, double-click **Install co-op.exe**, start the game.
The title screen says CO-OP under the logo when the mod is loaded. Everything else (controls, settings,
updating, removing, troubleshooting) is in `sh2coop README.txt`, which is also inside the zip.

What the installer does, and all it does: it copies the mod's files (`sh2coop.asi`, `sh2coop.ini`,
`sh2coop_check.exe`, its README and VERSION) into the game folder and changes one line in the Enhanced
Edition's `d3d8.ini` (`LoadPlugins = 1`, with a backup kept as `d3d8.ini.bak-before-coop`). It never
touches saves or game files, and it stops before changing anything if it finds other plugins or DLLs the
Enhanced Edition does not ship.

## Controls (short version)

| | |
|---|---|
| Join as James 2 | `G` on the keyboard, or BACK + START together on a controller |
| Leave | hold the same keys for about a second |
| Regroup (James 2 to James 1) | `Z`, or BACK alone on James 2's controller |
| James 1 to James 2 | LT + RT + LB + RB together on James 1's controller |
| Split screen | `F8`, or L3 + R3: side by side → top and bottom → off |
| Swap devices / reset | `F6` / `F7` |
| Status line into the log | `F10` |

James 1 plays exactly as the normal game on his own device; James 2 uses the game's own control layout
on his. Only James 1 uses the pause menu and the inventory. James 2 cannot be grabbed by enemies; when
his health runs out he is out for a few seconds and comes back beside James 1. A few parts of the story
are James 1's alone (the rowboat, the Nightmare Hotel finale and the final boss arenas): James 2 is
brought along and cannot start anything there, so he can't change the ending.

## Building from source

Needs the 32-bit MinGW-w64 toolchain (Debian/Ubuntu: `apt install gcc-mingw-w64-i686`) and `zip`.

```
./build.sh
```

builds `out/sh2coop.asi` (the mod), `out/Install co-op.exe` and `out/sh2coop_check.exe` (installer and
checker, both from `sh2coop_setup.c`) and the player zip `out/SH2 co-op mod <build>.zip`.
The build needs nothing from the game.

- `sh2coop.c` – the mod: James 2 (a second player work driven by the game's own James code on a
  swapped copy of the player block), input routing for two devices, enemy targeting, grab/hold rules,
  co-op interaction rules, the bone-pool handling, the log.
- `sh2coop_d9.c` – the split-screen render path (Direct3D 9 side, through the Enhanced Edition's
  d3d8 → d3d9 layer).
- `sh2coop_title.c` – the co-op title screen. The CO-OP lettering and the recoloured background are
  made at run time from the player's own Enhanced Edition title files; no artwork is shipped.
- `sh2coop_setup.c` – `Install co-op.exe` (default) and `sh2coop_check.exe` (`-DCHECK_ONLY`).

## Settings

`sh2coop.ini` next to the mod: who controls which James (`[input]`), the split-screen layout
(`[split]`), and `[coop]` switches for troubleshooting (weapon copy, enemy targeting, James 2 using
doors/items, red jacket, own bone slots, flashlight, title screen, texture dumps). All documented in the
file.

## Reporting problems

Send `sh2coop.log` (from the game folder; the previous run is kept as `sh2coop - <date> <time>.log`)
and `sh2coop_check.txt`, and say what you were doing. One of the first lines of the log says whether the
game folder is a clean Enhanced Edition install; reports from installs with other plugins can't be
looked into.

## Legal

This is a fan-made, non-commercial modification. It is not affiliated with, endorsed by or supported by
Konami or by the Silent Hill 2: Enhanced Edition project. Silent Hill is a trademark of Konami Digital
Entertainment; the name is used here only to say which game the mod is for. No game files, Enhanced
Edition files or artwork are included in this repository or in the release packages; you must own the
game to use the mod.

The mod's own code is released under the MIT License (see `LICENSE`).
