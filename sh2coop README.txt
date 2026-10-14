SILENT HILL 2 (PC, Enhanced Edition) - TWO-PLAYER CO-OP MOD
============================================================

WHAT THIS IS
  A plugin for the Silent Hill 2 Enhanced Edition that puts a second James into the game for a
  second player: keyboard + controller or two controllers, split screen (side by side or top and
  bottom), one shared story. Player 2 joins and leaves at any time; the game itself is unchanged
  while he is not in. It is a work in progress: the whole game is playable, but expect rough edges.

  >>> PLEASE USE A CLEAN INSTALL OF THE ENHANCED EDITION. <<<
  The mod was made for, and tested on, the Enhanced Edition exactly as its setup tool installs it.
  With other mods in the game folder it may not work correctly: other plugins (.asi files) and
  wrapper DLLs hook the same game code the co-op hooks. The installer will not install over them,
  and we can only look into bug reports from a clean Enhanced Edition install. If you want other
  mods too, keep a separate clean copy of the game for co-op.

WHAT YOU NEED
  - Silent Hill 2 (PC) with the Enhanced Edition installed (SH2EEsetup.exe from the Enhanced Edition
    project), and nothing else added to it. The mod was made for the Enhanced Edition's exe 1.0 and its
    "SH2 Enhancements" module 3.0.2218.0. It checks the exe when the game starts and does nothing on
    any other exe.
  - A second input device. Controllers are read as XInput (Xbox-style) pads; the keyboard counts
    as a device too, so keyboard + one pad works.

INSTALL (one click)
  1. Unzip this zip anywhere (your Downloads folder is fine).
  2. Double-click  Install co-op.exe.
     - If Windows shows "Windows protected your PC", click "More info", then "Run anyway" (the
       installer is not signed; that is all the warning means).
     - If the installer does not find your game folder by itself, a window asks you to choose it:
       pick the folder that contains sh2pc.exe. (Or drag that folder onto Install co-op.exe.)
     - If the game is under C:\Program Files, Windows will not let the installer write there: it says
       so and stops. Right-click Install co-op.exe, choose "Run as administrator", and run it again.
  3. It ends with READY. Start the game as usual: the title screen says CO-OP under the logo.

  What the installer does - and all it does:
     - copies the mod's own files into the game folder: sh2coop.asi, sh2coop.ini, sh2coop_check.exe,
       "sh2coop README.txt" and "sh2coop VERSION.txt" (an existing sh2coop.ini is kept)
     - changes ONE line in the Enhanced Edition's d3d8.ini:  LoadPlugins = 1  (the Enhanced Edition
       ships with plugins switched off); a backup is kept as d3d8.ini.bak-before-coop
     - runs the same check sh2coop_check.exe makes and shows the result
  It never touches your saves or your game files. If it finds other plugins (.asi files) or DLLs the
  Enhanced Edition does not ship, it stops before changing anything and lists them: move those out of
  the game folder (or rename each .asi to .asi.off), start the game once to make sure it still runs,
  then run the installer again. Leftovers of texture / model / sound mods are pointed out but do not
  stop the install - they cannot be told apart from the Enhanced Edition's own files reliably - the
  rule above still applies to them.

  To UPDATE: unzip the new version and run its Install co-op.exe again (choose U).
  To REMOVE:  run Install co-op.exe again and choose R - it deletes the mod's own files and sets
              LoadPlugins back as it was. (Or just rename sh2coop.asi to sh2coop.asi.off.)

THE ENHANCED EDITION'S OWN TOOLS
  SH2EEconfig.exe (the Configuration Tool) works as usual with the mod installed: its Advanced tab shows
  the mod's setting as "Enable loading ASI plugins", and saving keeps it. Only its "Default" button (reset
  all settings) switches it off again - if you ever use Default, tick "Enable loading ASI plugins" again
  before saving, or run Install co-op.exe once more. SH2EEsetup.exe (updating the Enhanced Edition's
  packages) leaves the mod's files and the setting alone.

INSTALL BY HAND (if you prefer)
  Copy sh2coop.asi, sh2coop.ini and sh2coop_check.exe next to sh2pc.exe. Open d3d8.ini with Notepad,
  find the [Plugins] section near the end and change  LoadPlugins = 0  to  LoadPlugins = 1.
  Then double-click sh2coop_check.exe: it reads your folder and ends with READY, or lists what to fix.

CHECKING AN INSTALL
  sh2coop_check.exe (in the game folder after installing) only reads your files - the one file it
  writes is its report, sh2coop_check.txt. It checks the exe, the Enhanced Edition module, the ini
  setting, the mod's files, other plugins and DLLs, and what the mod's log said the last time the
  game ran. After the first start, that log should say "N of N hooks installed" (both numbers the same).

CONTROLS
  James 1 plays exactly as the normal game on his own device. James 2 uses the game's own control
  layout (Options > Control Options) on his device.
  Join as James 2 ....... G on the keyboard, or BACK + START together on a controller. With the default
                          "auto" setting, the device that presses join becomes James 2's; James 1 keeps
                          the rest.
  Leave ................. hold the same keys for about a second while James 2 is in.
  Regroup ............... Z, or BACK alone on James 2's controller: James 2 jumps to James 1.
  James 1 to James 2 .... LT + RT + LB + RB together on James 1's controller.
  Split screen .......... F8, or L3 + R3 (both sticks pressed in) on either controller: side by side ->
                          top and bottom -> off (one shared screen). Also in Options > Game Options >
                          Co-op Options, together with who has which device.
  Swap devices .......... F6 swaps the two players' devices; F7 resets both to "auto".
  Map ................... James 2's Map button opens the map for him. Only James 1 uses the pause menu
                          and the inventory. James 2 cannot be grabbed by enemies; when his health runs
                          out he is out for a few seconds and comes back beside James 1.
  F10 ................... writes a STATUS line into sh2coop.log - press it when something odd happens,
                          it helps us find the cause.

SETTINGS (sh2coop.ini, next to the mod)
  [input] james1 / james2 = auto | keyboard | pad1 | pad2 | keyboard+pad1 | keyboard+pad2
  [split] layout = side | top | off
  [coop]  switches (1 = on, 0 = off) for troubleshooting: weapon (James 2 carries James 1's weapon),
          enemies_target_nearer, james2_interact (James 2 can use doors/items/save points), james2_red_jacket,
          james2_own_bones, james2_flashlight, title_screen. If something misbehaves, set one to 0 and tell
          us which one fixed it.

IF SOMETHING GOES WRONG
  First make sure it is a clean Enhanced Edition install: one of the first lines of sh2coop.log says either
  "game folder: no other plugins or wrapper DLLs - a clean Enhanced Edition install" or "OTHER CODE in
  the game folder: ...". With the second one, move those files out and try again before reporting.
  Then send us sh2coop.log (from the game folder; the previous run is kept as "sh2coop - <date> <time>.log")
  and sh2coop_check.txt, and say what you were doing. The mod never changes files on your disk: the
  only things it writes are those logs and its own ini. (If we ask you to set texture_dumps = 1 in
  sh2coop.ini for a jacket-colour problem, it also writes a few sh2coop_dump_*.dds snapshots of
  James's textures into the game folder; they are for troubleshooting only and safe to delete.)

NOT INCLUDED, ON PURPOSE
  This zip contains none of the Enhanced Edition's or Konami's files. The co-op title screen is drawn
  at run time from the title files your own Enhanced Edition already has.
