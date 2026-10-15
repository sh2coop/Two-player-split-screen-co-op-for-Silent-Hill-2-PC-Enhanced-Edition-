# Silent Hill 2 (PC) – two-player co-op mod

A plugin for the PC version of Silent Hill 2 that puts a second James into the game for a second
player: keyboard + controller or two controllers, split screen (side by side or top and bottom), one
shared story. Player 2 joins and leaves at any time; while he is not in, the game is unchanged.

It runs on top of the fan-made [Silent Hill 2: Enhanced Edition](https://enhanced.townofsilenthill.com/SH2/),
which you install first (see below).

It is a work in progress. The whole game has been played through with it, but expect rough edges.

**This repository contains no files from the game or from the Enhanced Edition** – only the mod's own
source code. You need your own copy of Silent Hill 2 (PC).

## What you need

- A copy of Silent Hill 2 for PC (the 2001/2002 PC release, also called "Restless Dreams" or
  "Director's Cut").
- The **Silent Hill 2: Enhanced Edition** installed on it: https://enhanced.townofsilenthill.com/SH2/
  (download `SH2EEsetup.exe` from that page and follow its instructions). The mod was made for the
  Enhanced Edition's exe 1.0 and its "SH2 Enhancements" module 3.0.2218.0; it checks the exe when the
  game starts and does nothing on any other exe.
- Nothing else added to the game. Other plugins (`.asi` files) and wrapper DLLs hook the same game code
  the co-op hooks: the installer refuses to install over them, and bug reports are only useful from a
  clean Enhanced Edition install. If you want other mods too, keep a separate clean copy of the game for
  co-op.
- A second input device. Controllers are read as XInput (Xbox-style) pads; the keyboard counts as a
  device too, so keyboard + one pad works.

## Install – step by step

**1. Install the game and the Enhanced Edition first.**
Install Silent Hill 2 (PC), then run `SH2EEsetup.exe` from
https://enhanced.townofsilenthill.com/SH2/ and let it install everything it offers. Start the game once
and make sure it runs. Note where the game is installed: the folder that contains `sh2pc.exe`
(for example `C:\Games\Silent Hill 2`). You need that folder in step 3.

**2. Download the mod.**
Click the green **Code** button at the top of this page, then **Download ZIP**. Unzip it anywhere – your
Downloads folder is fine. Inside the unzipped folder you will find **Install co-op.exe** next to the
mod's files (`sh2coop.asi`, `sh2coop.ini`, `sh2coop_check.exe`, `VERSION.txt`) and the source code.

**3. Run the installer.**
Double-click **Install co-op.exe**.
- If Windows shows "Windows protected your PC", click **More info**, then **Run anyway**. The
  installer is not signed; that is all the warning means.
- A window opens: **choose your Silent Hill 2 folder** – the folder from step 1 that contains
  `sh2pc.exe` – and click OK. (You can skip this window by unzipping the mod straight into the game
  folder, or by dragging the game folder onto `Install co-op.exe`.)
- If the game is under `C:\Program Files`, Windows will not let the installer write there: it says so
  and stops. Right-click `Install co-op.exe`, choose **Run as administrator**, and run it again.
- Answer **Y** when it asks "Install?". It copies the mod's files into the game folder, switches plugin
  loading on in the Enhanced Edition's `d3d8.ini`, checks everything, and ends with **READY**.

**4. Start the game.**
Start Silent Hill 2 as usual. The title screen says **CO-OP** under the logo when the mod is loaded.
Start or load a game, then player 2 presses **G** on the keyboard or **BACK + START** together on a
controller to join as James 2. The screen splits. Press **F8** (or L3 + R3) to change the split
layout.

What the installer does – and all it does: it copies the mod's own files (`sh2coop.asi`, `sh2coop.ini`,
`sh2coop_check.exe`, its README and VERSION) into the game folder and changes one line in the Enhanced
Edition's `d3d8.ini` (`LoadPlugins = 1`, with a backup kept as `d3d8.ini.bak-before-coop`). It never
touches saves or game files, and it stops before changing anything if it finds other plugins or DLLs the
Enhanced Edition does not ship.

**To update:** download the new zip, unzip it, run its `Install co-op.exe` again and choose **U**.
**To remove:** run `Install co-op.exe` again and choose **R** – it deletes the mod's own files and sets
`LoadPlugins` back as it was. (Or just rename `sh2coop.asi` to `sh2coop.asi.off`.)

The full player instructions (every control, every setting, troubleshooting) are in
`sh2coop README.txt`; the installer puts a copy into the game folder too.

## Controls

| | |
|---|---|
| Join as James 2 | `G` on the keyboard, or BACK + START together on a controller |
| Leave | hold the same keys for about a second while James 2 is in |
| Regroup (James 2 jumps to James 1) | `Z`, or BACK alone on James 2's controller |
| James 1 to James 2 | LT + RT + LB + RB together on James 1's controller |
| Split screen | `F8`, or L3 + R3 (both sticks pressed in): side by side → top and bottom → off |
| Swap the two players' devices / reset to auto | `F6` / `F7` |
| Write a status line into the log | `F10` (press it when something odd happens) |

Who controls whom: with the default `auto` setting, the device that presses **join** becomes James 2's;
James 1 keeps the rest. So with keyboard + one controller, whoever presses join on the controller is
James 2 and the keyboard player is James 1 (or the other way round). This can also be set in
`sh2coop.ini` or in Options > Game Options > Co-op Options in the game.

James 1 plays exactly as the normal game on his own device; James 2 uses the game's own control layout
(Options > Control Options) on his. Only James 1 uses the pause menu and the inventory; James 2's Map
button opens the map for him. James 2 cannot be grabbed by enemies; when his health runs out he is out
for a few seconds and comes back beside James 1.

James 2 can pick up items and read locked-door and examine messages where he stands: nobody is
teleported, the item goes into the shared inventory. A door that really opens, and a save point, bring
James 1 over to him. Scenes and scripted things (holes, ladders, puzzles, cutscene triggers) can only be
started by James 1. A few parts of the story are James 1's alone (the rowboat, the Nightmare Hotel
finale and the final boss arenas): James 2 is brought along and cannot start anything there, so he
can't change the ending.

## Settings

`sh2coop.ini` next to the mod: who controls which James (`[input]`), the split-screen layout
(`[split]`), and `[coop]` switches for troubleshooting (weapon copy, enemy targeting, James 2 using
doors/items, red jacket, own bone slots, flashlight, title screen, texture dumps). Every setting is
explained in the file itself.

## If something goes wrong

1. Make sure it is a clean Enhanced Edition install: one of the first lines of `sh2coop.log` (in the
   game folder) says either "a clean Enhanced Edition install" or "OTHER CODE in the game folder: ...".
   With the second one, move those files out and try again before reporting.
2. Double-click `sh2coop_check.exe` in the game folder: it only reads your files and writes a report,
   `sh2coop_check.txt`, that ends with READY or lists what to fix. After the first start it should say
   "N of N hooks installed" (both numbers the same).
3. Open an issue here with `sh2coop.log` (the previous run is kept as `sh2coop - <date> <time>.log`)
   and `sh2coop_check.txt`, and say what you were doing.

## Building from source

Needs the 32-bit MinGW-w64 toolchain (Debian/Ubuntu: `apt install gcc-mingw-w64-i686`) and `zip`.

```
./build.sh
```

builds `out/sh2coop.asi` (the mod), `out/Install co-op.exe` and `out/sh2coop_check.exe` (installer and
checker, both from `sh2coop_setup.c`) and the player zip `out/SH2 co-op mod <build>.zip`.
The build needs nothing from the game. The built `sh2coop.asi`, `Install co-op.exe`, `sh2coop_check.exe`
and `VERSION.txt` are committed here too, so that "Download ZIP" gives players a ready-to-run folder;
`VERSION.txt` lists the build name and the sha1 of each file.

- `sh2coop.c` – the mod: James 2 (a second player work driven by the game's own James code on a
  swapped copy of the player block), input routing for two devices, enemy targeting, grab/hold rules,
  co-op interaction rules, the bone-pool handling, the log.
- `sh2coop_d9.c` – the split-screen render path (Direct3D 9 side, through the Enhanced Edition's
  d3d8 → d3d9 layer).
- `sh2coop_title.c` – the co-op title screen. The CO-OP lettering and the recoloured background are
  made at run time from the player's own Enhanced Edition title files; no artwork is shipped.
- `sh2coop_setup.c` – `Install co-op.exe` (default) and `sh2coop_check.exe` (`-DCHECK_ONLY`).

## Legal

This is a fan-made, non-commercial modification. It is not affiliated with, endorsed by or supported by
Konami or by the Silent Hill 2: Enhanced Edition project. Silent Hill is a trademark of Konami Digital
Entertainment; the name is used here only to say which game the mod is for. No game files, Enhanced
Edition files or artwork are included in this repository or in the release packages; you must own the
game to use the mod.

The mod's own code is released under the MIT License (see `LICENSE`).
