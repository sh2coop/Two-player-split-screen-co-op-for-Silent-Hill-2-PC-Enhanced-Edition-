/*
 * sh2coop.asi  --  Silent Hill 2 PC (Enhanced Edition exe v1.0) two-player co-op.   PC build P33v
 * Built from sh2coop.c + sh2coop_d9.c (the Direct3D 9 part of the split screen) + sh2coop_title.c (the title screen):
 *   i686-w64-mingw32-gcc -O2 -Wall -shared -static-libgcc -o sh2coop.asi sh2coop.c sh2coop_d9.c sh2coop_title.c -ldinput8 -ldxguid
 *
 * P33: SPLIT SCREEN (the Xbox builds 09-16 design, re-traced on the PC exe): while James 2 is in, each James gets his
 *   own half of the screen with his own room camera - side by side (default) or top and bottom.
 *   F8 or L3+R3 (both sticks pressed) on either controller switches the layout (side -> top -> off);
 *   also Options > Game Options > Co-op Options > Split screen, and sh2coop.ini [split] layout.
 *   One full screen (James 1's camera) during cutscenes, scripted cameras and scenes, and with the layout set to off.
 * P33b (review fixes, nothing new for the player): James 2's camera copy takes the per-frame near / far planes and
 *   screen size from James 1's every frame; the "hide the newly equipped weapon for one frame" byte 0xAA9CF4 is left
 *   exactly as one view would leave it (P33 could keep James 1's weapon hidden); scripted flashlight fog survives;
 *   James 2's item pick-ups test his own half; a James 2 teleport mid-frame shows James 1's camera in his half for
 *   that frame; the depth bias is saved through d3d9 (the EE scales ZBIAS).
 * P33c: the diagonal layout is removed (asked by the player), and with it its second render target, the composite and
 *   both Direct3D Reset hooks it needed; an ini still saying "diagonal" is read as side by side and rewritten.
 * P33d: James 2 is "downed" as soon as his HP is gone (on the PC the hits are applied inside his own think, so the old
 *   "pending damage still set" test never fired and he lay dead at 0 HP until the next door or scene); the previous
 *   sh2coop.log is kept as "sh2coop - <date> <time>.log"; the log also records area changes, the bosses (appear,
 *   gone, HP 0, and every 5 s their HP, target and distance to each James), both Jameses' HP drops while a boss is
 *   around, James 1's dying flag and the game-over screen, and the boat flag; the F10 status line has the area and HP.
 * P33e: the title screen of the co-op version (sh2coop_title.c): "CO-OP" under "ENHANCED EDITION" in the same
 *   hanging-letter style (built at run time from the Enhanced Edition's own title letters), the background
 *   recoloured from blue to a faded kelly green, and all title text dark red.  sh2coop.ini [coop] title_screen = 0
 *   gives the original title screen back.
 * P33f: the title's menu words (the buttons) are white / grey again, as the game draws them; the logo, ENHANCED
 *   EDITION, CO-OP and the copyright line stay dark red.
 * P33g: Pyramid Head's choke can no longer lock the game. Pressing the inventory key a moment before James 1 died in
 *   the grab let the game's delayed inventory opening (0x48E915) through without its dying check; closing the
 *   inventory cleared the dying flag but not the grab, so James sat at 0 HP in Pyramid Head's hands for good. The
 *   delayed opening is now dropped while James is dying (same as the immediate one), and a watchdog finishes the
 *   death (Game Over -> Continue) if a grab ever gets stuck that way again. The log records each grab phase.
 * P33h: James 2 can't join, come back after being downed, be regrouped, or pull James 1 over while James 1 is held
 *   in a grab or dying (the choke doesn't set the "grabbed" flag the mod was checking, so all four were allowed; a
 *   James 2 made then started inside James 1's grab, and regroup put him in Pyramid Head's hands).
 * P33i: a grab aimed at James 1 that catches James 2 instead (he was in the way) no longer leaves James 2 struggling
 *   against nobody: the grabber lets go at once (its hold follows James 1's struggle), so James 2 is let go too.
 *   The log records James 2's grab steps.
 * P33j: James 2 can't be grabbed at all (asked by the player): every grab (Pyramid Head, Flesh Lip, Abstract Daddy,
 *   Mandarin) passes through his body and catches James 1 behind him, or misses. P33i stays as a backstop.
 * P33k: James 2 can never carry James 1 past an automatic story scene. The room rule brings James 2 back instead of
 *   pulling James 1 into the room James 2 entered first; the pad-1 "James 1 to James 2" combo and the pull for a
 *   door / item / save point James 2 uses are refused (James 2 comes to James 1 / James 1 walks there) when a
 *   walk-in scene trigger lies on the straight way, checked with the game's own trigger search. The log is quiet
 *   about James 2's skeleton while James 1 dies.
 * P33l: the log's first lines say whether the game folder is a clean Enhanced Edition install: any other .asi plugin
 *   (game folder, scripts\, plugins\) and any DLL the Enhanced Edition does not ship are listed as "OTHER CODE in the
 *   game folder", so a bug report shows at once when another mod is in the mix. The mod is made for, and only
 *   supported on, a clean Enhanced Edition install. Nothing else changed.
 * P33m: loading a save from the pause menu (Load Game) while James 2 is in no longer crashes. The load wipes every
 *   character (0x538050) and re-creates James 1, here in the work James 2 had used; the mod still held that work as
 *   "James 2" until its next update, and the split screen drew his camera from James 1's half-built work (crash at
 *   0x533DA5 in "split: cameras", user's log 13 Oct). Now the pool reset that goes with every wipe (0x53AAF0, already
 *   hooked) also forgets James 2 and his weapon copies (he rejoins when James 1 can move), and the split screen only
 *   uses James 2's work when it is still really his (think = the mod's, same check as the update).
 * P33n: the holds that follow a grab pass through James 2 too. P33j let the grabs themselves (shape 2) through, but
 *   the holds (attacks 0x36 Pyramid Head, 0x3D Flesh Lip, 0x40 Abstract Daddy, 0x42 / 0x45 Mandarin; shape 5/6) hit
 *   ANY James in reach. On James 2, who is not grabbed, a hold hit puts him in the held pose on the holder's spot
 *   with no grab to end it (user's Abstract Daddy fight, 13 Oct: James 2 snapped onto James 1's spot, 100 -> 94 HP,
 *   then walked around hunched). Now the hold's hit record (call 0x52839B in 0x528210) is not made for James 2.
 *
 * Port of the Xbox co-op mod (builds 01-30) to the PC game. Same rules as Xbox build 30:
 *   - one shared screen (P33: or split screen), James 1's camera; James 2 is tethered to stay near him;
 *   - James 2 leaves while James 1 is in a cutscene / door / script / menu and comes back beside him a second
 *     after James 1 can move again; START (pad 2) or G (keyboard) joins;
 *   - only James 1 skips scenes and uses the menus/inventory; saves never contain James 2.
 * Added since P1 (see CO-OP MOD - READ ME.txt): James 2's weapon copy (P10), doors / save points / items (P9, P20,
 * P25), enemies targeting the nearer James (P9), red jacket (P11), own bone slots (P29), flashlight (P29-P32).
 *
 * Controls (build P3): sh2coop.ini [input] james1 / james2 = keyboard | pad1 | pad2 | keyboard+pad1.
 *   While James 2 is spawned, each James only sees his assigned device(s); both use the game's own control layout
 *   (Options > Control Options), because the mod runs the game's own keyboard/joystick conversion once per James.
 *   Pad 2 is opened through the game's dinput8.dll (Xidi), so it maps exactly like pad 1.
 *   G, or BACK+START together on any pad = join (with "auto" that device becomes James 2's and James 1 keeps the
 *   rest), Z or BACK alone on James 2's pad = regroup, James 2's Map button = map, F6 = swap the two devices,
 *   pad 1 LT+RT+LB+RB = James 1 jumps to James 2, F10 = status line in the log.
 *
 * Everything the mod does is written to sh2coop.log next to the game (P33d: the previous session's log is kept as
 * "sh2coop - <date> <time>.log").
 */
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <d3d8.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include "sh2coop_ce.h"
#include "sh2coop_chr.h"
static int s_chr_sel[2];                     /* CS: character per James (CHR_JAMES = normal) */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

/* ===================== game addresses (EE sh2pc.exe v1.0, sha1 3201a5e1...) =====================
   Most were found by pairing the PC functions with their byte-identical Xbox twins. */
#define PLAYER       (*(u8 **)0x01FB7C80)  /* player work pointer = first dword of the player block */
#define R_BLOCK      0x01FB7C80u, 0x4A4u    /* player block (Xbox 0xe6a9e0) */
#define LIST_HEAD    (*(u8 **)0x01FB6984)  /* active character list */
#define LIST_OWNER   (*(u8 **)0x01FB6988)
#define CHARA_CNT    (*(u8 *)0x01FB698C)   /* live characters, max 32 */
#define POOL_FREE    (*(int *)0x01FAC644)  /* free bone nodes */
#define POOL_HEAD    (*(u8 **)0x01FAC640)  /* free list head */
#define POOL_BASE    ((u8 *)0x01F96840)    /* 400 nodes x 0xE0, built by 0x53AAF0 */
#define POOL_N       400
#define NODE_SZ      0xE0
#define EV_STATE     (*(int *)0x01F7A958)  /* event in progress (0 = none) */
#define EV_KIND      (*(int *)0x01F7AE80)  /* 2 = area script (cutscene) running */
#define EV_SKIP      (*(int *)0x01F7AE84)
#define EV_TRIGGER   (*(int *)0x0094CC58)  /* index of the trigger that started the event */
#define STAGE_BLOCK  (*(u32 *)0x01F7A950)
#define MODE_MAIN    (*(int *)0x00932174)  /* 0xd in game (PC) */
#define MODE_SUB     (*(int *)0x00932178)  /* 4 play, 5 map, 6 inventory, 0xd game over, 0x10 pause */
#define DMG_STAT     (*(u32 *)0x01DBBFA8)  /* "damage James took" ending statistic (Xbox 0xe90068) */
/* the game's input module state (verified against the matched Xbox source of in_build_pad_vin / in_update_edges
   and the PC poll 0x458930): 16 port structs of 0x60 (raw 0-3, stick<->d-pad derived copies 4-7, 8-11, 12-15,
   each with the previous frame, pressed/released edges and per-button hold counters), the virtual-input block
   (2 frames x 42 shorts, timers, frame index; zeroed as one 0x110 block by the game's own init 0x4466C0) and the
   port-0 pressed mask 0x932CD8.  James 2 owns a complete private copy; the game's own code fills it each frame. */
#define IN_PORTS     0x009408A0u
#define IN_PORTS_SZ  0x600u
#define IN_VIN       0x00932B68u
#define IN_VIN_SZ    0x110u
#define IN_EDGE      (*(u32 *)0x00932CD8)
#define VIN_IDX      (*(u8 *)0x00932C74)   /* frame index flipped by the input frame */
#define POLL_TAIL    0x00458A17u           /* poll 0x458930 after the device reads: port-0 conversion, derived ports, edges */
#define SITE_INPUT_FRAME 0x00446950u       /* first instruction of the input frame: call 0x458930 */
#define INST_CTR     0x01D7F5A4u           /* shared instance-id counter inside the statics: never swapped */
#define MAP_PRESSED  (*(u8 *)0x01FB7F14)   /* James 1's "map pressed" byte (Xbox 0xe6ac74) */
#define BLK_WEAPON   0x48C                 /* weapon id inside the player block */
#define BLK_HP       0x460                 /* hp copy inside the player block */

/* per-player statics outside the block that James's own code keeps across frames (PC addresses of the Xbox
   m3_play module statics 0x4afb10..0x4b0080, the nearest-character lists and the last-anim cache) */
typedef struct { u32 a, n; } Region;
/* Build P2: rebuilt from a cross-reference scan of the whole PC exe (every instruction that touches
   0x1D7F540..0x1D7FAC8, grouped by the function that owns it, against the call tree of James's think 0x536380).
   The old whole-span copy was wrong: that span is the .data of the entire character module, and most of it
   belongs to other characters (Maria 0x1D7F544.., the boat 0x1D7F5E0..79C, other NPCs 0x1D7F7A0..7BC,
   0x1D7F920..AB8) or is PER-FRAME SHARED BOOKKEEPING that the frame reset and the enemies write:
     0x1D7F7C0/7C4  enemy-slot counts for the tables 0x1F96700/0x1F965A0 (reset by cl_frame_prepare, filled by
                    enemy thinks).  A private copy for James 2 held a stale count against a freshly reset table,
                    so pl_scan_nearest_enemy read a NULL slot -> the P1/P1b/P1c crash at 0x542437.  (On Xbox
                    reading address 0x10 does not fault, which is why the Xbox build never showed it.)
     0x1D7F7D0..874 attack-request list (cl_push_attack_req_pl/en), 0x1D7F87C attack-volume count.
   What James's own code writes outside the player block is only per-think scratch; copying it is harmless and
   keeps James 2's think self-contained, so these small pieces stay: */
static const Region kStatics[] = {
    { 0x01D7F574, 0x8 },     /* pl_sample_input scratch */
    { 0x01D7F7C8, 0x4 },     /* pl_chainsaw_update */
    { 0x01D7F874, 0x8 },     /* pl_chainsaw_update, ch_process_hit_records last-hit value */
    { 0x01D7F880, 0x24 },    /* movement/heading scratch (pl_sync_heading_3d, room-change part masks) */
    { 0x01D7F8A4, 0x4 },     /* 2-D movement context pointer */
    { 0x01D7F914, 0x4 },     /* state-handler context pointer */
    { 0x008BA49C, 0x8 },     /* last lower/upper anim state (pl_anim_select_lower/upper) */
};
#define NSTAT (sizeof kStatics / sizeof kStatics[0])
#define STAT_TOTAL 0x700

/* functions */
typedef void (__cdecl *ThinkFn)(u8 *);
typedef void (__cdecl *VoidFn)(void);
#define F_UPDATE_ALL   ((VoidFn)0x00538C40)
#define F_JAMES_THINK  ((ThinkFn)0x00536380)
#define F_SPAWN_EV     ((void (__cdecl *)(int, int, float *, float *, int))0x0048F810)  /* ev_spawn_character */
#define F_FIND         ((u8 *(__cdecl *)(int, int))0x00537D00)
#define F_REMOVE       ((int (__cdecl *)(u8 *, int, int))0x0053A700)
#define F_BONES_NEEDED ((int (__cdecl *)(int))0x00537D70)
#define F_TELEPORT     ((void (__cdecl *)(u8 *, float *, float))0x005396C0)
#define F_EVBIT_USE    ((int (__cdecl *)(int))0x0049E970)             /* P33r: the search's one-shot event-bit use (kinds 5..10, word1 bit 3) */
#define F_TRIG_SEARCH  ((int (__cdecl *)(int, int, int))0x0048E070)   /* (action, use-item evid, probe only) -> trigger idx / 0x400+item / -1; reads PLAYER */
#define STAGE_IDX      (*(u8 *)0x01DBBF80)                              /* current stage 1..0x3A */
#define ACTION_EDGE    (*(u8 *)0x01FB7F0C)                              /* James 1's "action pressed this frame" (block +0x28C) */
#define BLK_ACTION     0x28C
/* enemy target choice and hit records (verified in the PC exe; see the enemies map) */
#define F_SELECT_TARGET ((u8 *(__cdecl *)(u8 *, int))0x00542E80)      /* (enemy work, mode 1 player / 2 Maria / else nearer) */
#define SITE_TARGET_A  0x004A5FBEu
#define SITE_TARGET_B  0x004A6F4Eu
#define SITE_FF        0x00545135u
/* James 2's weapon copy (verified in the PC exe: equip 0x48FA20 spawns 0x800+w inst 0 and, with 0x932194 & 0x10, 0x820+w;
   the weapon init equips it on the CURRENT player block via 0x53B540; attach 0x545EF0 takes the hand from the PLAYER's bone
   0x23 and the world matrix from the first James; the post pass 0x538B40 and the attack step 0x543310 run from 0x50B590) */
#define WID1           (*(u8 *)0x01FB810C)                      /* equipped weapon id in the (current) player block */
#define BLK_WCLASS     0x434
#define F_SET_WEAPON   ((void (__cdecl *)(int))0x0053B540)
#define F_POST_PASS    ((void (__cdecl *)(u8 *))0x005384F0)       /* flags & 4: full post-think pass (James, enemies) */
#define F_POST_A       ((void (__cdecl *)(u8 *))0x00538820)       /* work+0x18 set */
#define F_POST_B       ((void (__cdecl *)(u8 *))0x00538750)       /* the rest (weapons, objects) */
#define F_BUILD_ATK    ((VoidFn)0x00545A30)
#define DEMO_FLAGS     (*(u32 *)0x01F7A7A4)
#define ATK_FREE       (*(int *)0x01D7F7D0)
#define ATK_Q          ((u8 *)0x01D7F7D4)
#define ATK_ANY        (*(int *)0x01D7F87C)
#define SITE_POST_ALL  0x0050B5A9u
#define SITE_ATK       0x0050B5AEu
#define ALT_MODEL      (*(u32 *)0x00932194 & 0x10)
/* James 2's look (verified in the PC exe): the character render loop 0x50B980 draws each work through 0x50B3D0(work)
   at 0x50BA2A; every draw resolves the model's material textures through the texture lookup 0x45A2F0(id) (hash
   buckets 0x941380, record +0x8c = IDirect3DTexture8*) and binds them with SetTexture.  James's textures: 1239 face
   (+ scalp hair), 1240 jacket (+ hands), 1241 shirt/jeans/boots/HAIR, 1242 misc.  The Enhanced Edition hooks the
   same lookup from its own code (noose cutscene), so the lookup's ENTRY is hooked here, not its call sites. */
#define D3D_DEVICE     (*(IDirect3DDevice8 **)0x00A32894)
#define SITE_DRAW_WORK 0x0050BA2Au          /* call 0x50B3D0 (work, arg) */
#define F_DRAW_WORK    ((void (__cdecl *)(u8 *, int))0x0050B3D0)
#define TEX_LOOKUP     0x0045A2F0u          /* 8b 4c 24 04 8b c1 : mov ecx,[esp+4] ; mov eax,ecx */
#define TEX_LOOKUP_RET 0x0045A2F6u
#define TEX_REC_D3D    0x8C   /* kind-4 hit record gate inside 0x544F90: cmp word [ebp+0x10],0x12f ; jge accept */
#define F_ROOM_ID      ((int (__cdecl *)(void))0x00494D70)                               /* reads PLAYER */
#define F_SAVE_STATS   ((void (__cdecl *)(void *))0x0053ADC0)
#define F_SFX          ((int (__cdecl *)(int, float, int))0x00515580)
static const u32 kUpdateSites[] = { 0x005073A0, 0x0050B59A };
#define SITE_SAVE_STATS 0x004550F4
/* raw input (found in the PC exe: 0x458930 polls DirectInput, then 0x458060 turns keyboard + joystick into the
   port-0 struct 0x9408A0 using the game's own key/button bindings; 0x446950 runs the virtual-input builders) */
#define KB_BUF    0x00941040u          /* DirectInput keyboard state, 256 bytes (DIK scancodes) */
#define JOY_BUF   0x00940F20u          /* DIJOYSTATE2 of the game's joystick (Xidi controller 1), 0x110 bytes */
#define SYS_CUR   (*(u32 *)0x00941158) /* system-key mask built by 0x458060 (menu keys): current frame */
#define SYS_PREV  (*(u32 *)0x00941154) /*                                                previous frame */
#define SITE_PORT0 0x00458A89u         /* call 0x458060 inside the poll: replaced by port0_conv */
typedef void (__cdecl *ConvFn)(void *port, void *dev);
static ConvFn s_conv_orig = (ConvFn)0x00458060;   /* whatever the call at SITE_PORT0 pointed to (the Enhanced Edition hooks it too) */
static int hook_port0(void);
#define F_CONV    s_conv_orig
#define F_POLL    ((VoidFn)0x00458930)
/* options menu (main page = page 2 / sub-page 0: 10 rows at y = 0x64 + 0x1B*row; the generic cursor routine
   0x45FA90 hard-codes 10 rows for sub-page 0; the page is drawn by 0x4613A0, called only from 0x4696B7) */
#define OPT_PAGE   (*(u8 *)0x00941600)
#define OPT_SUB    (*(u8 *)0x00941601)
#define OPT_SEL    (*(short *)0x00941602)
#define OPT_POPUP  (*(u8 *)0x0094161C)      /* a confirmation box is open */
#define OPT_BUSY   (*(short *)0x00941608)
#define TEXT_FLAGS (*(u32 *)0x01F5F544)     /* 0x100 centre, 0x200 right-align */
#define F_COLOR    ((void (__cdecl *)(int, int, int, int))0x0047E900)
#define F_PREP     ((void *(__cdecl *)(const char *))0x004829F0)     /* ASCII -> game text */
#define F_PRINT    ((void (__cdecl *)(void *, int, int))0x00480500)  /* (text, x, y) */
#define F_OPT_DRAW ((VoidFn)0x004613A0)
#define F_CURSOR   ((VoidFn)0x0045FA90)
#define F_VIN_HELD ((int (__cdecl *)(int, u32, int))0x00446540)      /* (port, mask, 0): held / auto-repeat */
#define F_VIN_EDGE ((int (__cdecl *)(u32))0x00446690)                /* pressed this frame */
#define SITE_OPT_DRAW 0x004696B7u
#define SITE_CURSOR_A 0x00462EE3u
#define SITE_CURSOR_B 0x00464FADu
#define IMM_ROWS   0x0045FAAFu   /* mov ebx, 0xa  -> row count for sub-page 0 */
#define IMM_WRAP_A 0x0045FCB8u   /* mov ax, 9     -> wrap up from row 0 */
#define IMM_WRAP_B 0x0045FCBEu   /* cmp ax, 9     -> wrap down past the last row */
#define COOP_ROW0  10

/* character work fields */
#define W_FLAGS 0x04
#define W_MODEL 0x10
#define W_INST  0x12
#define W_STATE 0x14
#define W_POS   0x1C
#define W_ROT   0x2C
#define W_DMG   0x11C
#define W_HP    0x13C
#define W_HPMAX 0x140
#define W_THINK 0x188
#define W_PREV  0x190
#define W_NEXT  0x194
#define F_CONTROL 0x4u
#define F_DRAWN   0x10u
#define F_GRABBED 0x2000u
#define F_LOCKED  0x4000u
#define F_ESCORT  0x20000u
#define FLAGS(w)  (*(u32 *)((w) + W_FLAGS))
#define MODEL(w)  (*(short *)((w) + W_MODEL))
#define POSP(w)   ((float *)((w) + W_POS))
#define YAW(w)    (*(float *)((w) + W_ROT + 4))

#define SND_TELEPORT 0x2b21
#define SND_SPAWNED  0x2b17
#define SND_FAILED   0x3a9a

/* ===================== state ===================== */
typedef struct {
    u8 block[0x4A4];
    u8 stat[STAT_TOTAL];
} Save;
typedef struct { u8 ports[IN_PORTS_SZ]; u8 vin[IN_VIN_SZ]; u32 edge; } InState;
static Save s_p1, s_p2;
static InState s_in1, s_in2;    /* James 1's input state (parked while James 2's is live) and James 2's */
static int s_pass2;             /* the input conversion currently running is James 2's */
u8 *s_p2c;                      /* James 2's work (global: the friendly-fire stub reads it) */
int s_active;                   /* co-op on (global: same) */
static int s_want, s_inP2;
static int s_follow_timer, s_spawn_grace, s_downed, s_room_cool;
static DWORD s_free_since;      /* GetTickCount when James 1 entered plain free play (0 = not free) */
static float s_p1_last[3]; static int s_p1_last_ok;
static int s_last_r1, s_last_r2;
static float s_t1_old[3], s_t2_old[3]; static int s_t_ok;
static FILE *s_log;
static DWORD s_frame;
static int s_last_ev, s_last_mode, s_last_sub, s_last_kind;
static int s_map_pending;
static int s_own_spawn;            /* the mod itself is spawning (James 2 / weapon copy): the spawn guard lets it through */
int s_join_loud;                 /* the next join attempt was asked for by a key press: play the fail sound if it fails */
/* P15: the map James 2 opened is his: while it is up the game sees James 2's input (scroll, zoom, close) */
static int s_map_p2, s_map_seen, s_map_wait, s_map_armed;   /* a screen James 2 opened (map, save menu, message) */
static const char *s_ui_what = "map";
static u8 *s_w2[2]; static int s_w2_cool;
static int chr_owner(u8 *work, u8 **james);   /* CS: James drawn as another character, or his weapon */
/* P33o: Mary (0x206) defeated in the Mary / Maria arenas (stages 48-51): James 2 stays out until a load or until
   James 1 leaves those stages - the user's run had both Jameses on one spot after the post-fight scene.
   P33p, traced: the ending is NOT an examine. Mary's downed state 0x4B4CA0 (work+0xEC == 0xD) waits for a hit
   (0x4A9210: pending damage +0x11C > 0, damage-type gates), then heals James 1, 120 frames, sets flag 0x1FC ->
   triggers 8 (script) -> 9 (door) -> the ending. Cloud: shooting her lying body started it at once; action did not. */
static int s_final_out, s_final_said;
/* P33r: where James 2 stood when a scene / script took him out; he comes back there (not onto James 1) if James 1 is
   still in the same room and has not moved */
static int s_ret_ok, s_ret_stage, s_ret_room; static float s_ret_pos[4], s_ret_rot[4], s_ret_p1[3];
#define FINAL_AREA() (STAGE_IDX >= 48 && STAGE_IDX <= 51)
static void w2_drop(const char *why);
static int s_opt_weapon = 1, s_opt_target = 1, s_opt_interact = 1, s_opt_look = 1, s_opt_privbones = 1, s_opt_fl2 = 1, s_opt_title = 1, s_opt_dumps = 0;   /* sh2coop.ini [coop] switches for troubleshooting */   /* James 2's copies of James 1's weapon work(s): main and alt model */
/* P33 split-screen state (the split code is further down; these are read by earlier hooks) */
#define SPLIT_OFF  0
#define SPLIT_SIDE 1
#define SPLIT_TOP  2
#define SPLIT_MODES 3                  /* P33c: the diagonal layout (3) is gone */
static int s_split_cfg = SPLIT_SIDE;   /* chosen layout (sh2coop.ini [split] layout, F8, L3+R3, Co-op Options) */
static int s_split_now;                /* this frame is drawn split */
static int s_split_view = -1;          /* view being drawn: 0 = James 1's, 1 = James 2's, -1 = none */
static int s_cam2_valid;               /* James 2's camera copy is live (else it restarts from James 1's) */
static void split_cycle(int dir, const char *by);
static void *s_d9_dev8; static int s_d9_ok;   /* the game's device as seen by sh2coop_d9.c (attached once per device) */
static void split_attach(IDirect3DDevice8 *d);
int d9_get_rs(unsigned state, unsigned *v);
void d9_set_rs(unsigned state, unsigned v);
#ifdef SH2COOP_TEST_TOOLS
typedef struct { u32 magic; volatile int cmd, done; float v[4]; int arg; } TestCmd;
int s_tcap_arm, s_tcap_n, s_tcap_pc = -1, s_tcap_got; unsigned s_tcap_log[512][4]; float s_tcap_vsc[96 * 4];
float s_tview[16]; int s_tview_on; float s_tview_fov = 0.5f;   /* cloud test build: close-up view for James 2's half */
TestCmd s_tcmd = { 0x444D4354u, 0, 0, { 0, 0, 0, 0 }, 0 };   /* cloud test build only: the test harness writes here */
#endif
static int s_freeze;            /* James 1 is in a short interaction: James 2 stands still instead of leaving */
static int s_inject_action;     /* give James 1 one Action press after this frame's update (James 2 used a trigger) */
static volatile const char *s_phase = "start";   /* what the mod (or the game) was doing: written into crash reports */
static volatile int s_in_hook;

static void vLog_fwd(const char *fmt, va_list ap)
{
    if (!s_log) return;
    SYSTEMTIME t; GetLocalTime(&t);
    fprintf(s_log, "%02d:%02d:%02d.%03d f%05lu ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, (unsigned long)(s_frame % 100000));
    vfprintf(s_log, fmt, ap);
    fputc('\n', s_log); fflush(s_log);
}
void ce_log(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vLog_fwd(fmt, ap); va_end(ap); }
static void Log(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vLog_fwd(fmt, ap); va_end(ap); }

/* ===================== save / load James's state ===================== */
static void save_in(InState *s)
{
    memcpy(s->ports, (void *)IN_PORTS, IN_PORTS_SZ); memcpy(s->vin, (void *)IN_VIN, IN_VIN_SZ); s->edge = IN_EDGE;
}
static void load_in(const InState *s)
{
    memcpy((void *)IN_PORTS, s->ports, IN_PORTS_SZ); memcpy((void *)IN_VIN, s->vin, IN_VIN_SZ); IN_EDGE = s->edge;
}
static void save_state(Save *s, int with_input)
{
    u8 *d = s->stat;
    memcpy(s->block, (void *)0x01FB7C80, 0x4A4);
    for (unsigned i = 0; i < NSTAT; i++) { memcpy(d, (void *)kStatics[i].a, kStatics[i].n); d += kStatics[i].n; }
    if (with_input) save_in(s == &s_p1 ? &s_in1 : &s_in2);
}
static void load_state(const Save *s, int with_input)
{
    const u8 *d = s->stat;
    memcpy((void *)0x01FB7C80, s->block, 0x4A4);
    for (unsigned i = 0; i < NSTAT; i++) { memcpy((void *)kStatics[i].a, d, kStatics[i].n); d += kStatics[i].n; }
    if (with_input) load_in(s == &s_p1 ? &s_in1 : &s_in2);
}

/* ===================== list helpers ===================== */
static int is_in_list(u8 *w)
{
    int n = 0;
    for (u8 *p = LIST_HEAD; p && n < 64; p = *(u8 **)(p + W_NEXT), n++) if (p == w) return 1;
    return 0;
}
static void relink_head(u8 *w)
{
    if (LIST_HEAD == w || !is_in_list(w)) return;
    u8 *prev = *(u8 **)(w + W_PREV), *next = *(u8 **)(w + W_NEXT);
    if (prev) *(u8 **)(prev + W_NEXT) = next;
    if (next) *(u8 **)(next + W_PREV) = prev;
    *(u8 **)(w + W_PREV) = NULL; *(u8 **)(w + W_NEXT) = LIST_HEAD;
    if (LIST_HEAD) *(u8 **)(LIST_HEAD + W_PREV) = w;
    LIST_HEAD = w;
}
static float dist2(const float *a, const float *b) { float dx = a[0] - b[0], dz = a[2] - b[2]; return dx * dx + dz * dz; }
static int room_of(u8 *c) { u8 *save = PLAYER; PLAYER = c; int r = F_ROOM_ID(); PLAYER = save; return r; }
static void sfx(int id) { F_SFX(id, 1.0f, 0); }

/* P33h: James 1 is held (James state 0x19 = grabbed / being killed) or dying (0x1FB8117).
   The choke does NOT set F_GRABBED (the user's log: flags 0x214 all through Pyramid Head's choke), so p1_free let
   James 2 join, re-join after being downed, be regrouped into Pyramid Head's hands, or pull James 1 out of them.
   A James 2 made at that moment starts from a copy of James 1's block (save_state(&s_p2) right after the spawn):
   his first think (0x536380, work+0x14 = 0 -> 0x5349C0) clears the dying flag but keeps the grab phase and meter,
   and with state 0x19 it skips the animation reset - he would start inside a grab that has no attacker. */
static int p1_held_or_dying(void)
{
    /* the phase byte alone is not used: nothing resets it after a death + Continue (only the grab code writes it) */
    return *(u8 *)0x01FB8117 || *(u32 *)0x01FB7CE8 == 0x19 || *(u32 *)0x01FB7CE4 == 0x19;
}
/* James 1 is in plain, free play: the only state in which James 2 may exist, join or be brought over */
static int p1_free(u8 *p1)
{
    return p1 && (FLAGS(p1) & F_CONTROL) && (FLAGS(p1) & F_DRAWN) && !(FLAGS(p1) & (F_LOCKED | F_GRABBED | F_ESCORT))
        && EV_STATE == 0 && EV_KIND == 0 && MODE_SUB == 4 && !p1_held_or_dying();
}
/* P33p: James 1 is only held in a grab (no scene, no menu, not dying): James 2's weapon copy may be re-made then.
   User: James 2's gun vanished after James 1 healed in the inventory while held - the copy is dropped when a menu
   opens and was only re-made once James 1 was free again (seconds later). The copy is spawned on James 2's own
   swapped block, so James 1's grab state is not involved. */
static int p1_only_held(u8 *p1)
{
    return p1 && (FLAGS(p1) & F_DRAWN) && !(FLAGS(p1) & (F_LOCKED | F_ESCORT)) && EV_STATE == 0 && EV_KIND == 0
        && MODE_MAIN == 0xD && MODE_SUB == 4 && !*(u8 *)0x01FB8117 && (*(u32 *)0x01FB7CE8 == 0x19 || *(u32 *)0x01FB7CE4 == 0x19);
}
#define SETTLED (s_free_since && GetTickCount() - s_free_since >= 1000)

/* a spot 0.6 m behind c */
/* a free spot near c: tried in front first, then left, right, behind (collision ray from c's chest, the game's own
   line test 0x527C00: result 0 = clear, 3 = only a character in the way, 1/2 = wall / fence) */
#define F_LINE_TEST ((void (__cdecl *)(int *, u8 *, float *, float *, int))0x00527C00)
__attribute__((unused)) static void spot_near(u8 *c, float *out, int prefer_back)
{
    static const float kAng[2][5] = { { 0.0f, 1.5708f, -1.5708f, 3.1416f, 0.0f }, { 3.1416f, 1.5708f, -1.5708f, 0.0f, 3.1416f } };
    static const float kDist[5] = { 700.0f, 700.0f, 700.0f, 700.0f, 300.0f };
    float yaw = YAW(c), from[4] = { POSP(c)[0], *(float *)(c + 0xA0), POSP(c)[2], 1.0f };
    for (int i = 0; i < 5; i++) {
        float a = yaw + kAng[prefer_back][i];
        float to[4] = { POSP(c)[0] + sinf(a) * kDist[i], from[1], POSP(c)[2] + cosf(a) * kDist[i], 1.0f };
        float far_[4] = { POSP(c)[0] + sinf(a) * (kDist[i] + 300.0f), from[1], POSP(c)[2] + cosf(a) * (kDist[i] + 300.0f), 1.0f };
        int res[16]; memset(res, 0, sizeof res);
        F_LINE_TEST(res, c, from, far_, 1);                       /* test a bit beyond the spot so he is not pressed into a fence */
        if (res[0] == 0 || res[0] == 3) { out[0] = to[0]; out[1] = POSP(c)[1]; out[2] = to[2]; out[3] = 1.0f; return; }
    }
    out[0] = POSP(c)[0] + sinf(yaw) * 150.0f; out[1] = POSP(c)[1]; out[2] = POSP(c)[2] + cosf(yaw) * 150.0f; out[3] = 1.0f;
}
/* P14: every co-op teleport puts the moved James exactly where the other one stands (the two pass through each other,
   see body_pair_c), so no spot can end up inside a car, behind a gate or past a wall */
static void spot_on(u8 *c, float *out) { memcpy(out, POSP(c), 12); out[3] = 1.0f; }
__attribute__((unused)) static void spot_behind(u8 *c, float *out)
{
    float y = YAW(c);
    out[0] = POSP(c)[0] - sinf(y) * 600.0f; out[1] = POSP(c)[1]; out[2] = POSP(c)[2] - cosf(y) * 600.0f; out[3] = 1.0f;
}

/* ===================== the two Jameses pass through each other =====================
   cl_resolve_bodies 0x527290 -> 0x526280 pushes every pair of body cylinders apart; the pair test is the call at
   0x52666B: cl_cylinder_overlap(res, &other_record+0x10, mine) with ebp = my record index.  Records: 0x1F91760 +
   (bank 0x1F8F25C * 32 + i) * 0xC4, work* at +0x60.  For the James 1 / James 2 pair the test reports "no overlap". */
#define SITE_BODY_PAIR 0x0052666B
#define F_CYL_OVERLAP ((int (__cdecl *)(void *, u8 *, void *))0x00529C50)
int __cdecl body_pair_c(void *res, u8 *other, void *mine, int idx)
{
    if (s_active && s_p2c && (u32)idx < 32) {
        u8 *w_other = *(u8 **)(other + 0x50);
        u8 *w_mine = *(u8 **)(0x01F91760 + (*(int *)0x01F8F25C * 32 + idx) * 0xC4 + 0x60);
        u8 *p1 = PLAYER;
        if ((w_mine == s_p2c && w_other == p1) || (w_mine == p1 && w_other == s_p2c)) return 0;
    }
    return F_CYL_OVERLAP(res, other, mine);
}
extern void body_pair_stub(void);
__asm__(
    ".text\n"
    "_body_pair_stub:\n"
    "    pushl %ebp\n"
    "    pushl 0x10(%esp)\n"
    "    pushl 0x10(%esp)\n"
    "    pushl 0x10(%esp)\n"
    "    call _body_pair_c\n"
    "    addl $0x10, %esp\n"
    "    ret\n"
);

/* ===================== James 2's controls ===================== */
typedef DWORD (WINAPI *XInputGetStateFn)(DWORD, void *);
typedef struct { WORD wButtons; BYTE bLT, bRT; SHORT sLX, sLY, sRX, sRY; } XGP;
typedef struct { DWORD dwPacket; XGP g; } XST;
static XInputGetStateFn pXInputGetState;
/* ---- who controls whom (sh2coop.ini [input] james1 / james2: keyboard | pad1 | pad2 | keyboard+pad1; F6 swaps) ---- */
enum { SRC_KB = 0, SRC_PAD1 = 1, SRC_PAD2 = 2, SRC_KBPAD1 = 3, SRC_KBPAD2 = 4, SRC_AUTO = 5 };
static const char *kSrcName[] = { "keyboard", "pad1", "pad2", "keyboard+pad1", "keyboard+pad2", "auto" };
static int s_auto = 1;           /* the device that presses JOIN becomes James 2's; James 1 gets the rest */
static int s_cfg[2] = { SRC_AUTO, SRC_AUTO };   /* as configured (ini / menu); s_asg is the effective assignment */
static char s_ini[MAX_PATH];
static int s_asg[2] = { SRC_PAD1, SRC_KB };
static int s_prev_swapkey;
static LPDIRECTINPUT8A s_di; static LPDIRECTINPUTDEVICE8A s_dev2; static int s_di_tried, s_dev2_n;
static u8 s_joy2[0x110]; static int s_joy2_ok;
#define XB_UP 0x1
#define XB_DOWN 0x2
#define XB_LEFT 0x4
#define XB_RIGHT 0x8
#define XB_START 0x10
#define XB_BACK 0x20
#define XB_LB 0x100
#define XB_RB 0x200
#define XB_A 0x1000
#define XB_B 0x2000
#define XB_X 0x4000
#define XB_Y 0x8000
/* the game's action masks (bit per action, keyconf.dat) */
#define M_UP 0x1
#define M_DOWN 0x2
#define M_LEFT 0x4
#define M_RIGHT 0x8
#define M_MAP 0x100
#define M_ACTION 0x200
#define M_STRAFEL 0x400
#define M_STRAFER 0x800
#define M_READY 0x4000
#define M_RUN 0x8000

typedef struct { int join, regroup, map; } P2In;
static int s_prev_join, s_prev_regroup, s_prev_map, s_prev_combo1, s_prev_dump;
static int game_focused(void)
{
    DWORD pid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}
static int key(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }
static int read_pad(int user, XGP *g)
{
    XST st;
    if (!pXInputGetState) return 0;
    memset(&st, 0, sizeof st);
    if (pXInputGetState(user, &st) != 0) return 0;
    *g = st.g; return 1;
}
/* P19: device safety. A choice that names a controller which is not plugged in must never leave James 1 without
   input (P18: James 1 = Controller 2 with no second controller locked the player out of every menu, and the choice
   was saved). pad1 / pad2 = XInput users 0 / 1 (the same order Xidi gives the game). Checked at most once a second. */
static int pad_present(int u)
{
    static DWORD last[2]; static int on[2] = { 1, 1 };
    if (!pXInputGetState || u < 0 || u > 1) return 1;
    DWORD t = GetTickCount();
    if (!last[u] || t - last[u] > 1000) { XST st; memset(&st, 0, sizeof st); on[u] = pXInputGetState(u, &st) == 0; last[u] = t | 1; }
    return on[u];
}
static int src_works(int src) { return !((src == SRC_PAD1 && !pad_present(0)) || (src == SRC_PAD2 && !pad_present(1))); }
static int eff_src(int user)
{
    static int warned[2];
    int src = s_asg[user];
    if (src_works(src)) { warned[user] = 0; return src; }
#ifdef SH2COOP_TEST_J1_IDLE
    if (user == 0) return src;                       /* cloud test build only: James 1 on an absent pad = no input */
#endif
    int fb = src;
    if (user == 0) fb = (s_asg[1] != SRC_PAD1 && s_asg[1] != SRC_KBPAD1 && pad_present(0)) ? SRC_KBPAD1 : SRC_KB;
    if (!warned[user]) { warned[user] = 1; Log("James %d's device (%s) is not connected%s", user + 1, kSrcName[src], user == 0 ? "; James 1 uses the keyboard until it is" : ""); }
    return fb;
}
static int parse_src(const char *v, int dflt)
{
    char t[32]; int i = 0;
    for (; v[i] && i < 31; i++) t[i] = (char)((v[i] >= 'A' && v[i] <= 'Z') ? v[i] + 32 : v[i]);
    t[i] = 0;
    if (!strcmp(t, "keyboard") || !strcmp(t, "kb")) return SRC_KB;
    if (!strcmp(t, "pad1") || !strcmp(t, "controller1") || !strcmp(t, "pad")) return SRC_PAD1;
    if (!strcmp(t, "pad2") || !strcmp(t, "controller2")) return SRC_PAD2;
    if (!strcmp(t, "keyboard+pad1") || !strcmp(t, "both") || !strcmp(t, "all")) return SRC_KBPAD1;
    if (!strcmp(t, "keyboard+pad2")) return SRC_KBPAD2;
    if (!strcmp(t, "auto")) return SRC_AUTO;
    return dflt;
}
static void apply_cfg(void)
{
    s_auto = (s_cfg[0] == SRC_AUTO || s_cfg[1] == SRC_AUTO);
    s_asg[0] = s_cfg[0] == SRC_AUTO ? SRC_KBPAD1 : s_cfg[0];
    s_asg[1] = s_cfg[1] == SRC_AUTO ? SRC_PAD2 : s_cfg[1];
}
static void save_ini(void)
{
    if (!s_ini[0]) return;
    WritePrivateProfileStringA("input", "james1", kSrcName[s_cfg[0]], s_ini);
    WritePrivateProfileStringA("input", "james2", kSrcName[s_cfg[1]], s_ini);
}
static void load_ini(HMODULE self)
{
    char path[MAX_PATH], v[64]; GetModuleFileNameA(self, path, MAX_PATH);
    char *dot = strrchr(path, '.'); if (dot) strcpy(dot, ".ini");
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
        FILE *f = fopen(path, "w");
        if (f) {
            fputs("; sh2coop.ini - who controls which James while co-op is on (James 2 spawned).\n"
                  "; values: keyboard | pad1 | pad2 | keyboard+pad1   (pad1 = first controller, pad2 = second)\n"
                  "; F6 in game swaps the two. While James 2 is not spawned the game reads everything as usual.\n"
                  "; auto = the device that presses JOIN (G, or BACK+START on a pad) becomes James 2's and James 1 keeps the rest.\n"
                  "[input]\njames1 = auto\njames2 = auto\n"
                  "\n; split screen while James 2 is in: side | top | off   (F8 or L3+R3 in game, or Options > Game Options > Co-op Options)\n"
                  "[split]\nlayout = side\n", f);
            fclose(f);
        }
    }
    strcpy(s_ini, path);
    s_opt_weapon = GetPrivateProfileIntA("coop", "weapon", 1, path);
    s_opt_target = GetPrivateProfileIntA("coop", "enemies_target_nearer", 1, path);
    s_opt_interact = GetPrivateProfileIntA("coop", "james2_interact", 1, path);
    s_opt_look = GetPrivateProfileIntA("coop", "james2_red_jacket", 1, path);
    s_opt_privbones = GetPrivateProfileIntA("coop", "james2_own_bones", 1, path);
    s_opt_fl2 = GetPrivateProfileIntA("coop", "james2_flashlight", 1, path);
    s_opt_title = GetPrivateProfileIntA("coop", "title_screen", 1, path);
    s_opt_dumps = GetPrivateProfileIntA("coop", "texture_dumps", 0, path);     /* P33q: off unless asked for (writes copies of the game's textures) */
    if (GetPrivateProfileStringA("coop", "title_screen", "", v, sizeof v, path) == 0)          /* P33e: make the switch visible in older ini files */
        WritePrivateProfileStringA("coop", "title_screen", s_opt_title ? "1" : "0", path);
    Log("options: weapon copy %d, enemies target nearer James %d, James 2 interacts %d, red jacket %d, own bone slots %d, flashlight %d",
        s_opt_weapon, s_opt_target, s_opt_interact, s_opt_look, s_opt_privbones, s_opt_fl2);
    GetPrivateProfileStringA("split", "layout", "side", v, sizeof v, path);
    { char t[32]; int i = 0, was_diag; for (; v[i] && i < 31; i++) t[i] = (char)((v[i] >= 'A' && v[i] <= 'Z') ? v[i] + 32 : v[i]); t[i] = 0;
      s_split_cfg = !strcmp(t, "off") || !strcmp(t, "0") || !strcmp(t, "shared") ? SPLIT_OFF : !strncmp(t, "top", 3) || !strcmp(t, "horizontal") ? SPLIT_TOP : SPLIT_SIDE;
      was_diag = !strncmp(t, "diag", 4);
      if (GetPrivateProfileStringA("split", "layout", "", v, sizeof v, path) == 0 || was_diag)   /* add the section to older ini files */
          WritePrivateProfileStringA("split", "layout", s_split_cfg == SPLIT_TOP ? "top" : s_split_cfg == SPLIT_OFF ? "off" : "side", path);
      if (was_diag) Log("split screen: the ini said \"diagonal\" - that layout was removed in P33c, side by side instead (ini rewritten)"); }
    Log("split screen: %s (sh2coop.ini [split] layout = side | top | off; F8 or L3+R3 in game, or Co-op Options)",
        s_split_cfg == SPLIT_OFF ? "off (one shared screen)" : s_split_cfg == SPLIT_TOP ? "top and bottom" : "side by side");
    GetPrivateProfileStringA("input", "james1", "auto", v, sizeof v, path); s_cfg[0] = parse_src(v, SRC_AUTO);
    GetPrivateProfileStringA("input", "james2", "auto", v, sizeof v, path); s_cfg[1] = parse_src(v, SRC_AUTO);
    apply_cfg();
    Log("input assignment (%s): %sJames 1 = %s, James 2 = %s", path, s_auto ? "auto (whoever presses join is James 2); until then " : "", kSrcName[s_asg[0]], kSrcName[s_asg[1]]);
    ce_load(path, s_opt_look);                                   /* CE: [look] colours (defaults = the red jacket / brown hair James 2 had) */
    for (int w = 0; w < 2; w++) {                                /* CS: [look] jamesN_character = James | Maria | Eddie | Angela */
        char key[32], val[32]; snprintf(key, sizeof key, "james%d_character", w + 1);
        GetPrivateProfileStringA("look", key, "James", val, sizeof val, path);
        s_chr_sel[w] = 0;
        for (int k = 0; k < CHR_COUNT; k++) if (!_stricmp(val, chr_name(k))) s_chr_sel[w] = k;
        WritePrivateProfileStringA("look", key, chr_name(s_chr_sel[w]), path);
    }
    Log("characters: James 1 plays as %s, James 2 plays as %s", chr_name(s_chr_sel[0]), chr_name(s_chr_sel[1]));
    chr_mcolor_load(path);                                       /* CS: [look] jamesN_maria_top / skirt / boots / hair */
}

/* ---- second controller: opened through the game's own dinput8.dll (Xidi), so it gets the same mapping as pad 1 ---- */
static HWND s_hwnd;
static BOOL CALLBACK find_wnd(HWND h, LPARAM lp) { DWORD pid = 0; GetWindowThreadProcessId(h, &pid); if (pid == GetCurrentProcessId() && IsWindowVisible(h)) { s_hwnd = h; return FALSE; } (void)lp; return TRUE; }
static BOOL CALLBACK enum_dev(LPCDIDEVICEINSTANCEA di, LPVOID ref)
{
    int *n = (int *)ref; (*n)++;
    Log("  controller %d: %s", *n, di->tszInstanceName);
    if (*n == 2 && s_di) {
        if (s_di->lpVtbl->CreateDevice(s_di, &di->guidInstance, &s_dev2, NULL) != DI_OK) { s_dev2 = NULL; Log("  could not open controller 2"); }
    }
    return DIENUM_CONTINUE;
}
/* an idle DIJOYSTATE2: axes 0, buttons 0 and the four POV hats CENTRED (0xFFFFFFFF).  A plain zero would mean
   "d-pad up" to the Enhanced Edition's d-pad movement code, which walked James forward on its own. */
static void joy_clear(void *j)
{
    memset(j, 0, 0x110);
    for (int i = 0; i < 4; i++) *(u32 *)((u8 *)j + 0x20 + 4 * i) = 0xFFFFFFFFu;
}
static void open_pad2(void)
{
    { static DWORD last; DWORD t = GetTickCount();                 /* not found yet (plugged in later): look again every 3 s */
      if (s_di_tried && !s_dev2 && t - last > 3000) { last = t; s_di_tried = 0; if (s_di) { s_di->lpVtbl->Release(s_di); s_di = NULL; } } }
    if (s_di_tried) return;
    s_di_tried = 1;
    HMODULE h = LoadLibraryA("dinput8.dll");
    HRESULT (WINAPI *create)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN) = h ? (void *)GetProcAddress(h, "DirectInput8Create") : NULL;
    if (!create || create(GetModuleHandleA(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&s_di, NULL) != DI_OK || !s_di) { Log("pad2: DirectInput8Create failed"); return; }
    Log("pad2: looking for controllers through dinput8.dll");
    s_dev2_n = 0;
    s_di->lpVtbl->EnumDevices(s_di, DI8DEVCLASS_GAMECTRL, enum_dev, &s_dev2_n, DIEDFL_ATTACHEDONLY);
    if (!s_dev2) { Log("pad2: fewer than two controllers found (%d)", s_dev2_n); return; }
    EnumWindows(find_wnd, 0);
    s_dev2->lpVtbl->SetDataFormat(s_dev2, &c_dfDIJoystick2);
    s_dev2->lpVtbl->SetCooperativeLevel(s_dev2, s_hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    DIPROPRANGE r; memset(&r, 0, sizeof r); r.diph.dwSize = sizeof r; r.diph.dwHeaderSize = sizeof r.diph; r.diph.dwHow = DIPH_DEVICE; r.lMin = -32767; r.lMax = 32767;
    s_dev2->lpVtbl->SetProperty(s_dev2, DIPROP_RANGE, &r.diph);
    HRESULT hr = s_dev2->lpVtbl->Acquire(s_dev2);
    Log("pad2: controller 2 opened (acquire %08lx)", (unsigned long)hr);
}
static void poll_pad2(void)
{
    s_joy2_ok = 0;
    if (!s_dev2) { joy_clear(s_joy2); return; }
    s_dev2->lpVtbl->Poll(s_dev2);
    HRESULT hr = s_dev2->lpVtbl->GetDeviceState(s_dev2, sizeof s_joy2, s_joy2);
    if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) { s_dev2->lpVtbl->Acquire(s_dev2); hr = s_dev2->lpVtbl->GetDeviceState(s_dev2, sizeof s_joy2, s_joy2); }
    if (hr != DI_OK) joy_clear(s_joy2); else s_joy2_ok = 1;
}

/* run the game's keyboard+joystick -> port struct conversion with only the chosen source visible */
static void convert_from(int src, void *port, void *dev)
{
    u8 kb[0x100], joy[0x110];
    memcpy(kb, (void *)KB_BUF, 0x100); memcpy(joy, (void *)JOY_BUF, 0x110);
    if (src == SRC_PAD1 || src == SRC_PAD2) memset((void *)KB_BUF, 0, 0x100);
    if (src == SRC_KB) joy_clear((void *)JOY_BUF);
    else if (src == SRC_PAD2 || src == SRC_KBPAD2) memcpy((void *)JOY_BUF, s_joy2, 0x110);
    F_CONV(port, dev);
    memcpy((void *)KB_BUF, kb, 0x100); memcpy((void *)JOY_BUF, joy, 0x110);
}
/* replaces the call to the keyboard+joystick -> port-0 conversion inside the game's input poll (chained behind the
   Enhanced Edition's own hook of the same call).  Converts for the James whose pass is running. */
static void __cdecl port0_conv(void *port, void *dev)
{
    if (!s_active) { s_map_p2 = 0; F_CONV(port, dev); return; }  /* solo: the game reads everything as usual */
    if (!s_pass2) {
        /* P18: the map James 2 opened is driven by James 2's device.  Done here, inside the game's poll, because the
           map screen polls through its own path (P15 swapped states in the input-frame hook and the map never saw it) */
        /* P20: generalised from the map to every screen James 2 opens: the save menu (traced: 0x453D60 reads only
           the held / pressed queries 0x446540 / 0x446690, like the map), item / memo / door messages. James 2 owns it
           from the moment the game leaves plain play until it is back in plain play. Cutscenes (event kind 2) stay
           James 1's (skipping is James 1's job). */
        if (s_map_p2) {
            int in_play = MODE_SUB == 4 && EV_STATE == 0;
            if (!in_play) { if (!s_map_seen) Log("James 2's %s is up (mode %d, event state %d kind %d)", s_ui_what, MODE_SUB, EV_STATE, EV_KIND); s_map_seen = 1; }
            else if (s_map_seen || ++s_map_wait > 90) { s_map_p2 = 0; if (s_map_seen) Log("James 2's %s closed", s_ui_what); }
        }
        int p2ui = s_map_p2 && s_map_seen && EV_KIND != 2;
        if (p2ui && eff_src(1) >= SRC_PAD2) poll_pad2();       /* the map's own poll path does not run the input-frame hook */
        if (p2ui && !s_map_armed) {                      /* hand over only once James 2 has let go of the button that opened it,
                                                                    or the held Map button would read as a new press and close it again */
            u8 tmp[0x60]; memcpy(tmp, port, sizeof tmp);
            u32 c = SYS_CUR, pv = SYS_PREV;
            convert_from(eff_src(1), tmp, dev);
            SYS_CUR = c; SYS_PREV = pv;
            if (*(u32 *)(tmp + 0x18) == 0) { s_map_armed = 1; Log("James 2's %s: his controls now drive it", s_ui_what); }
        }
        convert_from(p2ui && s_map_armed ? eff_src(1) : eff_src(0), port, dev);
        return;
    }
    u32 cur = SYS_CUR, prev = SYS_PREV;                          /* menu / system keys stay James 1's ... */
    convert_from(eff_src(1), port, dev);
    SYS_CUR = cur; SYS_PREV = prev;
}
/* the game's own builders (register arguments): pad virtual inputs, keyboard/d-pad virtual sticks, edges */
static void call_builders(void)
{
    __asm__ __volatile__(
        "xorl %%eax, %%eax\n\tmovl $0x446140, %%edx\n\tcall *%%edx\n\t"
        "xorl %%eax, %%eax\n\tmovl $0x4466F0, %%edx\n\tcall *%%edx\n\t"
        "xorl %%ecx, %%ecx\n\tmovl $0x446360, %%edx\n\tcall *%%edx"
        ::: "eax", "ecx", "edx", "memory", "cc");
}
/* the second half of the game's poll (no device reads): port-0 conversion, derived ports, edges for all ports.
   Entered at POLL_TAIL with ebx = 0 as the real function has it; it ends in the game's own pop ebx / jmp / ret. */
static void poll_tail(void)
{
    __asm__ __volatile__(
        "pushl %%ebx\n\tpushl %%esi\n\tpushl %%edi\n\tpushl %%ebp\n\t"
        "call 1f\n\tjmp 2f\n\t"
        "1: pushl %%ebx\n\txorl %%ebx, %%ebx\n\tmovl $0x458A17, %%eax\n\tjmp *%%eax\n\t"
        "2: popl %%ebp\n\tpopl %%edi\n\tpopl %%esi\n\tpopl %%ebx"
        ::: "eax", "ecx", "edx", "memory", "cc");
}
/* replaces the call to the poll at the start of the game's input frame 0x446950.  The game then flips the frame
   index and runs the builders for the live (James 1) state; James 2's complete input state gets the same treatment
   here, from his own devices, and is parked again until his think swaps it in. */
static void __cdecl input_frame_hook(void)
{
    if (s_active && (eff_src(0) >= SRC_PAD2 || eff_src(1) >= SRC_PAD2)) { open_pad2(); poll_pad2(); }
    s_pass2 = 0;
    /* any menu / load: the inventory may change the weapon and unload the old model under the copy -> drop it now, re-made in play */
    if ((s_w2[0] || s_w2[1]) && !(MODE_MAIN == 0xD && MODE_SUB == 4)) w2_drop("menu / load");   /* PC: 0xD = in game (P11 had 0xC: copy dropped every frame) */
    F_POLL();                                                    /* devices read once; James 1's conversion */
    if (!s_active) return;
    s_phase = "James 2 input";
    save_in(&s_in1); load_in(&s_in2);
    s_pass2 = 1; poll_tail(); s_pass2 = 0;
    VIN_IDX = (u8)((VIN_IDX - 1) & 1);
    call_builders();
    save_in(&s_in2); load_in(&s_in1);
    s_phase = "game (outside the co-op frame hook)";
}

static int s_join_src = -1;     /* which device pressed join this frame (for auto assignment) */
static int pad_uses(int src, int user) { return (user == 0) ? (src == SRC_PAD1 || src == SRC_KBPAD1) : (src == SRC_PAD2 || src == SRC_KBPAD2); }
static void read_p2(P2In *in)
{
    memset(in, 0, sizeof *in); s_join_src = -1;
    if (!game_focused()) return;
    if (key('G')) { in->join = 1; s_join_src = SRC_KB; }
    in->regroup = key('Z');
    for (int u = 0; u < 2; u++) {
        XGP g; if (!read_pad(u, &g)) continue;
        if ((g.wButtons & (XB_START | XB_BACK)) == (XB_START | XB_BACK)) { in->join = 1; if (s_join_src < 0) s_join_src = u ? SRC_PAD2 : SRC_PAD1; }
        else if ((g.wButtons & XB_BACK) && pad_uses(s_asg[1], u)) in->regroup = 1;   /* BACK alone on James 2's pad */
    }
    int swap = key(VK_F6);
    if (swap && !s_prev_swapkey) { int t = s_asg[0]; s_asg[0] = s_asg[1]; s_asg[1] = t; Log("F6: James 1 = %s, James 2 = %s", kSrcName[s_asg[0]], kSrcName[s_asg[1]]); sfx(SND_TELEPORT); }
    s_prev_swapkey = swap;
    { static int prev7; int r = key(VK_F7);                    /* F7: reset both Jameses to Auto (way out of any bad choice) */
      if (r && !prev7) { s_cfg[0] = s_cfg[1] = SRC_AUTO; apply_cfg(); save_ini(); Log("F7: devices reset to Auto (James 1 = %s, James 2 = %s)", kSrcName[s_asg[0]], kSrcName[s_asg[1]]); sfx(SND_TELEPORT); }
      prev7 = r; }
    /* map: James 2's own Map button (pressed-this-frame edge of his port 0) */
    in->map = (*(u32 *)(s_in2.ports + 0x38) & M_MAP) != 0;
}
/* auto assignment: the device that pressed join is James 2's, James 1 keeps everything else */
static void assign_from_join(void)
{
    if (!s_auto || s_join_src < 0) return;
    int j2 = s_join_src, j1;
    if (j2 == SRC_KB) j1 = SRC_PAD1;                 /* keyboard joined: James 1 = pad 1 (+ pad 2 unused) */
    else if (j2 == SRC_PAD1) j1 = SRC_KBPAD2;        /* pad 1 joined:    James 1 = keyboard + pad 2 */
    else j1 = SRC_KBPAD1;                            /* pad 2 joined:    James 1 = keyboard + pad 1 */
    if (j1 != s_asg[0] || j2 != s_asg[1]) { s_asg[0] = j1; s_asg[1] = j2; Log("join from %s: James 1 = %s, James 2 = %s", kSrcName[j2], kSrcName[j1], kSrcName[j2]); }
}

/* ===================== Options menu: "Co-op Options" row and its own page ===================== */
static const char *kMenuName[] = { "Keyboard", "Controller 1", "Controller 2", "Keyboard + Controller 1", "Keyboard + Controller 2", "Auto" };
static const int kMenu1[] = { SRC_AUTO, SRC_KBPAD1, SRC_PAD1, SRC_KB, SRC_KBPAD2, SRC_PAD2 };
static const int kMenu2[] = { SRC_AUTO, SRC_PAD2, SRC_KB, SRC_PAD1 };
static char s_dir[MAX_PATH];                 /* the mod's folder (defined with the look diagnostics below) */
static int s_coop_page, s_coop_sel;          /* our own page, shown instead of the main options page (2 = Appearance, CE) */
static int s_app_sel, s_app_who; static float s_app_yaw;
#define APP_PREVIEW_X 400                    /* CE: preview rectangle in the menu's 640x480 units */
#define APP_PREVIEW_Y 60
#define APP_PREVIEW_W 170
#define APP_PREVIEW_H 330             /* CE: Appearance page row (0 = which James, 1.. = parts) and the James being edited */
#define F_OPT_HANDLER ((VoidFn)0x00462E50)
#define SITE_OPT_HANDLER 0x00469734u
#define MASK_CANCEL (*(u32 *)0x00932C7C)
#define MOUSE_X     (*(int *)0x00941358)
#define MOUSE_Y     (*(int *)0x0094135C)
#define MOUSE_CLICK (*(u8 *)0x00941209)     /* bit 0: left button pressed this frame */
/* real controller names, from the system's own DirectInput (not Xidi's), in enumeration order */
static char s_padname[4][64]; static int s_padnames, s_padnames_tried;
static BOOL CALLBACK enum_names(LPCDIDEVICEINSTANCEA di, LPVOID ref)
{
    (void)ref;
    if (s_padnames < 4) { strncpy(s_padname[s_padnames], di->tszProductName, 63); s_padname[s_padnames][63] = 0; s_padnames++; }
    return DIENUM_CONTINUE;
}
static void load_padnames(void)
{
    if (s_padnames_tried) return;
    s_padnames_tried = 1; s_padnames = 0;
    char path[MAX_PATH]; GetSystemDirectoryA(path, MAX_PATH); strcat(path, "\\dinput8.dll");
    HMODULE h = LoadLibraryA(path);
    HRESULT (WINAPI *create)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN) = h ? (void *)GetProcAddress(h, "DirectInput8Create") : NULL;
    LPDIRECTINPUT8A di = NULL;
    if (!create || create(GetModuleHandleA(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&di, NULL) != DI_OK || !di) return;
    di->lpVtbl->EnumDevices(di, DI8DEVCLASS_GAMECTRL, enum_names, NULL, DIEDFL_ATTACHEDONLY);
    di->lpVtbl->Release(di);
    for (int i = 0; i < s_padnames; i++) Log("controller %d: %s", i + 1, s_padname[i]);
}
static int pad_held_now(void)      /* 1-based index of a controller with any button pressed, else 0 */
{
    for (int u = 0; u < 4; u++) { XGP g; if (read_pad(u, &g) && (g.wButtons || g.bLT > 64 || g.bRT > 64)) return u + 1; }
    return 0;
}
#define SND_MOVE 0x2710
#define SND_ENTER 0x2712
static void menu_cycle(int row, int dir)
{
    const int *list = row ? kMenu2 : kMenu1; int n = row ? 4 : 6, i = 0;
    while (i < n && list[i] != s_cfg[row]) i++;
    for (int k = 0; k < n; k++) { i = (i + dir + n) % n; if (src_works(list[i])) break; }   /* skip controllers that are not plugged in */
    s_cfg[row] = list[i]; apply_cfg(); save_ini();
    Log("menu: James %d = %s (James 1 = %s, James 2 = %s)", row + 1, kSrcName[s_cfg[row]], kSrcName[s_asg[0]], kSrcName[s_asg[1]]);
    F_SFX(SND_MOVE, 1.0f, 0);
}
static void menu_text(const char *str, int x, int y, int flags, int r, int g, int b, int a)
{
    char buf[96]; sprintf(buf, "\\h%s", str);            /* \h = the game's proportional (menu) glyphs */
    u32 old = TEXT_FLAGS; TEXT_FLAGS = (old & ~0x300u) | flags;
    F_COLOR(r, g, b, a); F_PRINT(F_PREP(buf), x, y);
    TEXT_FLAGS = old;
}
__attribute__((unused)) static int in_main_options(void) { return OPT_PAGE == 2 && OPT_SUB == 0; }
/* P22: "Co-op Options" lives on the Game Options page (options page 2, sub-page 1), which shows 5 rows (6 when
   0x1DBBF85 is set) in a box made for 10, so it fits in the empty space with the game's own row spacing; the main
   page is the original game again (P21's squeezed layout removed).  Traced on the PC exe:
   - rows are at y 0x64 + 0x1B*i; the cursor routine 0x45FA90 gets the row count for sub-page 1 from
     `add ebx, 5` at 0x45FAC6 (+1 with the flag), wraps with `mov/cmp ax, 5` (0x45FCD3 / 0x45FCD9, flag) or
     `mov/cmp ax, 4` (0x45FCE4 / 0x45FCEA), and selects the row under the mouse when y is in (0x53+0x1B*i, 0x6E+0x1B*i)
     and x in (0x64, 0x1C2) for i < count - so one more row is just these bytes +1 while that page is up;
   - the page's handler (0x464181 branch of 0x462E50) and its draw (0x461CA3 branch of 0x4613A0) act on the selected
     row through switches bounded at 5 (`cmp eax, 5; ja default`); row 5 is the game's own (hidden) 6th row, so while
     our row is selected they run with the selection 99 (= their default: nothing), and it is put back afterwards. */
static int menu_confirm(void) { return F_VIN_EDGE(0x10) || F_VIN_EDGE(0x200) || F_VIN_HELD(0, 0x10, 0) || F_VIN_HELD(0, 0x200, 0); }
#define GAME_OPT_SUB 1
#define SEL_HIDDEN 99
static int in_game_options(void) { return OPT_PAGE == 2 && OPT_SUB == GAME_OPT_SUB; }
static int coop_row(void) { return 5 + (*(u8 *)0x01DBBF85 ? 1 : 0); }     /* right after the game's last row */
static int s_rows_ok;                                                      /* the five cursor bytes are what we expect */
static void set_rows(int extra)
{
    if (!s_rows_ok) return;
    *(u8 *)0x0045FAC8 = extra ? 6 : 5;
    *(u16 *)0x0045FCD5 = extra ? 6 : 5; *(u16 *)0x0045FCDB = extra ? 6 : 5;
    *(u16 *)0x0045FCE6 = extra ? 5 : 4; *(u16 *)0x0045FCEC = extra ? 5 : 4;
}
static void __cdecl cursor_hook(void)
{
    if (!in_game_options() || !s_rows_ok) { F_CURSOR(); return; }
    if (OPT_SEL == SEL_HIDDEN) OPT_SEL = (short)coop_row();
    set_rows(1);
    F_CURSOR();
    set_rows(0);
    if (OPT_SEL == coop_row()) OPT_SEL = SEL_HIDDEN;           /* hidden from the rest of the handler; restored after it */
}
static int coop_row_clicked(void)
{
    int top = 0x53 + 0x1B * coop_row();
    return (MOUSE_CLICK & 1) && MOUSE_Y > top && MOUSE_Y < top + 0x1B && MOUSE_X > 0x64 && MOUSE_X < 0x1C2;
}
/* replaces the call to the options input handler */
static void __cdecl opt_handler_hook(void)
{
    if (!s_coop_page) {
        if (!in_game_options() || !s_rows_ok) { F_OPT_HANDLER(); return; }
        int r = coop_row();
        if (!OPT_POPUP && OPT_BUSY == 0 && ((OPT_SEL == r && menu_confirm()) || coop_row_clicked())) {
            s_coop_page = 1; s_coop_sel = 0; OPT_SEL = (short)r; F_SFX(SND_ENTER, 1.0f, 0); Log("menu: co-op options opened (Game Options row %d)", r);
            s_padnames_tried = 0; load_padnames();                /* refresh the names each time the page opens */
            return;
        }
        F_OPT_HANDLER();
        if (OPT_SEL == SEL_HIDDEN) OPT_SEL = (short)r;
        return;
    }
    if (!in_game_options()) { s_coop_page = 0; return; }
    /* cancel: the game's cancel mask (held, 64-bit form as the game calls it) with our own edge detection, plus Esc / B */
    static int prev_cancel;
    int cancel = F_VIN_HELD(0, MASK_CANCEL, (int)MASK_CANCEL >> 31) || (game_focused() && key(VK_ESCAPE));
    { XGP g; if (read_pad(0, &g) && (g.wButtons & XB_B)) cancel = 1; }
    int cancel_hit = cancel && !prev_cancel; prev_cancel = cancel;
    if (s_coop_page == 2) {                                   /* CE: the Appearance page */
        if (cancel_hit) { s_coop_page = 1; F_SFX(SND_ENTER, 1.0f, 0); Log("menu: appearance closed"); return; }
        int up = F_VIN_HELD(0, 0x4000001, 0) || F_VIN_EDGE(1), down = F_VIN_HELD(0, 0x8000002, 0) || F_VIN_EDGE(2);
        int left = F_VIN_HELD(0, 0x2000004, 0) || F_VIN_EDGE(4), right = F_VIN_HELD(0, 0x1000008, 0) || F_VIN_EDGE(8) || menu_confirm();
        int nrows = (s_chr_sel[s_app_who] == CHR_MARIA ? CHR_MPARTS : CE_PARTS) + 2;
        if (s_app_sel >= nrows) s_app_sel = nrows - 1;
        if (down) { s_app_sel = (s_app_sel + 1) % nrows; F_SFX(SND_MOVE, 1.0f, 0); }
        else if (up) { s_app_sel = (s_app_sel + nrows - 1) % nrows; F_SFX(SND_MOVE, 1.0f, 0); }
        else if (left || right) {
            if (s_app_sel == 0) { s_app_who ^= 1; F_SFX(SND_MOVE, 1.0f, 0); }
            else if (s_app_sel == 1) {                            /* CS: character */
                int k = s_chr_sel[s_app_who];
                for (int t = 0; t < CHR_COUNT; t++) { k = (k + (left ? CHR_COUNT - 1 : 1)) % CHR_COUNT; if (k == CHR_JAMES || chr_ready(k)) break; }
                s_chr_sel[s_app_who] = k;
                char key[32]; snprintf(key, sizeof key, "james%d_character", s_app_who + 1);
                if (s_ini[0]) WritePrivateProfileStringA("look", key, chr_name(k), s_ini);
                Log("characters: James %d plays as %s", s_app_who + 1, chr_name(k));
                F_SFX(SND_MOVE, 1.0f, 0);
            }
            else if (s_chr_sel[s_app_who] == CHR_JAMES) { ce_cycle(s_app_who, s_app_sel - 2, left ? -1 : +1); F_SFX(SND_MOVE, 1.0f, 0); }
            else if (s_chr_sel[s_app_who] == CHR_MARIA && s_app_sel - 2 < CHR_MPARTS) { chr_mcolor_cycle(s_app_who, s_app_sel - 2, left ? -1 : +1); F_SFX(SND_MOVE, 1.0f, 0); }
        }
        return;
    }
    if (cancel_hit) { s_coop_page = 0; F_SFX(SND_ENTER, 1.0f, 0); Log("menu: co-op options closed"); return; }
    /* rows: 0 James 1, 1 James 2, 2 Split screen (P33), 3 Appearance (CE) */
    if (F_VIN_HELD(0, 0x8000002, 0) || F_VIN_EDGE(2)) { s_coop_sel = (s_coop_sel + 1) % 4; F_SFX(SND_MOVE, 1.0f, 0); }
    else if (F_VIN_HELD(0, 0x4000001, 0) || F_VIN_EDGE(1)) { s_coop_sel = (s_coop_sel + 3) % 4; F_SFX(SND_MOVE, 1.0f, 0); }
    else if (s_coop_sel == 3 && (menu_confirm() || F_VIN_HELD(0, 0x1000008, 0) || F_VIN_EDGE(8))) {
        s_coop_page = 2; s_app_sel = 1; s_app_yaw = 3.14159265f; F_SFX(SND_ENTER, 1.0f, 0); Log("menu: appearance opened (preview %s)", ce_preview_status());
    }
    else if (F_VIN_HELD(0, 0x2000004, 0) || F_VIN_EDGE(4)) { if (s_coop_sel == 2) { split_cycle(-1, "menu"); F_SFX(SND_MOVE, 1.0f, 0); } else if (s_coop_sel < 2) menu_cycle(s_coop_sel, -1); }
    else if (F_VIN_HELD(0, 0x1000008, 0) || F_VIN_EDGE(8) || menu_confirm()) { if (s_coop_sel == 2) { split_cycle(+1, "menu"); F_SFX(SND_MOVE, 1.0f, 0); } else if (s_coop_sel < 2) menu_cycle(s_coop_sel, +1); }
}
/* CE: the Appearance page - rows on the left, the James being edited turning slowly on the right */
static int s_app_px, s_app_py, s_app_pw, s_app_ph, s_app_pending;
typedef HRESULT (STDMETHODCALLTYPE *EndSceneFn)(IDirect3DDevice8 *);
static EndSceneFn s_endscene_orig;
static HRESULT STDMETHODCALLTYPE endscene_hook(IDirect3DDevice8 *dev)
{
    if (s_app_pending) { s_app_pending = 0;
        if (s_chr_sel[s_app_who] != CHR_JAMES) chr_draw_preview(dev, s_chr_sel[s_app_who], s_app_who, s_app_px, s_app_py, s_app_pw, s_app_ph, s_app_yaw);
        else ce_preview_draw(dev, s_app_who, s_app_px, s_app_py, s_app_pw, s_app_ph, s_app_yaw); }
    return s_endscene_orig(dev);
}
static void hook_endscene(void)
{
    static int done; IDirect3DDevice8 *dev = D3D_DEVICE;
    if (done || !dev) return;
    done = 1;
    void **vt = *(void ***)dev; int slot = offsetof(IDirect3DDevice8Vtbl, EndScene) / sizeof(void *);
    DWORD old;
    if (!VirtualProtect(&vt[slot], sizeof(void *), PAGE_EXECUTE_READWRITE, &old)) { Log("look preview: EndScene hook failed"); return; }
    s_endscene_orig = (EndSceneFn)vt[slot]; vt[slot] = (void *)endscene_hook;
    VirtualProtect(&vt[slot], sizeof(void *), old, &old);
    Log("look preview: drawing at EndScene (slot %d)", slot);
}
static void appearance_draw(void)
{
    #define AP_LABEL_X 0x9C
    #define AP_VALUE_X 0xB9
    menu_text("APPEARANCE", 0x100, 0x34, 0x100, 0x3F, 0x3F, 0x3F, 0xFF);
    for (int row = 0; row <= CE_PARTS + 1; row++) {
        int ch = s_chr_sel[s_app_who], other = ch != CHR_JAMES, mar = ch == CHR_MARIA;
        if (mar && row - 2 >= CHR_MPARTS) continue;                       /* Maria: four rows (top, skirt, boots, hair) */
        const char *label = row == 0 ? "Editing" : row == 1 ? "Character" : mar ? chr_mpart_name[row - 2] : ce_part_name[row - 2];
        const char *val = row == 0 ? (s_app_who ? "James 2" : "James 1") : row == 1 ? chr_name(ch)
                        : mar ? ce_palette[chr_mcolor[s_app_who][row - 2]].name : other ? "-" : ce_palette[ce_color[s_app_who][row - 2]].name;
        int y = 0x64 + 0x1B * (row + 1), sel = s_app_sel == row;
        menu_text(label, AP_LABEL_X, y, 0x200, 0x3F, 0x3F, 0x3F, 0x60);
        if (sel) menu_text(label, AP_LABEL_X - 2, y, 0x200, 0x3F, 0x3F, 0x3F, 0xFF);
        menu_text(val, AP_VALUE_X, y, 0, 0x3F, 0x3F, 0x3F, sel ? 0xFF : 0x60);
        if (sel) menu_text(val, AP_VALUE_X - 2, y, 0, 0x3F, 0x3F, 0x3F, 0xFF);
    }
    const char *help = s_app_sel == 0 ? "Left / right: which James to dress" : s_app_sel == 1 ? "Play as James, Maria, Eddie or Angela"
                     : s_chr_sel[s_app_who] == CHR_MARIA || s_chr_sel[s_app_who] == CHR_JAMES ? "Left / right: next colour (saved at once)" : "Colours are for James and Maria only";
    if (!ce_preview_load(s_dir)) help = "No preview: James's model file was not found";
    menu_text(help, 0x46, 0x18B, 0, 0x3F, 0x3F, 0x3F, 0xFF);
    /* the preview: the menu lays out in 640x480 units over the 4:3 middle of the screen; the model gets the right third */
    IDirect3DDevice8 *dev = D3D_DEVICE;
    if (!dev) return;
    int W = *(int *)0xA33480, H = *(int *)0xA33484;
    IDirect3DSurface8 *rt = NULL; D3DSURFACE_DESC rd;
    if (dev->lpVtbl->GetRenderTarget(dev, &rt) == D3D_OK && rt) { if (rt->lpVtbl->GetDesc(rt, &rd) == D3D_OK) { W = (int)rd.Width; H = (int)rd.Height; } rt->lpVtbl->Release(rt); }
    float u = H / 480.0f; int x0 = (int)((W - 640.0f * u) * 0.5f);
    int px = x0 + (int)(APP_PREVIEW_X * u), py = (int)(APP_PREVIEW_Y * u), pw = (int)(APP_PREVIEW_W * u), ph = (int)(APP_PREVIEW_H * u);
    if (px < 0) px = 0; if (py < 0) py = 0; if (px + pw > W) pw = W - px; if (py + ph > H) ph = H - py;
    s_app_yaw += 0.012f; if (s_app_yaw > 6.2831853f) s_app_yaw -= 6.2831853f;
    /* drawn at EndScene (after the menu's own 2D records, which would otherwise paint over him) */
    hook_endscene();
    s_app_px = px; s_app_py = py; s_app_pw = pw; s_app_ph = ph; s_app_pending = 1;
}
/* replaces the call to the options page drawing */
static void __cdecl opt_draw_hook(void)
{
    if (!s_coop_page || !in_game_options()) {
        int ours = in_game_options() && s_rows_ok, r = coop_row(), sel = ours && OPT_SEL == r;
        if (sel) OPT_SEL = SEL_HIDDEN;                            /* the game draws its rows with none of them selected */
        F_OPT_DRAW();
        if (sel) OPT_SEL = (short)r;
        if (ours) {                                               /* our row, in the game's own row style */
            int y = 0x64 + 0x1B * r;
            menu_text("Co-op Options", 0x100, y, 0x100, 0x3F, 0x3F, 0x3F, 0x60);
            if (sel) menu_text("Co-op Options", 0xFE, y, 0x100, 0x3F, 0x3F, 0x3F, 0xFF);
            if (sel) menu_text("Options for James 1 / James 2 controls", 0x46, 0x18B, 0, 0x3F, 0x3F, 0x3F, 0xFF);
        }
        return;
    }
    /* our page (P23 layout, measured on the P22 screenshot: the menu is 512 units wide, the game's box spans about
       x 14..493, menu text runs ~9.5 units a character, ~10.2 when selected). Columns moved left (labels end at
       0xB4, values start at 0xD1, the game's own 29-unit gap) so the longest value, "Keyboard + Controller 2"
       (~234 units), ends near x 443; controller names are shortened to 24 characters; help lines are kept under
       40 characters (~385 units from x 0x46). */
    #define CO_LABEL_X 0xB4
    #define CO_VALUE_X 0xD1
    if (s_coop_page == 2) { appearance_draw(); return; }
    menu_text("CO-OP OPTIONS", 0x100, 0x34, 0x100, 0x3F, 0x3F, 0x3F, 0xFF);
    for (int row = 0; row < 2; row++) {
        char label[16]; sprintf(label, "James %d", row + 1);
        int y = 0x64 + 0x1B * (row + 1), sel = s_coop_sel == row;
        menu_text(label, CO_LABEL_X, y, 0x200, 0x3F, 0x3F, 0x3F, 0x60);
        if (sel) menu_text(label, CO_LABEL_X - 2, y, 0x200, 0x3F, 0x3F, 0x3F, 0xFF);
        const char *val = kMenuName[s_cfg[row]];
        menu_text(val, CO_VALUE_X, y, 0, 0x3F, 0x3F, 0x3F, sel ? 0xFF : 0x60);
        if (sel) menu_text(val, CO_VALUE_X - 2, y, 0, 0x3F, 0x3F, 0x3F, 0xFF);
    }
    {   /* P33: split screen row (row 3, between the James rows and the controller rows) */
        static const char *kSplitMenu[SPLIT_MODES] = { "Off (shared screen)", "Side by side", "Top and bottom" };
        int y = 0x64 + 0x1B * 3, sel = s_coop_sel == 2;
        menu_text("Split screen", CO_LABEL_X, y, 0x200, 0x3F, 0x3F, 0x3F, 0x60);
        if (sel) menu_text("Split screen", CO_LABEL_X - 2, y, 0x200, 0x3F, 0x3F, 0x3F, 0xFF);
        menu_text(kSplitMenu[s_split_cfg], CO_VALUE_X, y, 0, 0x3F, 0x3F, 0x3F, sel ? 0xFF : 0x60);
        if (sel) menu_text(kSplitMenu[s_split_cfg], CO_VALUE_X - 2, y, 0, 0x3F, 0x3F, 0x3F, 0xFF);
    }
    {   /* CE: Appearance row (row 4) */
        int y = 0x64 + 0x1B * 4, sel = s_coop_sel == 3;
        menu_text("Appearance", CO_LABEL_X, y, 0x200, 0x3F, 0x3F, 0x3F, 0x60);
        if (sel) menu_text("Appearance", CO_LABEL_X - 2, y, 0x200, 0x3F, 0x3F, 0x3F, 0xFF);
        menu_text("Clothes and hair colours", CO_VALUE_X, y, 0, 0x3F, 0x3F, 0x3F, sel ? 0xFF : 0x60);
        if (sel) menu_text("Clothes and hair colours", CO_VALUE_X - 2, y, 0, 0x3F, 0x3F, 0x3F, 0xFF);
    }
    /* which physical controller is which: holding any button lights its row up */
    int held = pad_held_now();
    { static DWORD last; DWORD t = GetTickCount(); if (t - last > 2000) { last = t; s_padnames_tried = 0; load_padnames(); } }
    for (int i = 0; i < 2; i++) {
        char label[24], value[40]; sprintf(label, "Controller %d", i + 1);
        int y = 0x64 + 0x1B * (i + 5), on = held == i + 1;
        XGP g; int present = read_pad(i, &g);
        if (i < s_padnames) {
            const char *n = s_padname[i];
            char tmp[64]; snprintf(tmp, sizeof tmp, "%s", n);
            if (!strncmp(tmp, "Controller (", 12)) { memmove(tmp, tmp + 12, strlen(tmp + 12) + 1); char *e = strrchr(tmp, ')'); if (e) *e = 0; }
            if (strlen(tmp) > 24) { tmp[22] = '.'; tmp[23] = '.'; tmp[24] = 0; }
            snprintf(value, sizeof value, "%s", tmp);
        } else strcpy(value, present ? "Connected" : "Not connected");
        menu_text(label, CO_LABEL_X, y, 0x200, 0x3F, 0x3F, 0x3F, on ? 0xFF : 0x60);
        menu_text(value, CO_VALUE_X, y, 0, 0x3F, 0x3F, 0x3F, on ? 0xFF : 0x60);
    }
    const char *help;
    if (s_coop_sel == 3) help = "Press Enter: colours for James 1 and James 2";
    else if (s_coop_sel == 2) help = s_split_cfg == SPLIT_OFF ? "One screen for both, James 1's camera" : "F8 or L3+R3 in game also switch it";
    else if (!src_works(s_asg[s_coop_sel])) help = s_coop_sel ? "That controller is not plugged in" : "Unplugged: James 1 uses the keyboard";
    else if (held) help = "Lit row = the controller you hold";
    else help = s_coop_sel ? "Auto: whoever presses Join is James 2" : "Hold a button to find a controller";
    menu_text(help, 0x46, 0x18B, 0, 0x3F, 0x3F, 0x3F, 0xFF);
}

/* ===================== James 2 ===================== */
#define LEASH_R    6400.0f
#define LEASH_HARD 12000.0f
static void put_on_circle(u8 *c, u8 *centre)
{
    float dx = POSP(c)[0] - POSP(centre)[0], dz = POSP(c)[2] - POSP(centre)[2];
    float d = sqrtf(dx * dx + dz * dz);
    if (d < 1.0f) return;
    float k = LEASH_R / d;
    float p[4] = { POSP(centre)[0] + dx * k, POSP(c)[1], POSP(centre)[2] + dz * k, 1.0f };
    F_TELEPORT(c, p, YAW(c));
}
/* areas where only James 1 may start or reach anything (area numbers verified on the PC exe, 0x494EC0 table:
   38 = lake / rowboat (only area whose scripts use the boat model 0x10B), 44 = Nightmare Hotel finale (Pyramid Heads
   0x209, long-hallway recording and the final door that scores the ending), 47 = Angela's burning stairs,
   48-51 = the Mary / Maria arenas (model 0x206)). P26: in these areas James 2 is also never allowed to MOVE James 1 -
   the room rule and the tether always bring James 2 to James 1 instead, so James 2 running ahead can't carry
   James 1 into a story trigger (e.g. leave the long hallway early, which changes the ending). */
static const u8 kP1OnlyStages[] = { 38, 44, 47, 48, 49, 50, 51 };
static int p1_only_area(void) { for (unsigned i = 0; i < sizeof kP1OnlyStages; i++) if (STAGE_IDX == kP1OnlyStages[i]) return 1; return 0; }
static void tether_both(void)
{
    u8 *p1 = PLAYER;
    if (!s_active || !s_p2c || !p1_free(p1) || s_follow_timer > 0 || s_spawn_grace > 0 || room_of(p1) != room_of(s_p2c)) { s_t_ok = 0; return; }
    float d2 = dist2(POSP(p1), POSP(s_p2c));
    if (s_t_ok && d2 > LEASH_R * LEASH_R && d2 < LEASH_HARD * LEASH_HARD) {
        if (!p1_only_area() && dist2(POSP(p1), s_t2_old) > LEASH_R * LEASH_R) put_on_circle(p1, s_p2c);   /* James 1 walked out */
        if (dist2(POSP(s_p2c), POSP(p1)) > LEASH_R * LEASH_R) put_on_circle(s_p2c, p1);    /* James 2 (still) out */
    }
    memcpy(s_t1_old, POSP(p1), 12); memcpy(s_t2_old, POSP(s_p2c), 12); s_t_ok = 1;
}

/* ===================== James 2's weapon: his own copy of James 1's weapon work =====================
 * The game never stores a pointer to the in-hand weapon: everything looks it up as "the first work with model
 * 0x800 + equipped weapon id" (and 0x820 + id for the alternate James model).  James 2 gets his OWN copies (same
 * models, instance 1) kept right after James 1's in the list so James 1's lookups still find his; while James 2's
 * own code runs (think, post pass, attack volumes) James 1's weapon hides behind model id 0x0FFF so the lookups find
 * the copy.  The copies follow James 1's weapon: changed / removed / re-made as needed. */
static int is_weapon_copy(u8 *w) { return w && is_in_list(w) && (MODEL(w) & 0xFF00) == 0x800 && *(u16 *)(w + W_INST) == 1; }
static void w2_unequip(void)
{
    if (!s_active || !s_p2c || s_p2.block[BLK_WEAPON] == 0) return;
    save_state(&s_p1, 0); load_state(&s_p2, 0);
    F_SET_WEAPON(0);
    save_state(&s_p2, 0); load_state(&s_p1, 0);
}
static void w2_drop(const char *why)
{
    int had = 0;
    for (int k = 0; k < 2; k++) { if (is_weapon_copy(s_w2[k])) { F_REMOVE(s_w2[k], 0, 0); had = 1; } s_w2[k] = NULL; }
    if (had) Log("James 2's weapon copy removed (%s)", why);
    w2_unequip();
}
static void relink_after(u8 *c, u8 *after)
{
    if (c == after || *(u8 **)(c + W_PREV) == after) return;
    u8 *prev = *(u8 **)(c + W_PREV), *next = *(u8 **)(c + W_NEXT);
    if (prev) *(u8 **)(prev + W_NEXT) = next; else LIST_HEAD = next;
    if (next) *(u8 **)(next + W_PREV) = prev;
    *(u8 **)(c + W_PREV) = after; *(u8 **)(c + W_NEXT) = *(u8 **)(after + W_NEXT);
    if (*(u8 **)(after + W_NEXT)) *(u8 **)(*(u8 **)(after + W_NEXT) + W_PREV) = c;
    *(u8 **)(after + W_NEXT) = c;
}
/* hide James 1's weapon works (instance 0) behind model 0x0FFF; returns how many, with their ids in keep[] */
static int w1_hide(u8 **w1, short *keep)
{
    int n = 0;
    for (int k = 0; k < 2; k++) {
        w1[k] = NULL;
        if (!s_w2[k] || !is_in_list(s_w2[k])) continue;
        u8 *w = F_FIND(MODEL(s_w2[k]), 0);
        if (!w || w == s_w2[k]) continue;
        keep[k] = MODEL(w); MODEL(w) = 0x0FFF; w1[k] = w; n++;
    }
    return n;
}
static void w1_show(u8 **w1, const short *keep) { for (int k = 0; k < 2; k++) if (w1[k]) MODEL(w1[k]) = keep[k]; }
static void p2_remove(const char *why);
static void bring_p2(u8 *p1, const char *why);
static int node_known(u8 *p);
static int is_private_node(u8 *p);
/* P29 private bone slots (see "James 2's bones never come from the game's pool" below) */
#define PB_J2_N     64            /* nodes per James 2 slot (James needs 41) */
#define PB_J2_SLOTS 2
#define PB_W_N      8             /* nodes per weapon-copy slot (a weapon needs 2) */
#define PB_W_SLOTS  4
#define PRIV_J2     1
#define PRIV_WPN    2
static u8 s_pbj[PB_J2_SLOTS][PB_J2_N * NODE_SZ] __attribute__((aligned(64)));
static u8 s_pbw[PB_W_SLOTS][PB_W_N * NODE_SZ] __attribute__((aligned(64)));
static u8 *s_pbj_own[PB_J2_SLOTS], *s_pbw_own[PB_W_SLOTS];   /* work holding the slot, NULL = free */
static int s_priv_ok;         /* both call sites hooked */
static int s_priv_want;       /* the next alloc is for: PRIV_J2 / PRIV_WPN (set around the mod's own spawns) */
static int s_priv_hit;        /* the last mod spawn got a private slot */
static int pool_prepare(int need, const char *who);
static int list_is_block(u8 *head, int need);
static void w2_sync(u8 *p1)
{
    if (!s_active || !s_p2c || !p1 || !s_opt_weapon) { if (s_w2[0] || s_w2[1]) w2_drop("co-op off"); return; }
    int wid = WID1;
    int want[2] = { wid ? 0x800 + wid : 0, (wid && ALT_MODEL) ? 0x820 + wid : 0 };
    for (int k = 0; k < 2; k++) {
        if (s_w2[k] && !is_in_list(s_w2[k])) { s_w2[k] = NULL; Log("James 2's weapon copy %d was freed by the game (room / stage load)", k); }
        if (s_w2[k] && (MODEL(s_w2[k]) != want[k] || *(u16 *)(s_w2[k] + W_INST) != 1)) { w2_drop("James 1 changed weapon"); break; }
    }
    if (wid == 0) { if (s_w2[0] || s_w2[1]) w2_drop("James 1 unarmed"); else w2_unequip(); return; }
    if (s_w2_cool > 0) { s_w2_cool--; return; }
    for (int k = 0; k < 2; k++) {
        if (!want[k]) continue;
        u8 *w1 = F_FIND(want[k], 0);
        if (!w1) continue;                                       /* James 1's weapon work does not exist (yet) */
        if (!s_w2[k]) {
            if ((!p1_free(p1) && !p1_only_held(p1)) || s_spawn_grace > 0) return;
            if (CHARA_CNT >= 30) { s_w2_cool = 60; Log("weapon copy: no free character slot (characters %d)", CHARA_CNT); return; }
            int wneed = F_BONES_NEEDED(want[k]), wpriv = s_priv_ok && s_opt_privbones && wneed > 0 && wneed <= PB_W_N;
            if (!wpriv && pool_prepare(wneed, "weapon copy") < 0) { s_w2_cool = 60; return; }
            u8 *owner = LIST_OWNER;
            save_state(&s_p1, 0); load_state(&s_p2, 0);           /* the weapon init equips it on the CURRENT player block: James 2's */
            s_priv_hit = 0; s_priv_want = wpriv ? PRIV_WPN : 0;
            s_own_spawn = 1; F_SPAWN_EV(want[k], 1, POSP(s_p2c), POSP(s_p2c) + 4, 0); s_own_spawn = 0;
            s_priv_want = 0;
            s_w2[k] = F_FIND(want[k], 1);
            save_state(&s_p2, 0); load_state(&s_p1, 0);
            LIST_OWNER = owner;
            if (PLAYER != p1) PLAYER = p1;
            if (!s_w2[k]) { s_w2_cool = 60; Log("weapon copy: spawn of model 0x%x refused", want[k]); return; }
            Log("James 2's weapon copy made: model 0x%x (James 1's weapon %d, bones in %s)%s", want[k], wid, s_priv_hit ? "the mod's own slot" : "the game's pool", p1_free(p1) ? "" : " - while James 1 is held");
            /* safety: a copy re-made over and over means something keeps removing it; each make re-inits James 2's
               pose, so stop for 10 s instead of thrashing him */
            { static DWORD t0; static int n; DWORD t = GetTickCount();
              if (t - t0 > 5000) { t0 = t; n = 0; }
              if (++n > 4) { s_w2_cool = 600; n = 0; Log("weapon copy: re-made too often, pausing 10 s"); } }
        }
        relink_after(s_w2[k], w1);                                /* James 1's weapon stays the first match */
        if (FLAGS(w1) & F_DRAWN) FLAGS(s_w2[k]) |= F_DRAWN; else FLAGS(s_w2[k]) &= ~F_DRAWN;
    }
}
/* put a copy in James 2's hand: hand matrix = his bone 0x23 (0x21 for the Maria model), world matrix = his (= 0x545EF0 with James 2 as owner) */
static void w2_attach(u8 *j2)
{
    int cnt = MODEL(j2) == 0x10E ? 0x21 : 0x23;
    u8 *n = *(u8 **)(j2 + 0x9C);
    for (int i = 0; i < cnt && n; i++) n = *(u8 **)n;
    if (!n) return;
    for (int k = 0; k < 2; k++) {
        u8 *w = s_w2[k];
        if (!w || !is_in_list(w)) continue;
        u8 *wb = *(u8 **)(w + 0x9C);
        if (!wb) continue;
        memcpy(wb + 8, n + 8, 0x40);
        memcpy(wb + 0x48, n + 8 + 0x30, 0x10);
        memcpy(w + 0x5C, j2 + 0x5C, 0x40);
        memcpy(w + W_POS, j2 + 0x8C, 0x10);
    }
}
static void post_one(u8 *c, u32 f)
{
    if (*(u32 *)(c + 0x18)) F_POST_A(c);
    else if (f & F_CONTROL) F_POST_PASS(c);
    else F_POST_B(c);
}
/* The flashlight lens flare / glow (option 0xA33400) lives in ONE table (0xAC6B68..0xAC6C5F): the post pass
   (0x5386F5 / 0x53871C / 0x5387ED -> 0x512160) writes it for "the player", and 0x511F60 (called from 0x476752) draws it
   and keeps its fade (0xAC6BE0) and occlusion test. P29 kept the table on James 1 (James 2's pass had taken it over).
   P30: James 2 has his own copy of the table - swapped in around his post pass, and drawn by a second call of
   0x511F60 right after James 1's with James 2's player state in (0x5103F0 reads the player's flags and P+0x28A). The
   on/off switches (0x511F30 / 0x511F50: byte +0 of each entry) only ever touch the live table, so they are copied over. */
#define FLARE_TAB 0x00AC6B68u
#define FLARE_SZ  0xF8
static u8 s_flare_j1[FLARE_SZ], s_flare_j2[FLARE_SZ];
static int s_flare_j2_ok, s_flare_hook_ok;
static DWORD s_flare_j2_draws;
static void flare_sync_ctrl(void)
{
    *(u8 *)0x00AC6BD8 = s_flare_j1[0x00AC6BD8 - FLARE_TAB];
    *(u8 *)0x00AC6C14 = s_flare_j1[0x00AC6C14 - FLARE_TAB];
}
static void p2_post(u8 *c, u32 f)
{
    u8 *w1[2]; short keep[2] = { 0, 0 };
    s_phase = "James 2 post pass";
    memcpy(s_flare_j1, (void *)FLARE_TAB, FLARE_SZ);
    if (s_flare_j2_ok) { memcpy((void *)FLARE_TAB, s_flare_j2, FLARE_SZ); flare_sync_ctrl(); }
    save_state(&s_p1, 0); load_state(&s_p2, 0);
    w1_hide(w1, keep);
    post_one(c, f);
    w1_show(w1, keep);
    save_state(&s_p2, 0); load_state(&s_p1, 0);
    memcpy(s_flare_j2, (void *)FLARE_TAB, FLARE_SZ); s_flare_j2_ok = 1;
    memcpy((void *)FLARE_TAB, s_flare_j1, FLARE_SZ);
    w2_attach(c);
    s_phase = "game (outside the co-op frame hook)";
}

/* ---- diagnostics for the "James 2 sinks / spikes" glitch: runs after the post pass every frame ---- */
static int node_in_list(u8 *list, u8 *node) { int n = 0; for (u8 *p = list; p && n < 80; p = *(u8 **)p, n++) if (p == node) return 1; return 0; }
static void glitch_check(void)
{
    static int quiet;
    if (quiet > 0) { quiet--; return; }
    u8 *p1 = PLAYER;
    if (!s_active || !s_p2c || !p1 || !is_in_list(s_p2c)) return;
    if (*(u8 *)0x01FB8117) return;                   /* P33k: James 1's root drops to the floor while he dies - not a glitch */
    u8 *b1 = *(u8 **)(p1 + 0x220), *b2 = *(u8 **)(s_p2c + 0x220);
    if (!b1 || !b2) return;
    /* P29: the root bone's translation is relative to the character (P27 log: James 1 standing at y -1800 has root
       -559), so compare the two roots directly. P13-P28 subtracted the floor height, which flagged James 2 on stairs. */
    float d1 = *(float *)(b1 + 8 + 0x34), d2 = *(float *)(b2 + 8 + 0x34);
    /* shared bone nodes between James 2 and any other character? */
    u8 *shared = NULL; int n = 0;
    for (u8 *w = LIST_HEAD; w && n < 64; w = *(u8 **)(w + W_NEXT), n++) {
        if (w == s_p2c) continue;
        u8 *wl = *(u8 **)(w + 0x220); int m = 0;
        for (u8 *p = wl; p && m < 80 && node_known(p); p = *(u8 **)p, m++) if (node_in_list(b2, p)) { shared = w; break; }
        if (shared) break;
    }
    if (fabsf(d2 - d1) < 300.0f && !shared && *(u8 **)(s_p2c + 0x2A4) == *(u8 **)(p1 + 0x2A4)) return;
    Log("GLITCH: J2 root y %.0f (J1 %.0f) | J2 model %p/%p bones %p/%p | J1 model %p/%p bones %p/%p | lower %d upper %d wpn %d | shares bone nodes with %p (model 0x%x) | copies %p %p",
        d2, d1, *(void **)(s_p2c + 0x2A4), *(void **)(s_p2c + 0x2B0), b2, *(void **)(s_p2c + 0x9C),
        *(void **)(p1 + 0x2A4), *(void **)(p1 + 0x2B0), b1, *(void **)(p1 + 0x9C),
        (int)(signed char)s_p2.block[0x64], (int)(signed char)s_p2.block[0x68], s_p2.block[BLK_WEAPON],
        shared, shared ? MODEL(shared) : 0, s_w2[0], s_w2[1]);
    { int need = F_BONES_NEEDED(MODEL(s_p2c));
      Log("  J2 bones one block: %s | J1: %s", list_is_block(b2, need) ? "yes" : "NO", list_is_block(b1, need) ? "yes" : "NO"); }
    quiet = 30;
    static int dumps;
    if (dumps < 2) {                                    /* compare the two James works dword by dword */
        dumps++;
        Log("DUMP %d: dwords that differ between James 1 (%p) and James 2 (%p) works:", dumps, p1, s_p2c);
        char line[256]; int len = 0;
        for (int o = 0; o < 0x2CC; o += 4) {
            u32 a = *(u32 *)(p1 + o), b = *(u32 *)(s_p2c + o);
            if (a == b) continue;
            len += sprintf(line + len, " +%03x:%08x/%08x", o, a, b);
            if (len > 200) { Log("%s", line); len = 0; }
        }
        if (len) Log("%s", line);
        for (int t = 0; t < 2; t++) {
            u32 off = t ? 0x1A0 : 0x220;
            u8 *n1 = *(u8 **)(p1 + off), *n2 = *(u8 **)(s_p2c + off);
            for (int k = 0; k < 3 && n1 && n2; k++, n1 = *(u8 **)n1, n2 = *(u8 **)n2) {
                float *m1 = (float *)(n1 + 8), *m2 = (float *)(n2 + 8);
                Log("  list +%03x node %d: J1 t(%.0f %.0f %.0f) J2 t(%.0f %.0f %.0f) | J1 +4 %08x J2 +4 %08x", off, k,
                    m1[12], m1[13], m1[14], m2[12], m2[13], m2[14], *(u32 *)(n1 + 4), *(u32 *)(n2 + 4));
            }
        }
        u8 *a1 = *(u8 **)(p1 + 0x2A0), *a2 = *(u8 **)(s_p2c + 0x2A0);
        if (a1 && a2) { len = 0; for (int o = 0; o < 0x100; o += 4) { u32 a = *(u32 *)(a1 + o), b = *(u32 *)(a2 + o); if (a != b) { len += sprintf(line + len, " +%02x:%08x/%08x", o, a, b); if (len > 200) { Log("  anim+%s", line); len = 0; } } } if (len) Log("  anim+%s", line); }
    }
}
/* P29 watchdog: James 2's skeleton must stay ONE block of nodes that no other character uses. Checked twice a second
   in free play; if it is ever wrong he is taken out and rejoins at James 1 a moment later (at most 3 times a minute). */
static void p2_bones_watch(void)
{
    static int tick, heals; static DWORD t0;
    if (++tick < 30) return;
    tick = 0;
    u8 *p1 = PLAYER;
    if (!s_active || !s_p2c || !p1 || !is_in_list(s_p2c) || !p1_free(p1)) return;
    const char *why = NULL;
    u8 *b = *(u8 **)(s_p2c + 0x220);
    int need = F_BONES_NEEDED(MODEL(s_p2c));
    if (!b || b != *(u8 **)(s_p2c + 0x9C)) why = "his bone list pointers disagree";
    else if (need <= 0 || !list_is_block(b, need)) why = "his bones are not one block";
    else if (*(u8 **)(b + (need - 1) * NODE_SZ) != NULL) why = "his bone list runs past its end";
    else {
        int n = 0;
        for (u8 *w = LIST_HEAD; w && n < 64 && !why; w = *(u8 **)(w + W_NEXT), n++) {
            if (w == s_p2c) continue;
            int m = 0;
            for (u8 *p = *(u8 **)(w + 0x220); p && m < 128 && node_known(p); p = *(u8 **)p, m++)
                if (p >= b && p < b + need * NODE_SZ) { why = "another character uses his bone nodes"; break; }
        }
    }
    if (!why) return;
    DWORD t = GetTickCount();
    if (t - t0 > 60000) { t0 = t; heals = 0; }
    Log("SELF-HEAL: James 2's skeleton is broken (%s; bones %p, need %d, %s) - taking him out, he rejoins at James 1",
        why, b, need, b && is_private_node(b) ? "mod's own slot" : "game's pool");
    p2_remove("self-heal: broken skeleton");
    if (++heals > 3) { s_want = 0; s_follow_timer = 0; sfx(SND_FAILED); Log("SELF-HEAL: 4 times in a minute - James 2 stays out until join is pressed"); }
    else s_follow_timer = 15;
}
/* P29 fall guard. Outside scripts (where James 2 is taken out anyway) a James only moves up or down on stairs and
   slopes - he cannot jump or fall. So a drop of more than 3.5 m within one second, or being more than 12 m below
   James 1, can only mean James 2 went through the floor: he is put back on James 1's spot. */
static float s_y2_hist[64]; static int s_y2_n; static unsigned s_y2_f;
static void fall_guard(u8 *p1)
{
    if (!s_active || !s_p2c || !p1 || !p1_free(p1) || s_spawn_grace > 0 || s_follow_timer > 0 || !is_in_list(s_p2c)) { s_y2_n = 0; return; }
    s_y2_hist[s_y2_f++ & 63] = POSP(s_p2c)[1];
    if (s_y2_n < 64) s_y2_n++;
    if (s_y2_n < 60) return;
    float drop = POSP(s_p2c)[1] - s_y2_hist[(s_y2_f - 60) & 63], below = POSP(s_p2c)[1] - POSP(p1)[1];
    if (drop > 3500.0f || below > 12000.0f) {
        Log("FALL GUARD: James 2 dropped %.0f in a second and is %.0f below James 1 - put back on James 1", drop, below);
        bring_p2(p1, "fell through the floor"); sfx(SND_TELEPORT); s_y2_n = 0;
    }
}
/* James 2 is dropping fast right now (used by the room rule: never pull James 1 to a falling James 2) */
static int p2_unsteady(u8 *p1)
{
    if (!s_p2c || s_y2_n < 30) return 0;
    return POSP(s_p2c)[1] - s_y2_hist[(s_y2_f - 30) & 63] > 1500.0f || POSP(s_p2c)[1] - POSP(p1)[1] > 12000.0f;
}
/* replaces the game's post-update loop 0x538B40 (same loop; James 2's pass runs on his state) */
static void __cdecl post_update_all_hook(void)
{
    int n = 0;
    for (u8 *c = LIST_HEAD; c; c = *(u8 **)(c + W_NEXT)) {
        if (++n > 64) { Log("character list cycle in the post pass"); break; }
        u32 f = FLAGS(c);
        if (f & 0x100) continue;
        if (!(f & F_DRAWN) && !(f & 0x40000)) continue;
        f &= ~0x40000u; FLAGS(c) = f;
        if (s_active && c == s_p2c && c != PLAYER) p2_post(c, f); else post_one(c, f);
    }
    DEMO_FLAGS &= ~0x40u;
    glitch_check();
}
/* replaces the attack-volume step: James 2's queued attacks are built separately, with his state and his weapon copy */
static void __cdecl attack_volumes_hook(void)
{
    int n = 20 - ATK_FREE, n2 = 0;
    if (!s_active || !s_p2c || n <= 0 || n > 20) { F_BUILD_ATK(); return; }
    for (int i = 0; i < n; i++) if (*(u8 **)(ATK_Q + i * 8) == s_p2c) n2++;
    if (n2 == 0) { F_BUILD_ATK(); return; }
    s_phase = "James 2 attack volumes";
    u8 keep[0xA8]; memcpy(keep, (void *)0x01D7F7D0, 0xA8);
    int n1 = 0;
    memset(ATK_Q, 0, 0xA0);
    for (int i = 0; i < n; i++) if (*(u8 **)(keep + 4 + i * 8) != s_p2c) { memcpy(ATK_Q + n1 * 8, keep + 4 + i * 8, 8); n1++; }
    ATK_FREE = 20 - n1;
    F_BUILD_ATK();                                               /* everyone but James 2 */
    memset(ATK_Q, 0, 0xA0); n2 = 0;
    for (int i = 0; i < n; i++) if (*(u8 **)(keep + 4 + i * 8) == s_p2c) { memcpy(ATK_Q + n2 * 8, keep + 4 + i * 8, 8); n2++; }
    ATK_FREE = 20 - n2;
    u8 *w1[2]; short wk[2] = { 0, 0 };
    save_state(&s_p1, 0); load_state(&s_p2, 0);
    w1_hide(w1, wk);
    F_BUILD_ATK();                                               /* James 2, with his state and his weapon copy */
    w1_show(w1, wk);
    save_state(&s_p2, 0); load_state(&s_p1, 0);
    memcpy((void *)0x01D7F7D0, keep, 0xA8);
    ATK_ANY = 1;
    s_phase = "game (outside the co-op frame hook)";
}


/* ===================== James 2's look: red jacket, brown hair =====================
 * While James 2's work is being drawn, the texture lookup returns recoloured copies of James's textures (built once
 * from whatever the game has loaded - the Enhanced Edition's packs included - and rebuilt when the game reloads
 * them).  The copies are made block by block for DXT1/DXT5 (the two colour end-points of every 4x4 block are
 * recoloured) and pixel by pixel for 32-bit textures, so any resolution works.  Region rules are in texture
 * fractions and match the James model's UV layout (all James model variants share ids 1239..1242). */
static int bytes_are(u32 addr, const char *hex);
int s_draw_p2;                              /* CE: the James being drawn, 1 or 2 (0 = someone else); read by the asm stub */
typedef struct { u32 id; void *src; IDirect3DTexture8 *tex; u8 rec[0x100]; } TexCopy;
static TexCopy s_texcopy[2][6]; static int s_tex_fail;
static int s_look_who;                      /* CE: 0 / 1 = the colour set the recolour below uses */
extern u8 *__cdecl tex_orig(u32 id);
extern void tex_stub(void);
__asm__(
    ".text\n"
    "_tex_orig:\n"                                   /* the lookup's first two instructions, then the rest of it */
    "    movl 4(%esp), %ecx\n"
    "    movl %ecx, %eax\n"
    "    movl $0x45A2F6, %edx\n"
    "    jmp *%edx\n"
    "_tex_stub:\n"
    "    cmpl $0, _s_draw_p2\n"
    "    je _tex_orig\n"
    "    pushl %ecx\n"
    "    pushl %edx\n"
    "    movl 12(%esp), %eax\n"
    "    pushl %eax\n"
    "    call _tex_replace\n"
    "    addl $4, %esp\n"
    "    popl %edx\n"
    "    popl %ecx\n"
    "    testl %eax, %eax\n"
    "    jz _tex_orig\n"
    "    ret\n"
);
static inline u8 clamp8(float v) { return v < 0 ? 0 : v > 255 ? 255 : (u8)v; }
/* the rule: id, position (fractions of the texture), colour in -> colour out; returns 0 if unchanged */
static int recolor(u32 id, float fx, float fy, u8 *r, u8 *g, u8 *b)
{
    int part = ce_part_of(id, fx, fy, *r, *g, *b);                    /* CE: part rules + palette shared with the preview */
    if (part < 0 || !ce_color[s_look_who][part]) return 0;
    ce_apply(ce_color[s_look_who][part], r, g, b);
    return 1;
}
__attribute__((unused)) static int recolor_old(u32 id, float fx, float fy, u8 *r, u8 *g, u8 *b)
{
    /* P17: rules re-made from the textures the PC game really loads (dumped in P16). Their layout differs from the
       Xbox model files the earlier rules were measured on (hands at the left edge of 1240, hair = right half of 1241). */
    float L = 0.30f * *r + 0.59f * *g + 0.11f * *b;
    int R = *r, G = *g, B = *b;
    if (id == 1240) {                                                      /* jacket */
        if (B > G + 10 || L > 110.0f || (R > G + 25 && R > 70)) return 0;  /* badges, flag, metal studs */
        if (R > G && G > B && R - B > 20 && L > 40.0f) return 0;            /* skin */
        if (fx < 0.5625f && fy >= 0.5f && G < R) return 0;                 /* hands picture: only the green-ish texels there are jacket */
        *r = clamp8(L * 1.7f + 12); *g = clamp8(L * 0.26f + 2); *b = clamp8(L * 0.24f + 2);
        return 1;
    }
    int hair = 0;
    if (id == 1242) hair = 1;                                              /* all hair */
    else if (id == 1241) hair = fx >= 0.5f;                                /* right half = hair strands + their backing */
    else if (id == 1239) {                                                 /* hair painted on the head texture: told from skin by colour */
        int inreg = (fy < 0.30f && fx >= 0.27f && fx <= 0.97f) || (fx >= 0.60f && fy < 0.60f) ||
                    (fy >= 0.84f && (fx >= 0.70f || fx <= 0.08f)) || (fx <= 0.07f && fy >= 0.53f && fy <= 0.60f);
        hair = inreg && L > 8.0f && R - G < 20 && G >= B - 4;
    }
    if (!hair) return 0;
    { float l = L * 0.55f + 10.0f;                                          /* dark brown; highlights no longer read as blond */
      *r = clamp8(l); *g = clamp8(l * 0.62f); *b = clamp8(l * 0.40f); }
    return 1;
}
static u16 rgb565_recolor(u32 id, float fx, float fy, u16 c)
{
    u8 r = (u8)(((c >> 11) & 31) * 255 / 31), g = (u8)(((c >> 5) & 63) * 255 / 63), b = (u8)((c & 31) * 255 / 31);
    if (!recolor(id, fx, fy, &r, &g, &b)) return c;
    return (u16)(((r * 31 + 127) / 255) << 11 | ((g * 63 + 127) / 255) << 5 | (b * 31 + 127) / 255);
}
/* copy one mip level with the recolour; returns 0 on an unsupported format */
static int recolor_level(u32 id, D3DFORMAT fmt, int w, int h, const u8 *src, int spitch, u8 *dst, int dpitch)
{
    if (fmt == D3DFMT_DXT1 || fmt == D3DFMT_DXT5) {
        int bw = (w + 3) / 4, bh = (h + 3) / 4, bs = fmt == D3DFMT_DXT1 ? 8 : 16, coff = fmt == D3DFMT_DXT1 ? 0 : 8;
        for (int by = 0; by < bh; by++) {
            const u8 *sp = src + by * spitch; u8 *dp = dst + by * dpitch;
            for (int bx = 0; bx < bw; bx++, sp += bs, dp += bs) {
                memcpy(dp, sp, bs);
                float fx = (bx * 4 + 2) / (float)w, fy = (by * 4 + 2) / (float)h;
                u16 c0 = *(const u16 *)(sp + coff), c1 = *(const u16 *)(sp + coff + 2);
                u16 n0 = rgb565_recolor(id, fx, fy, c0), n1 = rgb565_recolor(id, fx, fy, c1);
                if (fmt == D3DFMT_DXT1 && ((c0 > c1) != (n0 > n1))) {      /* DXT1: the order picks the block mode - keep it */
                    u16 t = n0; n0 = n1; n1 = t;
                    u32 idx = *(const u32 *)(sp + 4), out = 0;              /* swap index 0<->1 (and 2<->3 in 4-colour mode) */
                    for (int i = 0; i < 16; i++) { u32 v = (idx >> (i * 2)) & 3; if (c0 > c1) v ^= 1; else if (v < 2) v ^= 1; out |= v << (i * 2); }
                    *(u32 *)(dp + 4) = out;
                }
                *(u16 *)(dp + coff) = n0; *(u16 *)(dp + coff + 2) = n1;
            }
        }
        return 1;
    }
    if (fmt == D3DFMT_A8R8G8B8 || fmt == D3DFMT_X8R8G8B8) {
        for (int y = 0; y < h; y++) {
            const u8 *sp = src + y * spitch; u8 *dp = dst + y * dpitch;
            memcpy(dp, sp, w * 4);
            for (int x = 0; x < w; x++) { u8 *p = dp + x * 4; recolor(id, (x + 0.5f) / w, (y + 0.5f) / h, p + 2, p + 1, p); }
        }
        return 1;
    }
    return 0;
}
static IDirect3DTexture8 *make_copy(u32 id, IDirect3DTexture8 *src)
{
    IDirect3DDevice8 *dev = D3D_DEVICE;
    if (!dev) return NULL;
    D3DSURFACE_DESC d; if (src->lpVtbl->GetLevelDesc(src, 0, &d) != D3D_OK) return NULL;
    int levels = src->lpVtbl->GetLevelCount(src);
    IDirect3DTexture8 *t = NULL;
    if (dev->lpVtbl->CreateTexture(dev, d.Width, d.Height, levels, 0, d.Format, D3DPOOL_MANAGED, &t) != D3D_OK || !t) { Log("look: CreateTexture failed (id %u)", id); return NULL; }
    for (int lv = 0; lv < levels; lv++) {
        D3DSURFACE_DESC ld; src->lpVtbl->GetLevelDesc(src, lv, &ld);
        D3DLOCKED_RECT sr, dr;
        if (src->lpVtbl->LockRect(src, lv, &sr, NULL, D3DLOCK_READONLY) != D3D_OK) { Log("look: the game's texture %u is not lockable (pool %d)", id, d.Pool); t->lpVtbl->Release(t); return NULL; }
        if (t->lpVtbl->LockRect(t, lv, &dr, NULL, 0) != D3D_OK) { src->lpVtbl->UnlockRect(src, lv); t->lpVtbl->Release(t); return NULL; }
        int ok = recolor_level(id, ld.Format, ld.Width, ld.Height, (const u8 *)sr.pBits, sr.Pitch, (u8 *)dr.pBits, dr.Pitch);
        t->lpVtbl->UnlockRect(t, lv); src->lpVtbl->UnlockRect(src, lv);
        if (!ok) { Log("look: texture %u has an unsupported format %u", id, ld.Format); t->lpVtbl->Release(t); return NULL; }
    }
    Log("look: recoloured copy of texture %u made (%ux%u, format %u, %d levels)", id, d.Width, d.Height, d.Format, levels);
    return t;
}
/* ---- P16 look diagnostics: save the game's James textures and our copies as .dds next to the mod, and log every
   texture the game binds while James 2 is drawn (anything that is not one of our copies is saved too) ---- */
static int save_dds(IDirect3DTexture8 *t, const char *name)
{
    D3DSURFACE_DESC d; D3DLOCKED_RECT lr;
    if (!s_opt_dumps) return 0;                                      /* P33q: the dumps are a troubleshooting aid only */
    if (!t || t->lpVtbl->GetLevelDesc(t, 0, &d) != D3D_OK) return 0;
    int dxt = d.Format == D3DFMT_DXT1 || d.Format == D3DFMT_DXT3 || d.Format == D3DFMT_DXT5;
    int argb = d.Format == D3DFMT_A8R8G8B8 || d.Format == D3DFMT_X8R8G8B8;
    if (!dxt && !argb) { Log("look dump: %s has format %u, not saved", name, d.Format); return 0; }
    if (t->lpVtbl->LockRect(t, 0, &lr, NULL, D3DLOCK_READONLY) != D3D_OK) { Log("look dump: %s not lockable (pool %d)", name, d.Pool); return 0; }
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s%s.dds", s_dir, name);
    FILE *f = fopen(path, "wb");
    if (f) {
        u32 h[32]; memset(h, 0, sizeof h);
        int rows = dxt ? (int)(d.Height + 3) / 4 : (int)d.Height;
        int rowbytes = dxt ? (int)((d.Width + 3) / 4) * (d.Format == D3DFMT_DXT1 ? 8 : 16) : (int)d.Width * 4;
        h[0] = 0x20534444; h[1] = 124; h[2] = 0x1007 | (dxt ? 0x80000 : 0x8); h[3] = d.Height; h[4] = d.Width;
        h[5] = dxt ? (u32)(rowbytes * rows) : (u32)rowbytes; h[7] = 1; h[19] = 32;
        if (dxt) { h[20] = 4; h[21] = d.Format; } else { h[20] = 0x41; h[22] = 32; h[23] = 0xFF0000; h[24] = 0xFF00; h[25] = 0xFF; h[26] = 0xFF000000u; }
        h[27] = 0x1000;
        fwrite(h, 4, 32, f);
        for (int y = 0; y < rows; y++) fwrite((u8 *)lr.pBits + y * lr.Pitch, 1, rowbytes, f);
        fclose(f);
        Log("look dump: saved %s (%ux%u format %u)", path, d.Width, d.Height, d.Format);
    }
    t->lpVtbl->UnlockRect(t, 0);
    return 1;
}
typedef HRESULT (STDMETHODCALLTYPE *SetTexFn)(IDirect3DDevice8 *, DWORD, IDirect3DBaseTexture8 *);
static SetTexFn s_settex_orig;
static void *s_seen_tex[48]; static DWORD s_seen_stage[48]; static int s_seen_n, s_dumps_n;
static HRESULT STDMETHODCALLTYPE settex_hook(IDirect3DDevice8 *dev, DWORD stage, IDirect3DBaseTexture8 *t)
{
    if (s_draw_p2 && s_seen_n < 48) {
        int seen = 0;
        for (int i = 0; i < s_seen_n; i++) if (s_seen_tex[i] == t && s_seen_stage[i] == stage) seen = 1;
        if (!seen) {
            s_seen_tex[s_seen_n] = t; s_seen_stage[s_seen_n] = stage; s_seen_n++;
            int copy = -1;
            for (int w = 0; w < 2; w++) for (int i = 0; i < 6; i++) if (t && (void *)s_texcopy[w][i].tex == (void *)t) copy = (int)s_texcopy[w][i].id;
            if (copy >= 0) Log("look: James 2 drawn with stage %lu = our copy of %d", (unsigned long)stage, copy);
            else {
                Log("look: James 2 drawn with stage %lu = %p, NOT one of our copies", (unsigned long)stage, (void *)t);
                if (t && s_dumps_n < 8 && t->lpVtbl->GetType(t) == D3DRTYPE_TEXTURE) {
                    char nm[64]; snprintf(nm, sizeof nm, "sh2coop_dump_other_%d_stage%lu", s_dumps_n++, (unsigned long)stage);
                    save_dds((IDirect3DTexture8 *)t, nm);
                }
            }
        }
    }
    return s_settex_orig(dev, stage, t);
}
static void hook_settex(void)
{
    static int done; IDirect3DDevice8 *dev = D3D_DEVICE;
    if (done || !dev) return;
    done = 1;
    void **vt = *(void ***)dev; int slot = offsetof(IDirect3DDevice8Vtbl, SetTexture) / sizeof(void *);
    DWORD old;
    if (!VirtualProtect(&vt[slot], sizeof(void *), PAGE_EXECUTE_READWRITE, &old)) { Log("look: SetTexture hook failed"); return; }
    s_settex_orig = (SetTexFn)vt[slot]; vt[slot] = (void *)settex_hook;
    VirtualProtect(&vt[slot], sizeof(void *), old, &old);
    Log("look: watching textures bound while James 2 is drawn (slot %d)", slot);
}
/* called from the asm stub while James 2 is drawn: a replacement record for James's textures, else NULL */
static void look_drop(int who)                                        /* CE: colours changed - the copies are remade on the next draw */
{
    for (int i = 0; i < 6; i++) if (s_texcopy[who][i].tex) { s_texcopy[who][i].tex->lpVtbl->Release(s_texcopy[who][i].tex); s_texcopy[who][i].tex = NULL; }
}
int look_recolor_dxt5(int who, ce_u32 id, int w, int h, const ce_u8 *src, ce_u8 *dst, int dpitch)
{
    int keep = s_look_who; s_look_who = who;
    int ok = recolor_level(id, D3DFMT_DXT5, w, h, src, ((w + 3) / 4) * 16, dst, dpitch);
    s_look_who = keep;
    return ok;
}
u8 *__cdecl tex_replace(u32 id)
{
    if (s_tex_fail > 8 || s_draw_p2 < 1 || s_draw_p2 > 2) return NULL;
    if (id < 1239 || id > 1242) return NULL;
    int who = s_draw_p2 - 1;
    if (!ce_any(who)) return NULL;
    { static int seen[2]; if (seen[who] != ce_gen[who]) { seen[who] = ce_gen[who]; look_drop(who); } }
    u8 *rec = tex_orig(id);
    if (!rec) return NULL;
    IDirect3DTexture8 *src = *(IDirect3DTexture8 **)(rec + TEX_REC_D3D);
    if (!src) return NULL;
    TexCopy *e = NULL;
    for (int i = 0; i < 6; i++) if (s_texcopy[who][i].id == id) e = &s_texcopy[who][i];
    if (!e) for (int i = 0; i < 6; i++) if (!s_texcopy[who][i].id) { e = &s_texcopy[who][i]; e->id = id; break; }
    if (!e) return NULL;
    if (e->src != src || !e->tex) {                                      /* first time, or the game reloaded the model */
        if (e->tex) { e->tex->lpVtbl->Release(e->tex); e->tex = NULL; }
        s_look_who = who;
        e->tex = make_copy(id, src);
        s_look_who = 0;
        if (!e->tex) { s_tex_fail++; return NULL; }
        e->src = src;
        { static int saved[4]; int k = (int)id - 1239;
          if (k >= 0 && k < 4 && !saved[k]) { char nm[64]; saved[k] = 1;
            snprintf(nm, sizeof nm, "sh2coop_dump_game_%u", id); save_dds(src, nm);
            snprintf(nm, sizeof nm, "sh2coop_dump_copy_%u", id); save_dds(e->tex, nm); } }
        memcpy(e->rec, rec, 0x90);
        *(IDirect3DTexture8 **)(e->rec + TEX_REC_D3D) = e->tex;
    }
    return e->rec;
}
/* replaces the per-work draw call in the character render loop */
static int s_fl2_mode;   /* F9: 0 = normal, 1 = with the map, 2 = all of James 2's flashlight off (defined below) */
/* P30: James 2's flashlight glow - the game's flare drawn a second time with James 2's table and player state */
static void __cdecl flare_draw_hook(void)
{
    /* P33: in a split frame the lens glare is left out. It runs once at the end of the frame, its occlusion test
       0x511520 renders with the current camera into a full-size target (SetRenderTarget 0x511835 / 0x511A01) and its
       screen position was worked out during the update for James 1's full-screen camera - in a split frame it would
       sit in the wrong place. */
    if (s_split_now) return;
    ((void (__cdecl *)(void))0x00511F60)();                    /* James 1's, as always */
    if (!s_flare_j2_ok || !s_opt_fl2 || s_fl2_mode == 2 || !s_active || !s_p2c || !(FLAGS(s_p2c) & F_DRAWN)) return;
    s_phase = "James 2's flashlight glow";
    memcpy(s_flare_j1, (void *)FLARE_TAB, FLARE_SZ);
    memcpy((void *)FLARE_TAB, s_flare_j2, FLARE_SZ);
    flare_sync_ctrl();
    save_state(&s_p1, 0); load_state(&s_p2, 0);
    ((void (__cdecl *)(void))0x00511F60)();
    load_state(&s_p1, 0);
    memcpy(s_flare_j2, (void *)FLARE_TAB, FLARE_SZ);
    memcpy((void *)FLARE_TAB, s_flare_j1, FLARE_SZ);
    s_flare_j2_draws++;
}
/* ===================== P29: James 2's flashlight =====================
   Traced in sh2pc.exe (11 Oct 2026; the Xbox notes had position/direction swapped and missed most of this):
   - Each James's post pass (0x5384F0 -> 0x536D70 -> 0x536540) writes "the player's" flashlight into the player block:
     +0x98 source position, +0xA8 direction (0x47AC50 reads them through 0x533CC0 in normal play). The mod runs
     James 2's post pass with his own block, so s_p2.block+0x98/+0xA8 already hold James 2's flashlight each frame.
   - The renderer has ONE spot light. 0x5B1BA0 copies it to 0x1DB8678 (position) / 0x1DB86B8 (direction); the
     per-object constant builder 0x5B28B0(world) turns them into vertex-shader constants c4..c7 (cone texture
     projection) and c11 (position in object space) and uploads c0..c27 from 0x1DB86E0. The lit world shaders
     (VS 5/6 + PS 1/3; handles 0x8F02D0, 0x8F02E0, 0x8F0314, 0x8F0324) draw
         colour = (ambient c19.. + 3 directional c15..c17 + baked vertex colour c20..c22) * texture + spot term.
     James 2's beam = the same geometry drawn once more, ADDED on top, with the game's own shaders: c4..c7 and c11
     made by the game's own 0x5B28B0 for James 2's position/direction, c15..c17 and c19..c22 set to zero (so only
     the spot term is left), blending ONE/ONE (SRCALPHA/ONE where the surface already blends), no depth writes,
     fog colour black. Colour, cone texture, range, fade and on/off are James 1's flashlight's.
   - Enemies / NPCs use a separate light pool (12 slots x 0x98 at 0xAA9CF8); for non-James models the flashlight is
     a type-3 spot in slot 0 (0x5087A0 / 0x508A40 -> 0x50D6C0). While such a character is drawn, a free slot gets a
     copy of slot 0 moved to James 2's flashlight, and the per-character pick (0x50D060, sorted strongest first)
     keeps only the stronger of the two (the per-pixel character shaders take one spot). */
/* P33: the "other" James's flashlight. In James 2's split view his own block is the live one (the game's spot light is
   his torch there), so the added light is James 1's - the same picture as James 1's view with the roles swapped. */
#define FL2_SRC (s_split_view == 1 ? s_p1.block : s_p2.block)
#define FL2_POS ((const float *)(FL2_SRC + 0x98))
#define FL2_DIR ((const float *)(FL2_SRC + 0xA8))
#define SPOT_POS ((float *)0x01DB8678)
#define SPOT_DIR ((float *)0x01DB86B8)
#define VSC_BUF  ((u8 *)0x01DB86E0)          /* c0..c27 as uploaded by 0x5B28B0 (cN at +16*N) */
#define J1_TORCH ((const float *)0x01FB7D18) /* live player block +0x98 = James 1's at render time */
static int s_fl2_ok, s_fl2_chars_ok;          /* world hooks / character hooks installed */
static int s_fl2_j2;                          /* James 2 in play this frame with a valid flashlight */
static int s_j2c_ok;                          /* s_j2c holds James 2's c4..c11 for the object being drawn */
static float s_j2c[8][4];
static const float s_zero4x4[4][4];
static const char *s_fl2_why = "-";
static DWORD s_fl2_objs, s_fl2_draws, s_fl2_chars, s_fl2_charsk;
static int s_fl2_slot = -1;
static u8 s_fl2_slot_save[0x98];
extern void __cdecl upl_orig(const float *world);
__asm__(
    ".text\n"
    "_upl_orig:\n"                      /* 0x5B28B0's first instruction, then the rest of it (its ret returns to our caller) */
    "    subl $0xa0, %esp\n"
    "    pushl $0x5B28B6\n"
    "    ret\n"
);
static int fl2_dir_ok(void)
{
    float l = FL2_DIR[0] * FL2_DIR[0] + FL2_DIR[1] * FL2_DIR[1] + FL2_DIR[2] * FL2_DIR[2];
    return l > 0.25f && l < 4.0f && (FL2_POS[0] != 0.0f || FL2_POS[2] != 0.0f);
}
/* render time: is the one spot light of this pass James 1's flashlight, and should James 2's be added? */
static int s_fl2_mode;
static int fl2_gate(void)
{
    if (!s_fl2_ok || !s_opt_fl2) { s_fl2_why = "switched off"; return 0; }
    if (s_fl2_mode == 2) { s_fl2_why = "turned off with F9"; return 0; }
    if (!s_fl2_j2) { s_fl2_why = "James 2 not in play"; return 0; }
    if (!*(u32 *)0x00A33374 || !*(u32 *)0x00A33370) { s_fl2_why = "the game is not using shaders"; return 0; }
    if (*(u32 *)0x01DB8A28 != 0) { s_fl2_why = "no flashlight light this frame"; return 0; }
    if (fabsf(SPOT_POS[0] - J1_TORCH[0]) > 2.0f || fabsf(SPOT_POS[1] - J1_TORCH[1]) > 2.0f || fabsf(SPOT_POS[2] - J1_TORCH[2]) > 2.0f)
        { s_fl2_why = "the light is not James 1's flashlight (room light / scene)"; return 0; }
    s_fl2_why = "on";
    return 1;
}
/* replaces 0x5B28B0 (jump at its first instruction): the game's constants as always, then James 2's c4..c11 */
static DWORD s_fl2_vok[3], s_fl2_vno[3];      /* P33: per view (none / James 1's half / James 2's half): beam added or not */
static const char *s_fl2_vwhy[3] = { "-", "-", "-" };
void __cdecl upl_hook_c(const float *world)
{
    upl_orig(world);
    s_j2c_ok = 0;
    int gate = fl2_gate();
    if (s_split_now) { int k = s_split_view + 1; if (gate) s_fl2_vok[k]++; else { s_fl2_vno[k]++; s_fl2_vwhy[k] = s_fl2_why; } }
    if (!gate) return;
    IDirect3DDevice8 *dev = D3D_DEVICE;
    if (!dev) return;
    float sp[4], sd[4], j1c[8][4];
    memcpy(sp, SPOT_POS, 16); memcpy(sd, SPOT_DIR, 16); memcpy(j1c, VSC_BUF + 4 * 16, sizeof j1c);
    SPOT_POS[0] = FL2_POS[0]; SPOT_POS[1] = FL2_POS[1]; SPOT_POS[2] = FL2_POS[2];
    SPOT_DIR[0] = FL2_DIR[0]; SPOT_DIR[1] = FL2_DIR[1]; SPOT_DIR[2] = FL2_DIR[2];
    upl_orig(world);                                           /* the game's own math for James 2's beam */
    memcpy(s_j2c, VSC_BUF + 4 * 16, sizeof s_j2c);
    memcpy(SPOT_POS, sp, 16); memcpy(SPOT_DIR, sd, 16); memcpy(VSC_BUF + 4 * 16, j1c, sizeof j1c);
    dev->lpVtbl->SetVertexShaderConstant(dev, 4, VSC_BUF + 4 * 16, 8);   /* James 1's c4..c11 back on the card */
    s_j2c_ok = 1; s_fl2_objs++;
}
static DWORD s_sv_ab, s_sv_sb, s_sv_db, s_sv_zw, s_sv_zf, s_sv_fc, s_sv_cw, s_sv_bo, s_sv_st;
static int fl2_pass_begin(IDirect3DDevice8 *d)
{
    DWORD vs = 0, ps = 0;
    if (d->lpVtbl->GetVertexShader(d, &vs) != D3D_OK || d->lpVtbl->GetPixelShader(d, &ps) != D3D_OK) return 0;
    if (!((vs == *(u32 *)0x008F02D0 || vs == *(u32 *)0x008F02E0) && (ps == *(u32 *)0x008F0314 || ps == *(u32 *)0x008F0324))) return 0;
    if (d->lpVtbl->GetRenderState(d, D3DRS_ALPHABLENDENABLE, &s_sv_ab) != D3D_OK || d->lpVtbl->GetRenderState(d, D3DRS_SRCBLEND, &s_sv_sb) != D3D_OK
        || d->lpVtbl->GetRenderState(d, D3DRS_DESTBLEND, &s_sv_db) != D3D_OK || d->lpVtbl->GetRenderState(d, D3DRS_ZWRITEENABLE, &s_sv_zw) != D3D_OK
        || d->lpVtbl->GetRenderState(d, D3DRS_ZFUNC, &s_sv_zf) != D3D_OK || d->lpVtbl->GetRenderState(d, D3DRS_FOGCOLOR, &s_sv_fc) != D3D_OK
        || d->lpVtbl->GetRenderState(d, D3DRS_COLORWRITEENABLE, &s_sv_cw) != D3D_OK || d->lpVtbl->GetRenderState(d, D3DRS_BLENDOP, &s_sv_bo) != D3D_OK
        || d->lpVtbl->GetRenderState(d, D3DRS_STENCILENABLE, &s_sv_st) != D3D_OK) return 0;
    DWORD sb = D3DBLEND_ONE;
    if (s_sv_ab) { if (s_sv_sb != D3DBLEND_SRCALPHA) return 0; sb = D3DBLEND_SRCALPHA; }   /* other blend modes: leave them alone */
    d->lpVtbl->SetVertexShaderConstant(d, 4, s_j2c, 8);
    d->lpVtbl->SetVertexShaderConstant(d, 15, s_zero4x4, 3);
    d->lpVtbl->SetVertexShaderConstant(d, 19, s_zero4x4, 4);
    d->lpVtbl->SetRenderState(d, D3DRS_ALPHABLENDENABLE, TRUE);
    d->lpVtbl->SetRenderState(d, D3DRS_SRCBLEND, sb);
    d->lpVtbl->SetRenderState(d, D3DRS_DESTBLEND, D3DBLEND_ONE);
    d->lpVtbl->SetRenderState(d, D3DRS_ZWRITEENABLE, FALSE);
    d->lpVtbl->SetRenderState(d, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    d->lpVtbl->SetRenderState(d, D3DRS_FOGCOLOR, 0);
    d->lpVtbl->SetRenderState(d, D3DRS_COLORWRITEENABLE, s_sv_cw & 7);   /* light only: never add to the alpha channel */
    d->lpVtbl->SetRenderState(d, D3DRS_BLENDOP, D3DBLENDOP_ADD);          /* never subtract, whatever was left set */
    d->lpVtbl->SetRenderState(d, D3DRS_STENCILENABLE, FALSE);
    return 1;
}
static void fl2_pass_end(IDirect3DDevice8 *d)
{
    d->lpVtbl->SetRenderState(d, D3DRS_ALPHABLENDENABLE, s_sv_ab);
    d->lpVtbl->SetRenderState(d, D3DRS_SRCBLEND, s_sv_sb);
    d->lpVtbl->SetRenderState(d, D3DRS_DESTBLEND, s_sv_db);
    d->lpVtbl->SetRenderState(d, D3DRS_ZWRITEENABLE, s_sv_zw);
    d->lpVtbl->SetRenderState(d, D3DRS_ZFUNC, s_sv_zf);
    d->lpVtbl->SetRenderState(d, D3DRS_FOGCOLOR, s_sv_fc);
    d->lpVtbl->SetRenderState(d, D3DRS_COLORWRITEENABLE, s_sv_cw);
    d->lpVtbl->SetRenderState(d, D3DRS_BLENDOP, s_sv_bo);
    d->lpVtbl->SetRenderState(d, D3DRS_STENCILENABLE, s_sv_st);
    d->lpVtbl->SetVertexShaderConstant(d, 4, VSC_BUF + 4 * 16, 8);      /* James 1's constants, as the game left them */
    d->lpVtbl->SetVertexShaderConstant(d, 15, VSC_BUF + 15 * 16, 3);
    d->lpVtbl->SetVertexShaderConstant(d, 19, VSC_BUF + 19 * 16, 4);
    s_fl2_draws++;
}
/* P30: the map is drawn (0x47D340) BEFORE the stencil shadows of James 1's flashlight (0x5A81A0 -> 0x445380, inside
   0x476680), and the shadows darken everything drawn before them - so James 2's beam drawn together with the map got
   James 1's shadows painted into it (the black streaks: James 2's / James 1's gun shadows cast by James 1's light).
   Now the map is drawn normally, and right after the shadow pass the map renderer runs once more with only James 2's
   added light (s_fl2_replay: the game's own draws are skipped). The draw lists (0x5AFEE0, run at the end of the frame
   from 0x476AC0) are after the shadows already, so James 2's light is still added right there. */
static int s_fl2_defer;          /* the shadow-pass hooks are in: the map's James 2 light is drawn after the shadows */
/* s_fl2_mode - F9 in game: 0 = after James 1's shadows (normal), 1 = with the map (the P29 way), 2 = off */
static int s_fl2_replay;         /* the map renderer is running again for James 2's light only */
static DWORD s_fl2_replays;
HRESULT __stdcall dip_hook(IDirect3DDevice8 *d, D3DPRIMITIVETYPE t, UINT mi, UINT nv, UINT si, UINT pc)
{
    if (s_fl2_replay) {
        if (s_j2c_ok && fl2_pass_begin(d)) { d->lpVtbl->DrawIndexedPrimitive(d, t, mi, nv, si, pc); fl2_pass_end(d); }
        return D3D_OK;
    }
#ifdef SH2COOP_TEST_TOOLS
    if (s_tcap_arm) {   /* cloud test: record the draw calls of one frame and the vertex shader constants of one of them */
        if (s_tcap_n < 512) { s_tcap_log[s_tcap_n][0] = t; s_tcap_log[s_tcap_n][1] = nv; s_tcap_log[s_tcap_n][2] = pc; s_tcap_log[s_tcap_n][3] = si; s_tcap_n++; }
        if ((int)pc == s_tcap_pc && !s_tcap_got) { d->lpVtbl->GetVertexShaderConstant(d, 0, s_tcap_vsc, 96); s_tcap_got = 1; }
    }
#endif
    HRESULT hr = d->lpVtbl->DrawIndexedPrimitive(d, t, mi, nv, si, pc);
    if ((!s_fl2_defer || s_fl2_mode == 1) && s_j2c_ok && fl2_pass_begin(d)) { d->lpVtbl->DrawIndexedPrimitive(d, t, mi, nv, si, pc); fl2_pass_end(d); }
    return hr;
}
/* P31 diagnostics for the black streaks (F11 / F12 in game). The flashlight shadows: 0x50BB20 registers the James /
   weapon models as shadow casters (0x509C20 mode 1, call 0x50BBD6) before the shadow pass 0x5A81A0, and the character
   draw loop 0x50B980 again after drawing each one (mode 0, call 0x50BAD0). James 1 and James 2 share one model slot
   (0x1F7D820 table, shadow data at slot+0x1C), so James 2's casters use the same shadow data as James 1's. */
static int s_dbg_noshadow;           /* F11: the game's flashlight shadow pass is skipped (test) */
static int s_dbg_noshadow_j2 = 1;    /* F12: James 2 and his weapon copies are not registered as shadow casters.
   P31 default ON: the shadow code is written for one James - 0x5A5AA0 keeps one "James" state (0x1DBBD10, filled by
   0x5AC920 / 0x5ACD60 at each James registration) that the build step 0x5A69F0 (via 0x5A7830, inside 0x5A81A0) uses
   for EVERY James record (0x5AA480 / 0x5AA2E0 get &0x1DBBD10), plus the draw mode 0x932B50/54 (0x445630) - the
   last James to register wins, and James 2 registers after James 1. F12 turns it back to the game's behaviour. */
static DWORD s_casts_j1, s_casts_j2;
/* the shadow code keeps ONE set of "James" values (0x5A5AA0 for models 0x100-0x103: 0x1DBBD10 from 0x5AC920 /
   0x5ACD60, 0x1DBBE40, 0x1DBBF70, 0x1DBBD30; 0x1DBBD20 = James registered, read by the weapon models 0x800-0x80A),
   cleared each frame by 0x5A59A0. Recorded right after James 1's and after James 2's registration, for the log. */
typedef struct { short st, cnt; float e40[4], f70[4], d30[4]; int seen; } ShdSnap;
static ShdSnap s_shd[2];
static void shd_snap(ShdSnap *v)
{
    v->st = *(short *)0x01DBBD10; v->cnt = *(short *)0x01DBBDB0;
    memcpy(v->e40, (void *)0x01DBBE40, 16); memcpy(v->f70, (void *)0x01DBBF70, 16); memcpy(v->d30, (void *)0x01DBBD30, 16);
    v->seen = 1;
}
static void __cdecl shadow_cast_hook(int mode, u8 *slot, u8 *work)
{
    int j2 = s_active && work && (work == s_p2c || work == s_w2[0] || work == s_w2[1]);
    if (s_split_view == 1 && work)   /* P33: James 2's split view: his torch is the light, so James 1 is "the other James" */
        j2 = work == *(u8 **)s_p1.block || ((MODEL(work) & 0xFFC0) == 0x800 && *(u16 *)(work + W_INST) == 0);
    if (j2) { s_casts_j2++; if (s_dbg_noshadow_j2) return; } else s_casts_j1++;
    if (work && chr_owner(work, NULL) >= 0) return;   /* CS: James's shadow shape would fall on the character (dark stripes) */
    ((void (__cdecl *)(int, u8 *, u8 *))0x00509C20)(mode, slot, work);
    if (s_active && work && (work == PLAYER || work == s_p2c)) shd_snap(&s_shd[work == s_p2c]);
}
static void shd_log(void)
{
    for (int j = 0; j < 2; j++) {
        ShdSnap *v = &s_shd[j];
        if (!v->seen) { Log("  shadow values after James %d registers: (not registered in the last frame logged)", j + 1); continue; }
        Log("  shadow values after James %d registers: D10 %d casters %d | E40 %.0f %.0f %.0f %.0f | F70 %.0f %.0f %.0f %.0f | D30 %.0f %.0f %.0f %.0f",
            j + 1, v->st, v->cnt, v->e40[0], v->e40[1], v->e40[2], v->e40[3], v->f70[0], v->f70[1], v->f70[2], v->f70[3],
            v->d30[0], v->d30[1], v->d30[2], v->d30[3]);
        v->seen = 0;
    }
}
/* P32 spawn diagnostics: the streaks appear the moment James 2 joins ("only when James initially spawns in") and stay
   put. For 3 s after each join, and then every 5 s, the log records James 2's flashlight values, whether any of his
   bone nodes or his weapon copy's nodes are empty (all-zero rotation) or wild (far away), and where the weapon copy's
   two nodes are compared with James 1's own weapon (same model) - anything created once at the join and left
   stale shows up here. */
static int s_diag_frames;
static void node_stats(u8 *head, int n, int *flat, int *wild)
{
    *flat = *wild = 0;
    u8 *p = head;
    for (int k = 0; k < n && p; k++, p = *(u8 **)p) {
        const float *m = (const float *)(p + 8);
        float r = fabsf(m[0]) + fabsf(m[1]) + fabsf(m[2]) + fabsf(m[4]) + fabsf(m[5]) + fabsf(m[6]) + fabsf(m[8]) + fabsf(m[9]) + fabsf(m[10]);
        float t = fabsf(m[12]) + fabsf(m[13]) + fabsf(m[14]);
        if (!(r > 1e-6f)) (*flat)++;
        if (!(t < 30000.0f) || !(r < 100.0f)) (*wild)++;
    }
}
static const float *node_t(u8 *w, int k)
{
    static const float z[3];
    u8 *p = w ? *(u8 **)(w + 0x9C) : NULL;
    for (int i = 0; i < k && p; i++) p = *(u8 **)p;
    return p ? (const float *)(p + 8 + 0x30) : z;
}
static void spawn_diag(int periodic)
{
    if (!s_active || !s_p2c || !is_in_list(s_p2c)) return;
    if (!periodic) {
        if (s_diag_frames <= 0) return;
        s_diag_frames--;
        if (s_diag_frames % 15) return;
    }
    int need = F_BONES_NEEDED(MODEL(s_p2c)), fl = 0, wl = 0;
    node_stats(*(u8 **)(s_p2c + 0x9C), need, &fl, &wl);
    Log("%s: J2 light %s (gate: %s) at %.0f %.0f %.0f dir %.2f %.2f %.2f | J1 torch %.0f %.0f %.0f | J2 pos %.0f %.0f %.0f | J2 bones %d empty, %d wild of %d",
        periodic ? "check" : "after join", s_fl2_j2 ? "on" : "off", s_fl2_why, FL2_POS[0], FL2_POS[1], FL2_POS[2], FL2_DIR[0], FL2_DIR[1], FL2_DIR[2],
        J1_TORCH[0], J1_TORCH[1], J1_TORCH[2], POSP(s_p2c)[0], POSP(s_p2c)[1], POSP(s_p2c)[2], fl, wl, need);
    for (int k = 0; k < 2; k++) {
        u8 *w = s_w2[k];
        if (!w || !is_in_list(w)) continue;
        u8 *w1 = F_FIND(MODEL(w), 0);
        int wn = F_BONES_NEEDED(MODEL(w)), wf = 0, ww = 0;
        node_stats(*(u8 **)(w + 0x9C), wn, &wf, &ww);
        const float *a0 = node_t(w, 0), *a1 = node_t(w, 1), *b0 = node_t(w1, 0), *b1 = node_t(w1, 1);
        Log("  weapon copy %d (model 0x%x, %d nodes, %d empty, %d wild, %s, pos %.0f %.0f %.0f): node0 %.0f %.0f %.0f node1 %.0f %.0f %.0f | James 1's: node0 %.0f %.0f %.0f node1 %.0f %.0f %.0f",
            k, MODEL(w), wn, wf, ww, (FLAGS(w) & F_DRAWN) ? "drawn" : "hidden", POSP(w)[0], POSP(w)[1], POSP(w)[2],
            a0[0], a0[1], a0[2], a1[0], a1[1], a1[2], b0[0], b0[1], b0[2], b1[0], b1[1], b1[2]);
    }
}
/* Everything the map renderer (0x47D340 -> 0x5B4CC0 / 0x5B4940 / 0x5B2D40 / 0x5B2880 / 0x5B28B0) can change on the
   device is read before the re-run and set back after it - through the game's own D3D8 interface, so the Enhanced
   Edition's d3d8-to-d3d9 layer keeps its own copies (current shaders, index base) right. */
static const u8 kSvRS[] = { 7, 8, 9, 14, 15, 19, 20, 22, 23, 24, 25, 26, 27, 28, 29, 34, 35, 36, 37, 38, 47, 48,
                            52, 53, 54, 55, 56, 57, 58, 59, 60, 136, 137, 139, 140, 141, 142, 143, 145, 146, 147, 148,
                            151, 152, 168, 171 };
static const u8 kSvTSS[] = { 1, 2, 3, 4, 5, 6, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 24, 25, 26, 27, 28 };
static const DWORD kSvXF[] = { 2, 3, 16, 17, 18, 19, 256 };
typedef struct {
    DWORD rs[sizeof kSvRS]; u8 rs_ok[sizeof kSvRS];
    DWORD tss[4][sizeof kSvTSS]; u8 tss_ok[4][sizeof kSvTSS];
    IDirect3DBaseTexture8 *tex[4]; u8 tex_ok[4];
    DWORD vs, ps; int vs_ok, ps_ok;
    float vsc[96][4]; int vsc_ok; float psc[8][4]; int psc_ok;
    IDirect3DVertexBuffer8 *vb; UINT stride; int vb_ok;
    IDirect3DIndexBuffer8 *ib; UINT base; int ib_ok;
    D3DMATRIX xf[sizeof kSvXF / sizeof kSvXF[0]]; u8 xf_ok[sizeof kSvXF / sizeof kSvXF[0]];
    D3DMATERIAL8 mat; int mat_ok;
    unsigned zb; int zb_ok;                  /* d3d9 D3DRS_DEPTHBIAS, saved there instead of ZBIAS (47) when the d3d9 device is known */
} DevState;
static DevState s_dev_sv;
static void dev_save(IDirect3DDevice8 *d, DevState *v)
{
    v->zb_ok = s_d9_ok && d9_get_rs(195 /* D3DRS_DEPTHBIAS */, &v->zb);
    for (unsigned i = 0; i < sizeof kSvRS; i++)
        v->rs_ok[i] = (kSvRS[i] == 47 && v->zb_ok) ? 0 : d->lpVtbl->GetRenderState(d, (D3DRENDERSTATETYPE)kSvRS[i], &v->rs[i]) == D3D_OK;
    for (int t = 0; t < 4; t++) {
        for (unsigned i = 0; i < sizeof kSvTSS; i++) v->tss_ok[t][i] = d->lpVtbl->GetTextureStageState(d, t, (D3DTEXTURESTAGESTATETYPE)kSvTSS[i], &v->tss[t][i]) == D3D_OK;
        v->tex[t] = NULL; v->tex_ok[t] = d->lpVtbl->GetTexture(d, t, &v->tex[t]) == D3D_OK;
    }
    v->vs_ok = d->lpVtbl->GetVertexShader(d, &v->vs) == D3D_OK;
    v->ps_ok = d->lpVtbl->GetPixelShader(d, &v->ps) == D3D_OK;
    v->vsc_ok = d->lpVtbl->GetVertexShaderConstant(d, 0, v->vsc, 96) == D3D_OK;
    v->psc_ok = d->lpVtbl->GetPixelShaderConstant(d, 0, v->psc, 8) == D3D_OK;
    v->vb = NULL; v->vb_ok = d->lpVtbl->GetStreamSource(d, 0, &v->vb, &v->stride) == D3D_OK;
    v->ib = NULL; v->ib_ok = d->lpVtbl->GetIndices(d, &v->ib, &v->base) == D3D_OK;
    for (unsigned i = 0; i < sizeof kSvXF / sizeof kSvXF[0]; i++) v->xf_ok[i] = d->lpVtbl->GetTransform(d, (D3DTRANSFORMSTATETYPE)kSvXF[i], &v->xf[i]) == D3D_OK;
    v->mat_ok = d->lpVtbl->GetMaterial(d, &v->mat) == D3D_OK;
}
static void dev_restore(IDirect3DDevice8 *d, DevState *v)
{
    if (v->vs_ok) d->lpVtbl->SetVertexShader(d, v->vs);
    if (v->ps_ok) d->lpVtbl->SetPixelShader(d, v->ps);
    if (v->vsc_ok) d->lpVtbl->SetVertexShaderConstant(d, 0, v->vsc, 96);
    if (v->psc_ok) d->lpVtbl->SetPixelShaderConstant(d, 0, v->psc, 8);
    for (int t = 0; t < 4; t++) {
        if (v->tex_ok[t]) { d->lpVtbl->SetTexture(d, t, v->tex[t]); if (v->tex[t]) v->tex[t]->lpVtbl->Release(v->tex[t]); }
        for (unsigned i = 0; i < sizeof kSvTSS; i++) if (v->tss_ok[t][i]) d->lpVtbl->SetTextureStageState(d, t, (D3DTEXTURESTAGESTATETYPE)kSvTSS[i], v->tss[t][i]);
    }
    for (unsigned i = 0; i < sizeof kSvRS; i++) if (v->rs_ok[i]) d->lpVtbl->SetRenderState(d, (D3DRENDERSTATETYPE)kSvRS[i], v->rs[i]);
    if (v->zb_ok) d9_set_rs(195 /* D3DRS_DEPTHBIAS */, v->zb);
    if (v->vb_ok) { d->lpVtbl->SetStreamSource(d, 0, v->vb, v->stride); if (v->vb) v->vb->lpVtbl->Release(v->vb); }
    if (v->ib_ok) { d->lpVtbl->SetIndices(d, v->ib, v->base); if (v->ib) v->ib->lpVtbl->Release(v->ib); }
    for (unsigned i = 0; i < sizeof kSvXF / sizeof kSvXF[0]; i++) if (v->xf_ok[i]) d->lpVtbl->SetTransform(d, (D3DTRANSFORMSTATETYPE)kSvXF[i], &v->xf[i]);
    if (v->mat_ok) d->lpVtbl->SetMaterial(d, &v->mat);
}
/* replaces the two calls of the shadow pass 0x5A81A0 inside 0x476680 (0x4766C5, 0x4766DA) */
static void __cdecl shadow_pass_hook(void)
{
    if (!s_dbg_noshadow) ((void (__cdecl *)(void))0x005A81A0)();
    if (!s_fl2_defer || s_fl2_mode != 0 || !fl2_gate()) return;
    IDirect3DDevice8 *d = D3D_DEVICE;
    if (!d) return;
    split_attach(d);
    dev_save(d, &s_dev_sv);
    u32 cursor = *(u32 *)0x00941174;                     /* the map renderer's scratch memory (0x4594A0 / 0x4594F0) */
    u32 map = *(u32 *)0x01F5ED40;                         /* current map set by 0x5B3CB0 */
    d->lpVtbl->SetRenderState(d, D3DRS_STENCILENABLE, FALSE);
    s_phase = "James 2's flashlight (map, after the shadows)";
    s_fl2_replay = 1;
    ((int (__cdecl *)(void))0x0047D340)();
    s_fl2_replay = 0;
    *(u32 *)0x00941174 = cursor;
    *(u32 *)0x01F5ED40 = map;
    dev_restore(d, &s_dev_sv);
    s_fl2_replays++;
}
HRESULT __stdcall dp_hook(IDirect3DDevice8 *d, D3DPRIMITIVETYPE t, UINT sv, UINT pc)
{
    HRESULT hr = d->lpVtbl->DrawPrimitive(d, t, sv, pc);
    if (s_j2c_ok && fl2_pass_begin(d)) { d->lpVtbl->DrawPrimitive(d, t, sv, pc); fl2_pass_end(d); }
    return hr;
}
/* characters: while a non-James character lit by James 1's flashlight (slot 0) is drawn, a free slot holds the same
   light at James 2's flashlight */
static int fl2_char_begin(u8 *work)
{
    if (!s_fl2_chars_ok || !s_opt_fl2 || s_fl2_mode == 2 || !s_fl2_j2 || !work || s_fl2_slot >= 0) return -1;
    int m = MODEL(work);
    if ((m >= 0x100 && m <= 0x103) || (m >= 0x120 && m <= 0x123) || m == 0x10E || m == 0x12E || (m >= 0x800 && m <= 0x82C)) return -1;
    u8 *s0 = (u8 *)0x00AA9CF8;
    if (*(int *)s0 != 3) return -1;
    const float *p0 = (const float *)(s0 + 0x20);
    if (fabsf(p0[0] - J1_TORCH[0]) > 2.0f || fabsf(p0[1] - J1_TORCH[1]) > 2.0f || fabsf(p0[2] - J1_TORCH[2]) > 2.0f) return -1;
    int k = -1;
    for (int i = 11; i >= 1 && k < 0; i--) if (*(int *)(s0 + i * 0x98) == 0) k = i;
    if (k < 0) { static int warned; if (!warned) { warned = 1; Log("flashlight 2: no free character light slot (all 12 used) - enemies here only get James 1's"); } return -1; }
    u8 *sk = s0 + k * 0x98;
    memcpy(s_fl2_slot_save, sk, 0x98);
    memcpy(sk, s0, 0x98);
    float *pk = (float *)(sk + 0x20);
    pk[0] = FL2_POS[0]; pk[1] = FL2_POS[1]; pk[2] = FL2_POS[2]; pk[3] = 1.0f;
    float dir[4] = { FL2_DIR[0], FL2_DIR[1], FL2_DIR[2], 0.0f };
    ((void (__cdecl *)(int, float *))0x0050C710)(k, dir);      /* the game's own: normalized direction into +0x40 */
    ((void (__cdecl *)(void))0x0050C190)();                    /* the game's own: all slots to view space */
    s_fl2_slot = k;
    return k;
}
static void fl2_char_end(int k)
{
    if (k < 0) return;
    memcpy((u8 *)0x00AA9CF8 + k * 0x98, s_fl2_slot_save, 0x98);
    s_fl2_slot = -1; s_fl2_chars++;
}
/* replaces the call of the per-character light pick (0x50DCED -> 0x50D1C0 = jmp 0x50D060) */
void __cdecl sel_hook(void *a, void *b)
{
    ((void (__cdecl *)(void *, void *))0x0050D060)(a, b);
    if (s_fl2_slot < 0) return;
    u8 *s0 = (u8 *)0x00AA9CF8, *sk = s0 + s_fl2_slot * 0x98, **list = (u8 **)0x00AAA5A8;
    int *cnt = (int *)0x00AAA420, i0 = -1, ik = -1;
    if (*cnt < 0 || *cnt > 12) return;
    for (int i = 0; i < *cnt; i++) { if (list[i] == s0) i0 = i; else if (list[i] == sk) ik = i; }
    if (ik >= 0) s_fl2_charsk++;
    if (i0 < 0 || ik < 0) return;
    int drop = i0 < ik ? ik : i0;                              /* the list is sorted strongest first */
    for (int i = drop; i + 1 < *cnt; i++) list[i] = list[i + 1];
    (*cnt)--;
}
#ifdef SH2COOP_TEST_TOOLS
/* cloud test: the character primitive draw (0x501F21): record each call with the work being drawn and grab the vertex
   shader constants of the one asked for */
unsigned s_tcc_log[256][4]; int s_tcc_n, s_tcc_arm, s_tcc_pc = -1, s_tcc_got; u32 s_tcc_work; float s_tcc_vsc[96 * 4];
HRESULT __stdcall chr_dip_cap(IDirect3DDevice8 *d, D3DPRIMITIVETYPE t, UINT mi, UINT nv, UINT si, UINT pc)
{
    if (s_tcc_arm) {
        u32 w = *(u32 *)0x01F7FC64;
        if (s_tcc_n < 256) { s_tcc_log[s_tcc_n][0] = w; s_tcc_log[s_tcc_n][1] = nv; s_tcc_log[s_tcc_n][2] = pc; s_tcc_log[s_tcc_n][3] = si; s_tcc_n++; }
        if ((int)pc == s_tcc_pc && (!s_tcc_work || s_tcc_work == w) && !s_tcc_got) { d->lpVtbl->GetVertexShaderConstant(d, 0, s_tcc_vsc, 96); s_tcc_got = 1; }
    }
    return d->lpVtbl->DrawIndexedPrimitive(d, t, mi, nv, si, pc);
}
#endif
static int patch_call6(u32 site, const char *expect_hex, void *target)
{
    DWORD old;
    if (!bytes_are(site, expect_hex)) { Log("hook at %08x NOT installed: bytes differ", site); return 0; }
    if (!VirtualProtect((void *)site, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *(u8 *)site = 0xE8; *(int32_t *)(site + 1) = (int32_t)((u32)target - (site + 5)); *(u8 *)(site + 5) = 0x90;
    VirtualProtect((void *)site, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)site, 6);
    return 1;
}
/* CS: is this work a James drawn as another character, or a weapon in such a James's hand?  Returns which James
   (0 / 1, *james = his work) or -1.  Never in cutscenes (James is shown there). */
static int chr_cut(void) { return *(int *)0x01F7A7C4 != 0 || (*(u32 *)0x01F7A7A4 & 0x40) || (*(u32 *)0x00932194 & 0x40); }
static u8 *chr_james_work(int who)
{
    if (who == 1) return s_active ? s_p2c : NULL;
    u8 *j1w = (u8 *)PLAYER;
    if (s_active && j1w == s_p2c) j1w = *(u8 **)s_p1.block;
    return j1w;
}
/* CS: the weapon a James holds right now (James 1: the game's weapon, instance 0; James 2: his copy), or NULL */
static u8 *chr_weapon_of(int who)
{
    if (who == 1) { u8 *w = s_w2[0]; return w && is_in_list(w) && (FLAGS(w) & F_DRAWN) ? w : NULL; }
    for (u8 *w = *(u8 **)0x01FB6984; w; w = *(u8 **)(w + W_NEXT)) {
        u32 m = MODEL(w);
        if (m >= 0x801 && m < 0x80C && w != s_w2[0] && w != s_w2[1] && *(u16 *)(w + W_INST) == 0 && (FLAGS(w) & F_DRAWN)) return w;
    }
    return NULL;
}
static int chr_owner(u8 *work, u8 **james)
{
    if (!work || chr_cut()) return -1;
    u32 m = MODEL(work);
    for (int who = 0; who < 2; who++) {
        if (s_chr_sel[who] == CHR_JAMES) continue;
        u8 *jw = chr_james_work(who);
        if (!jw || (MODEL(jw) & 0xFF00) != 0x100) continue;
        int mine = 0;
        if (work == jw) mine = 1;
        else if ((m >= 0x800 && m < 0x80C) || (m >= 0x820 && m < 0x82C)) {
            int copy = work == s_w2[0] || work == s_w2[1];
            mine = who == 1 ? copy : (!copy && *(u16 *)(work + W_INST) == 0 && !memcmp(work + 0x5C, jw + 0x5C, 64));
        }
        if (mine) { if (james) *james = jw; return who; }
    }
    return -1;
}
/* CS: replaces the call of the skinning-matrix build 0x509E50 (slot) in the character draw loop 0x50B980, which reads
   the work's bone nodes (the work is in 0x1F7FC64): the weapon of a James drawn as another character is moved into
   her hand for that build only, then its nodes are put back (the game's own uses of them stay James's) */
/* CS: replaces the call of the player's flashlight update 0x536D70 in the post-think pass (0x5386DF; it sets the light
   at player block +0x98 and the lens at +0xB8 from James's bones, then 0x512160 places the glow from +0xB8): for a James
   drawn as another character both move to the flashlight on her chest */
static void __cdecl chr_torch_hook(void)
{
    ((void (__cdecl *)(void))0x00536D70)();
    u8 *w = (u8 *)PLAYER, *jw; float d[3];
    int who = w ? chr_owner(w, &jw) : -1;
    if (who < 0 || jw != w || !chr_torch_delta(s_chr_sel[who], w, chr_weapon_of(who), d)) return;
    float *pl = (float *)0x01FB7D18, *lens = (float *)0x01FB7D38;
    for (int i = 0; i < 3; i++) { pl[i] += d[i]; lens[i] += d[i]; }
}
/* CS: footsteps. James's step sounds come from 0x54AD50 (step timing from his animation frames, one sound per foot by
   the floor material under it: foot probes at P+0x1D0 / P+0x1F8). The game's own playable Maria (model 0x10E) uses
   0x549500 instead, with her own heel sounds 0x4A57..0x4A5E. Her timing tables are for her animations, so James's
   timing is kept and only the two sound calls (0x54B3AB foot A, 0x54B355 foot B) are swapped for hers when the James
   who is thinking plays as Maria or Angela (Angela has no step sounds of her own; Eddie keeps James's). */
static int maria_step(int material)
{
    switch (material) {
    case 1: return 0x4A5B; case 2: return 0x4A58; case 5: return 0x4A5E; case 6: return 0x4A5D;
    case 7: return 0x4A5C; case 8: return 0x4A59; case 10: return 0x4A5A; default: return 0x4A57;
    }
}
static int step_heels(void)
{
    u8 *pl = (u8 *)PLAYER;
    int who = (s_active && pl == s_p2c) ? 1 : 0;
    int k = s_chr_sel[who];
    return (k == CHR_MARIA || k == CHR_ANGELA) && !chr_cut();
}
static int step_material(u32 hit, u32 ptr) { return *(int *)hit == 1 && *(u8 **)ptr ? *(int *)(*(u8 **)ptr + 8) : 0xC; }
static int __cdecl step_sfx_a(int id, float vol, void *pos, int x)
{
    if (step_heels()) { int was = id; id = maria_step(step_material(0x01FB7E50, 0x01FB7E74));
        static int said; if (!said) { said = 1; Log("characters: footstep sound 0x%x -> Maria's 0x%x", was, id); } }
    return ((int (__cdecl *)(int, float, void *, int))0x00516E90)(id, vol, pos, x);
}
static int __cdecl step_sfx_b(int id, float vol, void *pos, int x)
{
    if (step_heels()) id = maria_step(step_material(0x01FB7E78, 0x01FB7E9C));
    return ((int (__cdecl *)(int, float, void *, int))0x00516E90)(id, vol, pos, x);
}
static ChrWeaponSave s_wsave; static u8 *s_wsave_work;   /* weapon nodes moved for this draw (put back after it) */
static int __cdecl chr_skin_hook(u8 *slot)
{
    u8 *work = *(u8 **)0x01F7FC64, *jw;
    int who = work && (MODEL(work) & 0xFF00) == 0x800 ? chr_owner(work, &jw) : -1;
    if (who < 0) return ((int (__cdecl *)(u8 *))0x00509E50)(slot);
    if (s_wsave.n) chr_weapon_end(&s_wsave);
    if (chr_weapon_begin(s_chr_sel[who], jw, work, &s_wsave)) s_wsave_work = work;
    return ((int (__cdecl *)(u8 *))0x00509E50)(slot);
}
static void __cdecl draw_work_hook(u8 *work, int arg)
{
    {   /* CS: a James playing as another character: the mod draws her in his place (gameplay only, never in cutscenes) */
        int who = (s_active && work && work == s_p2c) ? 1 : (work && work == (u8 *)PLAYER && work != s_p2c) ? 0 : -1;
        if (who < 0 && s_active && work && PLAYER == s_p2c && work == *(u8 **)s_p1.block) who = 0;
        int cut = *(int *)0x01F7A7C4 != 0 || (*(u32 *)0x01F7A7A4 & 0x40) || (*(u32 *)0x00932194 & 0x40);
        if (who >= 0 && s_chr_sel[who] != CHR_JAMES && !cut && (MODEL(work) & 0xFF00) == 0x100) {
            static int said[2];
            if (chr_draw_world(D3D_DEVICE, s_chr_sel[who], work, chr_weapon_of(who), who)) {
                if (!said[who]) { said[who] = 1; Log("characters: James %d drawn as %s", who + 1, chr_name(s_chr_sel[who])); }
#ifdef SH2COOP_TEST_TOOLS
                extern float chr_test_offset[3];
                if (chr_test_offset[0] == 0 && chr_test_offset[1] == 0 && chr_test_offset[2] == 0)
#endif
                return;
            }
        }
    }
    if (s_active && work == s_p2c) hook_settex();
    /* P33t: which James is this?  In James 2's half of the split screen his block is swapped in, so PLAYER is
       James 2 there and James 1's work is the one saved in s_p1 (the user saw James 1 in his own colours only in
       James 1's half). */
    u8 *j1w = (u8 *)PLAYER;
    if (s_active && j1w == s_p2c) j1w = *(u8 **)s_p1.block;
    s_draw_p2 = (s_active && work && work == s_p2c) ? 2 : (work && work == j1w && work != s_p2c && ce_any(0)) ? 1 : 0;
    /* P33v: a James who plays as Maria / Eddie / Angela is shown as plain James where the game draws James himself
       (cutscenes): his Appearance colours belong to the James look he is not using, and the page hides them ("-") */
    if (s_draw_p2 && s_chr_sel[s_draw_p2 - 1] != CHR_JAMES) s_draw_p2 = 0;
    int k = fl2_char_begin(work);
    F_DRAW_WORK(work, arg);
    fl2_char_end(k);
    s_draw_p2 = 0;
    if (s_wsave.n && s_wsave_work == work) { chr_weapon_end(&s_wsave); s_wsave_work = NULL; }   /* CS: weapon nodes back */
}
static int hook_tex_lookup(void)
{
    DWORD old;
    if (!bytes_are(TEX_LOOKUP, "8b4c24048bc1")) { Log("look: texture lookup bytes differ, not installed"); return 0; }
    if (!VirtualProtect((void *)TEX_LOOKUP, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *(u8 *)TEX_LOOKUP = 0xE9; *(int32_t *)(TEX_LOOKUP + 1) = (int32_t)((u32)tex_stub - (TEX_LOOKUP + 5)); *(u8 *)(TEX_LOOKUP + 5) = 0x90;
    VirtualProtect((void *)TEX_LOOKUP, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)TEX_LOOKUP, 6);
    return 1;
}

/* ===================== P33: split-screen =====================
 * The Xbox builds 09-16 had it (side by side, top and bottom, diagonal); this is the same design re-traced on the PC
 * exe (12 Oct 2026, notes in the project doc claude/sh2-pc-coop-splitscreen.md); the PC keeps side by side and top and
 * bottom (the diagonal layout was removed in P33c). What the PC game does, verified:
 * - Gameplay frame = 0x476B40, called only at 0x4025C6 (0x401FC0, sub-mode 4); the frame ends in 0x476280 (links the
 *   -0x50 post node, executes the draw list 0x942348 with 0x5AFEE0), then 0x476100 EndScene/Present. Same steps as the
 *   Xbox FUN_000ebd60: camera 0x47CBA0, logic 0x476180 / 0x47B190 / 0x50AB80 / 0x50B590, then the render part
 *   0x47B3C0(0) lights, 0x479DA0 states + view/proj push, 0x47D340 map (drawn at once), 0x47D400 translucent map parts
 *   (draw list), 0x476680 world (characters drawn at once, flashlight stencil shadows, map effects 0x4C7650 and effect
 *   tasks 0x4EF9F0 into the draw list), 0x482F20 task layer 5, the complex fog 0x4896E0, and the static draw-list nodes.
 * - The game never sets a viewport during a frame (only at device reset 0x4F5C10) and nothing in the render part switches
 *   render targets, so a viewport set per view stays; the Enhanced Edition's soft shadows do switch targets inside the
 *   shadow pass - sh2coop_d9.c puts the view's viewport back each time.
 * - Camera state = 0x1F80C10..0x1F8DB6C (one block, same layout as the Xbox 0xfed330..0xff7f9c); the camera reads the
 *   player only through the player block, so James 2's camera = the game's camera update 0x5193A0 run with his block
 *   and his copy of the camera block. Shared with James 1's: the camera table pointer 0x1F811E4 and the script shake
 *   0x1F81210. Its two side effects outside the block: the enemy look timers (call 0x51FA71, run for James 1 only) and
 *   the stage camera's FOV def+0x60 (0x5223A0; James 1's value put back after James 2's pass).
 * - Per call (not per dt) steppers that must run once: map effects 0x4C7650, effect tasks 0x4EF9F0, the complex fog
 *   0x4896E0, task layer 5 0x482F20, the flash decay 0x50A2A0 (call 0x508E5B), the shadow casters (reset 0x5A59A0).
 *   They run in James 1's view and put their draw-list entries into a second list, which is linked into BOTH views'
 *   lists, so particles, blood, rain and fog show in both halves (sorted and faced for James 1's camera).
 * - The on-screen character list 0xA587F8 (0x204 bytes) is filled by the character pass and read by logic (event
 *   triggers 0x48E070, the map darkness check, enemy AI 0x4A8D70, James's aiming 0x5424A0): James 1's list stays the
 *   game's; James 2's own list is put in place while his think runs (his aiming follows what his half shows).
 * - The draw-list executor does not close its last state group on exit: each view links a do-nothing callback node at
 *   the list end (like the original -0x20 node), and James 2's view starts from the device state James 1's started from.
 */
#define F_FRAME_ORIG  ((VoidFn)0x00476B40)
#define F_CAM_FRAME   ((VoidFn)0x0047CBA0)
#define F_CAM_UPDATE  ((void (__cdecl *)(int, int, int, int, int, int, int, int))0x005193A0)
#define F_PROJ_COPY   ((VoidFn)0x005202B0)                 /* projection -> 0x1F80E40 */
#define F_PV_UPDATE   ((VoidFn)0x00520310)                 /* 0x1F80E80 = 0x1F80E40 x view 0x1F80E00 */
#define F_PUSH_VIEW   ((VoidFn)0x0047CB30)                 /* view + fov -> projection module, D3D VIEW / WORLD / PROJ */
#define F_FL_BASIS    ((VoidFn)0x0047D800)                 /* camera / flashlight basis 0x9438F0..0x94391F */
#define F_FL_FOG      ((VoidFn)0x0047C7A0)                 /* flashlight fog distance 0x942C34 */
#define F_SET_ASPECT  ((void (__cdecl *)(float))0x00458D30)
#define F_STAGE_PRE   ((VoidFn)0x00476180)
#define F_FL_AUTO     ((VoidFn)0x0047B190)
#define F_FL_LOGIC    ((VoidFn)0x0050AB80)
#define F_GAME_UPDATE ((VoidFn)0x0050B590)
#define F_LIGHTS      ((void (__cdecl *)(int))0x0047B3C0)
#define F_RSTATES     ((VoidFn)0x00479DA0)
#define F_MAPDRAW     ((VoidFn)0x0047D340)
#define F_MAP_TRANSL  ((VoidFn)0x0047D400)
#define F_WORLD       ((VoidFn)0x00476680)
#define F_LAYER5      ((VoidFn)0x00482F20)
#define F_GLARE_A     ((VoidFn)0x00489870)
#define F_GLARE_B     ((VoidFn)0x004896E0)
#define F_INGAME      ((int (__cdecl *)(void))0x0047AD40)
#define F_FOGROOM     ((int (__cdecl *)(void))0x004763C0)
#define F_FOGFLAG     ((int (__cdecl *)(void))0x0047C9B0)
#define F_OUTDOOR     ((int (__cdecl *)(int))0x004948D0)
#define F_POSTFX_ON   ((int (__cdecl *)(void))0x00478760)
#define F_LIST_EXEC   ((void (__cdecl *)(void *))0x005AFEE0)
#define F_LIST_RESET  ((void (__cdecl *)(void *))0x005AF470)
#define F_LIST_INIT   ((void (__cdecl *)(void *, int, int, void *))0x005AF330)
#define F_SHD_RESET   ((VoidFn)0x005A59A0)
#define F_MAPFX      ((VoidFn)0x004C7650)
#define F_TASKFX      ((VoidFn)0x004EF9F0)
#define F_NOISETEX    ((VoidFn)0x00478210)
#define F_ENEMY_TIMERS ((VoidFn)0x00519C50)
#define F_FLASH_DECAY ((VoidFn)0x0050A2A0)
#define SITE_FRAME    0x004025C6u
#define SITE_ETIMERS  0x0051FA71u
#define SITE_FLASHDEC 0x00508E5Bu
static const u32 kMapFxSites[] = { 0x004766B1, 0x004766EE }, kTaskFxSites[] = { 0x004766B6, 0x004766F3 }, kNoiseSites[] = { 0x004766C0, 0x004766FD };
#define DRAW_LIST     ((u8 *)0x00942348)
#define LIST_SLOTS    (*(u8 **)0x00942348)                 /* slot 0; slots -10..4105 are 8 bytes each */
#define LIST_SNAP_SZ  ((0x1000 + 2 * 10) * 8)
#define RING_BASE     (*(u32 *)0x0094116C)
#define RING_END      (*(u32 *)0x00941170)
#define RING_CUR      (*(u32 *)0x00941174)
#define VIS_LIST      ((u8 *)0x00A587F8)
#define VIS_SZ        0x204
#define GAME_DT       (*(float *)0x00932D48)
#define GAME_FDELTA   (*(int *)0x00932D54)
#define SKIP_ONCE     (*(u8 *)0x00AA9CF4)
#define RT_STATE      (*(int *)0x009423C0)
#define PROJ_ASPECT   (*(float *)0x01F5EEBC)
#define CAM_LO        0x01F80C10u
#define CAM_HI        0x01F8DB6Cu
#define CAM_SZ        (CAM_HI - CAM_LO)
#define CAM_OFF(a)    ((a) - CAM_LO)
#define CAM_FLAGS     (*(u32 *)0x01F811E8)
#define CAM_FOV       (*(float *)0x01F80FC0)
#define CAM_DEF       (*(u8 **)0x01F8B7B8)
#define DEMO_ID       (*(int *)0x01F7A7C4)

/* sh2coop_d9.c */
int d9_attach(void *dev8, void (*log)(const char *, ...));
int title_install(void (*log)(const char *, ...), int enable);   /* sh2coop_title.c */
void d9_view_begin(int x, int y, int w, int h);
void d9_view_end(void);
unsigned d9_reapplied(void);

static const char *kSplitName[SPLIT_MODES] = { "off (one shared screen)", "side by side", "top and bottom" };
static int s_split_ok;                       /* all split hooks are in */
static const char *s_split_why = "-";
static int s_cam_p2;                         /* James 2's camera pass is running (enemy timers stay James 1's) */
static int s_cam_fresh_prev;
static u8 s_cam1[CAM_SZ], s_cam2[CAM_SZ];
static u32 s_camtab[10];
static u8 s_vis1[VIS_SZ], s_vis2[VIS_SZ]; static int s_vis2_ok;
static u8 s_lsnap[LIST_SNAP_SZ];
static u8 s_fxmem[LIST_SNAP_SZ] __attribute__((aligned(16)));
static u8 s_fxlist[16];                      /* header of the effects list (same layout as 0x942348) */
static u8 s_hdr_keep[16];
typedef struct { int slot; u8 *first, *last; } FxRun;
static FxRun s_fxruns[0x1000 + 20]; static int s_nfxruns;
static u32 s_term[4];                        /* do-nothing callback node {next, 0x2C, fn, arg} */
static void __cdecl term_cb(void *a) { (void)a; }
static DevState s_split_sv;
static D3DVIEWPORT8 s_vp0;
static int s_vx[2], s_vy[2], s_vw[2], s_vh[2], s_gap, s_layout;
static float s_vaspect[2], s_vfovk[2];
/* statistics for the log (every 5 s) */
static DWORD s_sp_frames, s_sp_single, s_sp_fxruns, s_cam2_inits, s_ring_hi;
static double s_sp_ms;

/* one-time (per device) set-up of the Direct3D 9 viewport keeper; also used outside split frames (the P30 map re-run
   saves and restores the depth bias through it).  P33c: no Reset hooks any more - the mod no longer owns any
   D3DPOOL_DEFAULT object (those were the diagonal layout's), so a device Reset needs nothing from it. */
static void split_attach(IDirect3DDevice8 *d)
{
    if (!d || (void *)d == s_d9_dev8) return;
    if (s_d9_dev8) Log("split: the game's Direct3D device changed - the viewport keeper is attached to the new one");
    s_d9_dev8 = d;
    s_d9_ok = d9_attach(d, Log);
}

/* view rectangles in the frame target's pixels (vp0 = its full viewport); aspect and FOV factor per view */
static void split_rects(int mode, float aspect0)
{
    int X = (int)s_vp0.X, Y = (int)s_vp0.Y, W = (int)s_vp0.Width, H = (int)s_vp0.Height;
    int g = H / 180; if (g < 2) g = 2; g = (g + 1) & ~1; s_gap = g;
    if (mode == SPLIT_TOP) {
        int h2 = H / 2;
        s_vx[0] = X; s_vy[0] = Y; s_vw[0] = W; s_vh[0] = h2 - g / 2;
        s_vx[1] = X; s_vy[1] = Y + h2 + g / 2; s_vw[1] = W; s_vh[1] = H - h2 - g / 2;
    } else {
        int w2 = W / 2;
        s_vx[0] = X; s_vy[0] = Y; s_vw[0] = w2 - g / 2; s_vh[0] = H;
        s_vx[1] = X + w2 + g / 2; s_vy[1] = Y; s_vw[1] = W - w2 - g / 2; s_vh[1] = H;
    }
    for (int v = 0; v < 2; v++) {
        /* the projection's aspect is the screen's (the frame target can be a stretched copy texture): scale it by the
           part of the width / height this view gets.  Views wider than the screen keep the full screen's horizontal
           field of view (the game's projection distance h grows); narrower ones keep the vertical one. */
        s_vaspect[v] = aspect0 * ((float)s_vw[v] / (float)W) / ((float)s_vh[v] / (float)H);
        s_vfovk[v] = s_vaspect[v] > aspect0 ? s_vaspect[v] / aspect0 : 1.0f;
    }
    s_layout = mode;
}

/* the effects list: the per-call effects run once (in James 1's view) into this list; its entries are linked into
   both views' lists */
static void fx_list_reset(void)
{
    if (!*(u32 *)s_fxlist) F_LIST_INIT(s_fxlist, 0x1000, 10, s_fxmem);
    F_LIST_RESET(s_fxlist);
    s_nfxruns = 0;
}
static void fx_begin(void) { memcpy(s_hdr_keep, DRAW_LIST, 16); memcpy(DRAW_LIST, s_fxlist, 16); }
static void fx_end(void) { memcpy(DRAW_LIST, s_hdr_keep, 16); }
static void fx_collect(void)
{
    u8 *base = *(u8 **)s_fxlist;
    s_nfxruns = 0;
    for (int i = -10; i < 0x1000 + 10; i++) {
        u8 *end = i == -10 ? NULL : base + (i - 1) * 8, *p = *(u8 **)(base + i * 8), *last = NULL;
        int n = 0;
        while (p && p != end && n < 200000) { last = p; p = *(u8 **)p; n++; }
        if (last) { s_fxruns[s_nfxruns].slot = i; s_fxruns[s_nfxruns].first = *(u8 **)(base + i * 8); s_fxruns[s_nfxruns].last = last; s_nfxruns++; }
    }
    s_sp_fxruns += s_nfxruns;
}
static void fx_splice(void)
{
    u8 *mb = LIST_SLOTS;
    for (int k = 0; k < s_nfxruns; k++) {
        u8 *ms = mb + s_fxruns[k].slot * 8;
        *(u8 **)s_fxruns[k].last = *(u8 **)ms;
        *(u8 **)ms = s_fxruns[k].first;
    }
}
/* the call sites of the per-call effects inside 0x476680. (Written out per function on purpose: GCC 13 -O2 turned a
   shared helper taking the game function as a constant pointer argument into "unreachable" and dropped the calls.) */
static void __cdecl mapfx_hook(void)
{
    if (s_split_view < 0) { F_MAPFX(); return; }
    if (s_split_view == 1) return;                    /* James 2's view draws the entries made in James 1's */
    fx_begin(); F_MAPFX(); fx_end();
}
static void __cdecl taskfx_hook(void)
{
    if (s_split_view < 0) { F_TASKFX(); return; }
    if (s_split_view == 1) return;
    fx_begin(); F_TASKFX(); fx_end();
}
static void __cdecl noisetex_hook(void) { if (s_split_view != 1) F_NOISETEX(); }
static void __cdecl enemy_timers_hook(void) { if (!s_cam_p2) F_ENEMY_TIMERS(); }
static void __cdecl flash_decay_hook(void) { if (s_split_view != 1) F_FLASH_DECAY(); }

/* the complex fog / flashlight dust (0x4896E0) with the frame's own condition (0x476B7C..0x476BC3) */
static void glare_cond(void)
{
    int r;
    if (!*(u32 *)0x00A333F8) return;
    if (F_INGAME()) { if (!F_FOGROOM()) return; r = F_FOGFLAG(); }
    else { if (!F_FOGFLAG()) return; r = F_OUTDOOR(0) ? 1 : F_FOGROOM(); }
    if (!r) return;
    F_GLARE_A(); F_GLARE_B();
}
/* the frame's tail (0x476BC8..0x476C49): the static callback nodes */
static void link_tail(void)
{
    u8 *L = LIST_SLOTS;
    *(u32 *)0x00942360 = 0x2C; *(u32 *)0x0094235C = *(u32 *)(L + 0x8020); *(u32 *)(L + 0x8020) = 0x0094235C;
    L = LIST_SLOTS;
    *(u32 *)0x009423A0 = 0x2C; *(u32 *)0x0094239C = *(u32 *)(L - 0x20); *(u32 *)(L - 0x20) = 0x0094239C;
    L = LIST_SLOTS;
    *(u32 *)0x00942380 = 0x2C; *(u32 *)0x0094237C = *(u32 *)(L - 0x30); *(u32 *)(L - 0x30) = 0x0094237C;
    if (!F_POSTFX_ON()) {
        L = LIST_SLOTS;
        *(u32 *)0x00942370 = 0x2C; *(u32 *)0x0094236C = *(u32 *)(L - 0x40); *(u32 *)(L - 0x40) = 0x0094236C;
    }
}

/* camera-derived values the render part uses, for whatever camera / player / aspect is live now */
static void view_derived(void)
{
    F_PUSH_VIEW();
    F_PROJ_COPY();
    F_PV_UPDATE();
    memcpy((void *)0x009424D8, (void *)0x01F80E80, 64);
    F_FL_BASIS();
    F_FL_FOG();
}

/* James 1's camera as the game runs it, then James 2's: the game's camera update with his player block and his own copy
   of the camera block (no outputs outside the block; those are made per view) */
static void split_cameras(void)
{
    int fresh = (CAM_FLAGS & 0x40) != 0;             /* set only by the camera init at a room load (0x519320) */
    if (fresh && !s_cam_fresh_prev) s_cam2_valid = 0;
    s_cam_fresh_prev = fresh;
    F_CAM_FRAME();
    memcpy(s_cam1, (void *)CAM_LO, CAM_SZ);
    if (memcmp(s_camtab, (void *)0x00AC70F0, sizeof s_camtab)) { memcpy(s_camtab, (void *)0x00AC70F0, sizeof s_camtab); s_cam2_valid = 0; }
    if (!s_cam2_valid) {
        memcpy(s_cam2, s_cam1, CAM_SZ);
        *(u32 *)(s_cam2 + CAM_OFF(0x01F811E8)) |= 0x1C;   /* snap to James 2 on its first update */
        s_cam2_valid = 1; s_cam2_inits++;
    }
    *(u32 *)(s_cam2 + CAM_OFF(0x01F811E4)) = *(u32 *)(s_cam1 + CAM_OFF(0x01F811E4));
    *(u32 *)(s_cam2 + CAM_OFF(0x01F81210)) = *(u32 *)(s_cam1 + CAM_OFF(0x01F81210));
    /* values the camera frame copies in from global settings every frame, never written by the camera update:
       0x1F80FC4..D0 screen half / quarter size (device init 0x47CA90), 0x1F80FD4..E0 = 2.0 and the near / far planes
       from 0x94295C / 0x94294C / 0x942958 (0x47CBBF..0x47CBD5; rooms, scripts and the flashlight change those) */
    memcpy(s_cam2 + CAM_OFF(0x01F80FC4), s_cam1 + CAM_OFF(0x01F80FC4), 0x20);
    u8 *def1 = CAM_DEF; float fov1 = def1 ? *(float *)(def1 + 0x60) : 0.0f;
    save_state(&s_p1, 0); load_state(&s_p2, 0);
    memcpy((void *)CAM_LO, s_cam2, CAM_SZ);
    s_cam_p2 = 1;
    F_PROJ_COPY();
    F_CAM_UPDATE(0, 0, 0, 0, 0, 0, 0, 0);
    s_cam_p2 = 0;
    memcpy(s_cam2, (void *)CAM_LO, CAM_SZ);
    if (def1) *(float *)(def1 + 0x60) = fov1;
    memcpy((void *)CAM_LO, s_cam1, CAM_SZ);
    load_state(&s_p1, 0);
}

static void view_setup(IDirect3DDevice8 *d, int v)
{
    D3DVIEWPORT8 vp = { (DWORD)s_vx[v], (DWORD)s_vy[v], (DWORD)s_vw[v], (DWORD)s_vh[v], 0.0f, 1.0f };
    F_SET_ASPECT(s_vaspect[v]);
    CAM_FOV = CAM_FOV * s_vfovk[v];
    view_derived();
    d->lpVtbl->SetViewport(d, &vp);
    if (s_d9_ok) d9_view_begin(s_vx[v], s_vy[v], s_vw[v], s_vh[v]);
}
static void view_draw(int v)
{
    F_LIGHTS(0);
    F_RSTATES();
    F_MAPDRAW();
    F_MAP_TRANSL();
    F_WORLD();
    if (v == 0) { fx_begin(); glare_cond(); fx_end(); fx_collect(); }
}
static void view_execute(void)
{
    u8 *L = LIST_SLOTS;
    fx_splice();
    s_term[0] = *(u32 *)(L - 0x50); s_term[1] = 0x2C; s_term[2] = (u32)term_cb; s_term[3] = 0;
    *(u32 *)(L - 0x50) = (u32)s_term;
    F_LIST_EXEC(DRAW_LIST);
}

/* the on-screen list (0xA587F8: count, then up to 32 entries of 16 bytes, entry +0 = work; built by the character
   pass 0x508220..0x508350, capacity check at 0x5082C4) with the entries of a second list added (works not in it yet) */
static void vis_union(u8 *dst, const u8 *a, const u8 *b)
{
    memcpy(dst, a, VIS_SZ);
    int n = *(int *)dst, nb = *(const int *)b;
    n = n < 0 ? 0 : n > 32 ? 32 : n;
    nb = nb < 0 ? 0 : nb > 32 ? 32 : nb;
    for (int j = 0; j < nb && n < 32; j++) {
        const u8 *e = b + 4 + j * 16; u32 w = *(const u32 *)e; int have = 0;
        for (int i = 0; i < n && !have; i++) have = *(u32 *)(dst + 4 + i * 16) == w;
        if (!have) { memcpy(dst + 4 + n * 16, e, 16); n++; }
    }
    *(int *)dst = n;
}
static int s_vis_union;                       /* James 2 started an item / trigger this frame: next event main sees both halves */
static float s_fog_cam;                        /* flashlight fog end 0x942C34 as James 1's camera frame left it */
#define FL_FOG_END (*(float *)0x00942C34)

/* both views; returns 0 when the frame must be drawn the normal way (James 2 left during the update, no device) */
static int split_views(void)
{
    IDirect3DDevice8 *d = D3D_DEVICE;
    LARGE_INTEGER t0, t1, fq;
    if (!d || !s_active || !s_p2c || !is_in_list(s_p2c) || s_p2c == PLAYER) return 0;
    if (d->lpVtbl->GetViewport(d, &s_vp0) != D3D_OK || s_vp0.Width < 64 || s_vp0.Height < 64) return 0;
    QueryPerformanceCounter(&t0);
    split_attach(d);
    int mode = s_split_cfg;
    if (mode == SPLIT_OFF) return 0;                  /* switched off during this frame's update (F8 / L3+R3 / menu) */
    if (mode != SPLIT_TOP) mode = SPLIT_SIDE;
    float aspect0 = PROJ_ASPECT;
    if (!(aspect0 > 0.1f && aspect0 < 10.0f)) aspect0 = (float)s_vp0.Width / (float)s_vp0.Height;
    split_rects(mode, aspect0);
    { static int last_mode = -1; static DWORD last_w, last_h; static int last_rt = -1;
      if (mode != last_mode || s_vp0.Width != last_w || s_vp0.Height != last_h || RT_STATE != last_rt) {
          Log("split: %s on a %lux%lu target (render target state %d, aspect %.3f): view 1 %d,%d %dx%d aspect %.3f fov x%.2f | view 2 %d,%d %dx%d aspect %.3f fov x%.2f | gap %d px | Direct3D 9 viewport keeper %s",
              kSplitName[mode], (unsigned long)s_vp0.Width, (unsigned long)s_vp0.Height, RT_STATE, aspect0,
              s_vx[0], s_vy[0], s_vw[0], s_vh[0], s_vaspect[0], s_vfovk[0], s_vx[1], s_vy[1], s_vw[1], s_vh[1], s_vaspect[1], s_vfovk[1], s_gap, s_d9_ok ? "on" : "OFF");
          last_mode = mode; last_w = s_vp0.Width; last_h = s_vp0.Height; last_rt = RT_STATE; } }

    memcpy(s_cam1, (void *)CAM_LO, CAM_SZ);          /* James 1's camera after the update (scripts may have moved it) */
    memcpy(s_lsnap, LIST_SLOTS - 0x50, LIST_SNAP_SZ); /* entries made before the views (e.g. enemy billboards in the update) */
    u8 skip = SKIP_ONCE;
    /* flashlight fog end: in the game it is set by the camera frame (0x47C7A0) and may then be overridden by the
       update (stage callbacks via 0x47C770, the flashlight logic 0x47B990).  View 1 uses exactly what the game would;
       view 2 gets James 2's own distance-based value unless the update overrode it. */
    float fog_upd = FL_FOG_END; int fog_ovr = fog_upd != s_fog_cam;
    fx_list_reset();
    dev_save(d, &s_split_sv);

    /* ---- view 1: James 1 ---- */
    s_phase = "split: James 1's view";
    s_split_view = 0;
    view_setup(d, 0);
    FL_FOG_END = fog_upd;
    view_draw(0);
    view_execute();
    memcpy(LIST_SLOTS - 0x50, s_lsnap, LIST_SNAP_SZ);
    memcpy(s_vis1, VIS_LIST, VIS_SZ);
    if (s_d9_ok) d9_view_end();
    u8 skip_after1 = SKIP_ONCE;                       /* what the game's single view would leave for the next frame */

    /* ---- view 2: James 2 (his block and camera; nothing advances a second time) ---- */
    s_phase = "split: James 2's view";
    save_state(&s_p1, 0); load_state(&s_p2, 0);
    /* James 2 moved by the co-op rules during the update (regroup, room rule, fall guard...): his camera restarts
       next frame; this frame his half shows James 1's camera, which is where he was put */
    memcpy((void *)CAM_LO, s_cam2_valid ? s_cam2 : s_cam1, CAM_SZ);
    float dt = GAME_DT; int fdelta = GAME_FDELTA;
    GAME_DT = 0.0f; GAME_FDELTA = 0;
    F_SHD_RESET();
    SKIP_ONCE = skip;                                 /* the newly equipped weapon stays hidden in both halves alike */
    dev_restore(d, &s_split_sv);
    s_split_view = 1;
#ifdef SH2COOP_TEST_TOOLS
    if (s_tview_on) { memcpy((void *)0x01F80E00, s_tview, 64); CAM_FOV = s_tview_fov; }   /* cloud test: close-up camera in the right half */
#endif
    view_setup(d, 1);
    if (fog_ovr) FL_FOG_END = fog_upd;
    view_draw(1);
    view_execute();
    F_LIST_RESET(DRAW_LIST);
    memcpy(s_vis2, VIS_LIST, VIS_SZ); s_vis2_ok = 1;
    if (s_d9_ok) d9_view_end();
    SKIP_ONCE = skip_after1;
    s_split_view = -1;
    GAME_DT = dt; GAME_FDELTA = fdelta;
    memcpy((void *)CAM_LO, s_cam1, CAM_SZ);
    load_state(&s_p1, 0);
    if (s_vis_union && s_vis2_ok) vis_union(VIS_LIST, s_vis1, s_vis2);   /* James 2's pick-up: his half counts too */
    else memcpy(VIS_LIST, s_vis1, VIS_SZ);
    s_vis_union = 0;

    /* ---- full screen again, James 1's camera, for the 2D / fades / post part at the end of the frame ---- */
    s_phase = "split: back to full screen";
    d->lpVtbl->SetViewport(d, &s_vp0);
    F_SET_ASPECT(aspect0);
    view_derived();
    FL_FOG_END = fog_upd;
    F_LIGHTS(0);
    F_RSTATES();
    {   /* the black divider between the halves (Clear with one rectangle) */
        int X = (int)s_vp0.X, Y = (int)s_vp0.Y, W = (int)s_vp0.Width, H = (int)s_vp0.Height;
        D3DRECT r;
        if (mode == SPLIT_TOP) { r.x1 = X; r.x2 = X + W; r.y1 = s_vy[0] + s_vh[0]; r.y2 = s_vy[1]; }
        else { r.y1 = Y; r.y2 = Y + H; r.x1 = s_vx[0] + s_vw[0]; r.x2 = s_vx[1]; }
        if (r.x2 > r.x1 && r.y2 > r.y1) d->lpVtbl->Clear(d, 1, &r, D3DCLEAR_TARGET, 0xFF000000u, 1.0f, 0);
    }
    u32 used = RING_CUR - RING_BASE; if (used > s_ring_hi) s_ring_hi = used;
    QueryPerformanceCounter(&t1); QueryPerformanceFrequency(&fq);
    s_sp_ms += (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)fq.QuadPart;
    s_sp_frames++;
    return 1;
}

/* Cutscenes and scripted shots are one shared full screen.  Everything that starts them runs in the event main
   0x48E800, which the play handler calls (0x4025B9) right before the gameplay frame (0x4025C6), so this test sees the
   current frame's state:
   - real-time cutscenes: demo_run 0x4A2110 sets the demo id 0x1F7A7C4 and 0x932194 |= 0x40 every frame it runs
     (both cleared at the start of each event main, 0x48E858 / 0x48E867), its camera 0x4A09F0 sets 0x1F7A7A4 |= 0x40
     (cleared by the post-update loop 0x538B97); stage scripts also set the demo id directly for scripted sequences;
   - scripted camera shots: 0x51D280 sets camera flags 1 / 2 (0x519A00 / 0x519950), released by 0x51D520;
   - area scripts: event kind 2.  Pre-rendered movies run in other play modes (0xE / 0xF) and never reach this frame.
   James 2 also steps out whenever James 1 loses control (doors, scenes), which ends the split by itself. */
#define GAME_FLAGS (*(u32 *)0x00932194)
static void __cdecl p2_think(u8 *c);
static int split_wanted(u8 *p1)
{
    if (!s_split_ok) { s_split_why = "split hooks not installed"; return 0; }
    if (s_split_cfg == SPLIT_OFF) { s_split_why = "switched off"; return 0; }
    if (!s_active || !s_p2c || !p1 || s_p2c == p1 || !is_in_list(s_p2c) || *(u32 *)(s_p2c + W_THINK) != (u32)p2_think)
        { s_split_why = "James 2 not in play"; return 0; }
    if (DEMO_ID != 0 || (DEMO_FLAGS & 0x40) || (GAME_FLAGS & 0x40)) { s_split_why = "cutscene"; return 0; }
    if (CAM_FLAGS & 3) { s_split_why = "scripted camera"; return 0; }
    /* P33r: EV_KIND is the handler's sub-step, 2 = "running" for scripts (states 2/4) but also "message up" for the
       short interactions (6/7/8/10/11), during which James 2 now stays (frozen) - keep the split for those */
    if (EV_KIND == 2 && !(EV_STATE == 6 || EV_STATE == 7 || EV_STATE == 8 || EV_STATE == 10 || EV_STATE == 11)) { s_split_why = "scene / script"; return 0; }
    IDirect3DDevice8 *d = D3D_DEVICE; D3DVIEWPORT8 vp;
    if (!d || d->lpVtbl->GetViewport(d, &vp) != D3D_OK || vp.Width < 64 || vp.Height < 64) { s_split_why = "no usable screen"; return 0; }
    s_split_why = "on";
    return 1;
}

/* replaces the call of the gameplay frame 0x476B40 (at 0x4025C6) */
static int s_split_log_on;
static void __cdecl split_frame_hook(void)
{
    u8 *p1 = PLAYER;
    if (!split_wanted(p1)) {
        if (s_split_log_on) { Log("split: one screen again (%s)", s_split_why); s_split_log_on = 0; }
        s_split_now = 0; s_cam2_valid = 0; s_vis2_ok = 0; s_vis_union = 0;
        F_FRAME_ORIG();
        return;
    }
    if (!s_split_log_on) { Log("split: on (%s)", kSplitName[s_split_cfg]); s_split_log_on = 1; }
    s_split_now = 1;
    const char *outer = (const char *)s_phase;
    s_phase = "split: cameras";
    split_cameras();
    s_fog_cam = FL_FOG_END;
    s_phase = "split: game update";
    F_STAGE_PRE(); F_FL_AUTO(); F_FL_LOGIC(); F_GAME_UPDATE();
    if (!split_views()) {                            /* James 2 went away during the update: this frame as the game draws it */
        s_phase = "split: single view";
        F_LIGHTS(0); F_RSTATES(); F_MAPDRAW(); F_MAP_TRANSL(); F_WORLD(); F_LAYER5(); glare_cond();
        s_sp_single++;
        s_split_now = 0; s_cam2_valid = 0; s_vis2_ok = 0; s_vis_union = 0;   /* nothing of this frame was split */
    } else F_LAYER5();
    link_tail();
    s_phase = outer;
}

static void split_set(int mode, const char *by)
{
    s_split_cfg = (mode % SPLIT_MODES + SPLIT_MODES) % SPLIT_MODES;
    if (s_ini[0]) WritePrivateProfileStringA("split", "layout", s_split_cfg == SPLIT_OFF ? "off" : s_split_cfg == SPLIT_TOP ? "top" : "side", s_ini);
    Log("split: layout %s (%s)", kSplitName[s_split_cfg], by);
}
/* side -> top -> off -> side (F8, L3+R3, menu right); menu left goes the other way */
static void split_cycle(int dir, const char *by) { split_set(s_split_cfg + (dir < 0 ? SPLIT_MODES - 1 : 1), by); }

static void split_log5s(void)
{
    if (!s_split_ok) return;
    Log("split: last 5 s: %lu split frames (%s now, %s), %.2f ms per frame for both views, %lu single-view fallbacks, effect runs linked %lu, James 2 camera (re)starts %lu, draw ring high-water %lu of %lu KB, viewport kept after %u render-target switches",
        (unsigned long)s_sp_frames, kSplitName[s_split_cfg], s_split_why, s_sp_frames ? s_sp_ms / s_sp_frames : 0.0, (unsigned long)s_sp_single,
        (unsigned long)s_sp_fxruns, (unsigned long)s_cam2_inits, (unsigned long)(s_ring_hi >> 10), (unsigned long)((RING_END - RING_BASE) >> 10),
        d9_reapplied());
    if (s_sp_frames)
        Log("split: other James's flashlight beam added - James 1's half: %lu draws (not added %lu, last reason: %s) | James 2's half: %lu draws (not added %lu, last reason: %s)",
            (unsigned long)s_fl2_vok[1], (unsigned long)s_fl2_vno[1], s_fl2_vwhy[1], (unsigned long)s_fl2_vok[2], (unsigned long)s_fl2_vno[2], s_fl2_vwhy[2]);
    memset(s_fl2_vok, 0, sizeof s_fl2_vok); memset(s_fl2_vno, 0, sizeof s_fl2_vno); s_fl2_vwhy[0] = s_fl2_vwhy[1] = s_fl2_vwhy[2] = "-";
    s_sp_frames = s_sp_single = s_sp_fxruns = s_cam2_inits = s_ring_hi = 0; s_sp_ms = 0;
}

/* P33j: James 2 can't be grabbed - grabs pass through him (asked by the player: "only James 1").
   Grabs are shape-2 attack volumes (attack table 0x6EC380: 0x35 Pyramid Head, 0x3C Flesh Lip, 0x3F Abstract Daddy,
   0x41/0x44 Mandarin).  The collision step 0x528210 sends shape 2 to the sweep 0x527F10 (call at 0x52826E; volume copy
   at the caller's [ebp-0x40]: +0 shape, +2 id+0x100, +4 attacker), which tests the bodies of the cylinder bank
   0x1F91760 + b*0x1880 (b = ([0x1F8F25C] == 0), count 0x1F8F260[b], 0xC4 per body, work* at +0x60) in 0x525060 and
   SKIPS a body whose word +0xBE is 0 (0x5250AC).  For a grab volume, James 2's body gets +0xBE = 0 for that one call,
   so the sweep goes through him: it catches James 1 behind him, or misses, exactly as if James 2 were not there.
   Everything else (other attacks, holds, James 2's own hits) is untouched. */
int s_gs_ret, s_gs_orig = 0x00527F10;
static u8 *s_gs_rec; static u16 s_gs_keep; static unsigned long s_gs_n; static DWORD s_gs_logt;
#ifdef SH2COOP_TEST_GS_J1
#define GS_WHO PLAYER                          /* test build: hide James 1 from every shape-2 attack */
#define GS_ID_OK(id) 1
#else
#define GS_WHO s_p2c
#define GS_ID_OK(id) ((id) == 0x35 || (id) == 0x3C || (id) == 0x3F || (id) == 0x41 || (id) == 0x44)
#endif
void __cdecl grab_hide(u8 *fr)
{
    s_gs_rec = NULL;
    u8 *who = GS_WHO;
    if (!who || (!s_active && who == s_p2c)) return;
    int id = *(u8 *)(fr - 0x3E);
    if (!GS_ID_OK(id)) return;
    int b = *(u32 *)0x01F8F25C == 0, n = *(int *)(0x01F8F260 + b * 4);
    if (n <= 0 || n > 32) return;
    u8 *base = (u8 *)(0x01F91760 + b * 0x1880);
    for (int i = 0; i < n; i++) {
        u8 *r = base + i * 0xC4;
        if (*(u8 **)(r + 0x60) != who) continue;
        s_gs_rec = r; s_gs_keep = *(u16 *)(r + 0xBE); *(u16 *)(r + 0xBE) = 0; s_gs_n++;
        DWORD now = GetTickCount();
        if (now - s_gs_logt > 2000) {
            u8 *att = *(u8 **)(fr - 0x3C);
            Log("grab 0x%x by model 0x%x passed through %s (it can only catch James 1) - %lu sweeps so far",
                id, att ? (unsigned)(u16)MODEL(att) : 0u, who == s_p2c ? "James 2" : "James 1 (TEST BUILD)", s_gs_n);
            s_gs_logt = now;
        }
        return;
    }
}
void __cdecl grab_unhide(void)
{
    if (s_gs_rec) { *(u16 *)(s_gs_rec + 0xBE) = s_gs_keep; s_gs_rec = NULL; }
}
extern void grab_sweep_stub(void);
__asm__(
    ".text\n"
    "_grab_sweep_stub:\n"
    "    pushal\n"
    "    pushl %ebp\n"                     /* the caller's frame: volume copy at -0x40 */
    "    call _grab_hide\n"
    "    addl $4, %esp\n"
    "    popal\n"
    "    popl _s_gs_ret\n"                 /* same stack and registers for the game's sweep */
    "    call *_s_gs_orig\n"
    "    pushl _s_gs_ret\n"
    "    pushal\n"                         /* keeps the sweep's eax */
    "    call _grab_unhide\n"
    "    popal\n"
    "    ret\n"
);

/* P33n: holds pass through James 2 as well.  Shapes 5/6 go to 0x526CD0 and then ONE hit record 0x523460 (call at
   0x52839B, args: (rec, out), rec+4 = the victim's work); a hold volume (ids below, same frame layout as above:
   id at [ebp-0x3E], attacker at [ebp-0x3C]) whose victim is James 2 gets no record - nothing is changed for James 1,
   other attacks or other victims.  0x528435 (after the call) does not use eax. */
int s_hold_orig = 0x00523460, s_hold_skip;
static unsigned long s_hold_n; static DWORD s_hold_logt;
#define HOLD_ID_OK(id) ((id) == 0x36 || (id) == 0x3D || (id) == 0x40 || (id) == 0x42 || (id) == 0x45)
int __cdecl hold_skip(u8 *rec, u8 *fr)
{
    if (!s_active || !s_p2c || !rec) return 0;
    int id = *(u8 *)(fr - 0x3E);
    if (!HOLD_ID_OK(id) || *(u8 **)(rec + 4) != s_p2c) return 0;
    s_hold_n++;
    DWORD now = GetTickCount();
    if (now - s_hold_logt > 2000) {
        u8 *att = *(u8 **)(fr - 0x3C);
        Log("hold 0x%x by model 0x%x passed through James 2 (only James 1 can be held) - %lu so far", id, att ? (unsigned)(u16)MODEL(att) : 0u, s_hold_n);
        s_hold_logt = now;
    }
    return 1;
}
extern void hold_rec_stub(void);
__asm__(
    ".text\n"
    "_hold_rec_stub:\n"
    "    pushal\n"
    "    pushl %ebp\n"                     /* the caller's frame: volume copy at -0x40 */
    "    pushl 40(%esp)\n"                 /* the record's first argument */
    "    call _hold_skip\n"
    "    addl $8, %esp\n"
    "    movl %eax, _s_hold_skip\n"
    "    popal\n"
    "    cmpl $0, _s_hold_skip\n"
    "    jne 1f\n"
    "    jmp *_s_hold_orig\n"              /* same stack: the game's record function returns to the caller */
    "1:  ret\n"
);

/* P33i: James 2 caught in a grab meant for James 1.
   Every grabber targets James 1 only (select_target_hook keeps Flesh Lip 0x203, Abstract Daddy 0x204, Mandarin 0x206,
   Pyramid Head 0x208/0x209 on him), but their grabs are shape-2 sweeps (attack table 0x6EC380: 0x35, 0x3C, 0x3F,
   0x41, 0x44) that catch the NEAREST body (0x528258 -> 0x527F10 -> one 0x523460 record), so James 2 standing in the
   way is caught instead.  His own think then starts a normal grab in his block (kind-4 hit 0x54512A: +0x114 = id,
   +0x138 = attacker; 0x5340B0: phase 1, meter 0.8 for Flesh Lip on Normal).  The grabber, however, keeps holding
   only while JAMES 1's struggle meter is up (0x4A6500 = 0x543210 reads 0x1FB80D4 outside the swap: Flesh Lip
   0x4B9AB0, Abstract Daddy 0x4B6D40.., Mandarin 0x4B55BF.., Pyramid Head 0x4C0542/0x4C0776/0x4C1078) - so it lets go
   on its first hold frame and James 2 is left struggling against nobody (~50 presses on Normal at 60 fps).
   Rule: while James 2's grab is at phase 1/2 and James 1's meter (s_p1, saved at the top of p2_think) is 0, James 2's
   meter is set to 0 -> the game's own escape runs (phase 2 -> 3, release animation -> 0).  If James 1 is held at the
   same time (his meter up), the grabber really holds, and James 2 struggles as usual.  Also logs James 2's grab steps. */
static void p2_grab_rule(u8 *c)
{
    static int last = -1;
    int phase = *(u8 *)0x01FB811A;
    float m = *(float *)0x01FB80D4, m1 = *(float *)(s_p1.block + 0x454);
    u8 *by = *(u8 **)(c + 0x138);
    if (phase != last) {
        if (last >= 0 || phase) Log("James 2 grab phase %d -> %d (attack 0x%x, by model 0x%x, struggle meter %.2f, hp %.1f, James 1's meter %.2f)",
            last < 0 ? 0 : last, phase, *(u16 *)(c + 0x114), by ? (unsigned)(u16)MODEL(by) : 0u, m, *(float *)(c + W_HP), m1);
        last = phase;
    }
    if ((phase == 1 || phase == 2) && m != 0.0f && m1 == 0.0f) {
        *(float *)0x01FB80D4 = 0.0f;
        Log("James 2 caught by model 0x%x in James 1's place: the grabber only holds while James 1 struggles, so James 2 is let go too",
            by ? (unsigned)(u16)MODEL(by) : 0u);
    }
}

static void __cdecl p2_think(u8 *c)
{
    const char *outer = (const char *)s_phase;
    u8 *p1 = PLAYER;
    if (c == p1) { *(u32 *)(c + W_THINK) = (u32)F_JAMES_THINK; F_JAMES_THINK(c); return; }   /* never drive James 1 */
    if (!p1 || !(FLAGS(p1) & F_CONTROL) || (FLAGS(p1) & F_LOCKED) || s_freeze) return;       /* frozen while James 1 is */
    /* P33d: downed as soon as his HP is gone.  The old test also wanted pending damage (+0x11C) still set, but on the PC
       the hit records are applied and cleared INSIDE James's think (0x535480 -> 0x544F90), so at this point the pending
       damage is always 0 again: James 2 lay dead at 0 HP and never came back (cloud test, Pyramid Head stairwell). */
    if (*(float *)(c + W_HP) <= 0.0f || s_p2.block[0x497] != 0) { s_downed = 1; return; }
    u32 dmg = DMG_STAT;
    s_inP2 = 1;
    s_phase = "James 2 think: swap in";
    save_state(&s_p1, 1);
    load_state(&s_p2, 1);
    if (!s_w2[0] && !s_w2[1]) { *(u8 *)(0x01FB7C80 + BLK_WEAPON) = 0; *(u8 *)(0x01FB7C80 + BLK_WCLASS) = 0; }   /* no copy: unarmed */
    u8 *w1[2]; short keep[2] = { 0, 0 };
    w1_hide(w1, keep);                                   /* his lookups must find his copy */
    /* P33 split: James 2 steers relative to HIS camera (0x547FA0: eye 0x1F8123C via 0x519B60, look target / angles
       0x1F81278..0x1F81297 via 0x519AA0, settled flag 0x1F8DB64 via 0x5190D0 - read only), his sounds pan with his
       view 0x1F80E00, and his aiming (0x5424A0) sees what his half of the screen showed last frame */
    int cam_swap = s_split_now && s_cam2_valid, vis_swap = cam_swap && s_vis2_ok;
    u8 keep_view[0x40], keep_eye[0x1F81298 - 0x1F8123C], keep_vis[VIS_SZ]; u32 keep_settled = 0;
    if (cam_swap) {
        memcpy(keep_view, (void *)0x01F80E00, sizeof keep_view); memcpy((void *)0x01F80E00, s_cam2 + CAM_OFF(0x01F80E00), sizeof keep_view);
        memcpy(keep_eye, (void *)0x01F8123C, sizeof keep_eye); memcpy((void *)0x01F8123C, s_cam2 + CAM_OFF(0x01F8123C), sizeof keep_eye);
        keep_settled = *(u32 *)0x01F8DB64; *(u32 *)0x01F8DB64 = *(u32 *)(s_cam2 + CAM_OFF(0x01F8DB64));
    }
    if (vis_swap) { memcpy(keep_vis, VIS_LIST, VIS_SZ); memcpy(VIS_LIST, s_vis2, VIS_SZ); }
    s_phase = "James 2 think: game's James code running for James 2";
    F_JAMES_THINK(c);
    s_phase = "James 2 think: swap out";
    p2_grab_rule(c);
    if (vis_swap) memcpy(VIS_LIST, keep_vis, VIS_SZ);
    if (cam_swap) {
        memcpy((void *)0x01F80E00, keep_view, sizeof keep_view);
        memcpy((void *)0x01F8123C, keep_eye, sizeof keep_eye);
        *(u32 *)0x01F8DB64 = keep_settled;
    }
    w1_show(w1, keep);
    save_state(&s_p2, 1);
    load_state(&s_p1, 1);
    s_inP2 = 0;
    DMG_STAT = dmg;
    relink_head(p1);
    s_phase = outer;
}


/* ---- bone-node pool: James's code addresses his bones as ONE block (node k = head + k*0xE0: 0x555920, 0x555A30,
   0x555E00, 0x555F10 switch bones on/off that way). The allocator 0x53ABD0 just pops the free list, so a James spawned
   after the list got shuffled (cutscene actors freed / re-made) gets scattered nodes and the switches land on the
   wrong nodes -> his skeleton sinks / pops every few frames. Before a spawn, move a run of consecutive free nodes
   (ascending) to the front of the free list. ---- */
/* P19: the game's pool is 400 nodes, sized for ONE James. In crowded areas (outside the apartments: 16 characters)
   the game itself uses ~390 of them, so James 2 (41) could not join and - worse - James 2 holding 41 nodes could make
   the game's own enemy / cutscene-actor spawns fail. James 2 now brings his own nodes: blocks of XB_N extra nodes (in
   the mod's memory) are added to the game's free list when he joins. Nothing in the exe checks node addresses (the
   only reference to the pool base is its init 0x53AAF0), nodes are reached through pointers only. The extra blocks
   are handed out again after every pool init (0x53AAF0 at new game / load, hooked), when the game forgets them. */
#define XB_N    64
#define XB_MAX  6
#define IDX_N   (POOL_N + XB_N * XB_MAX)
static u8 s_xpool[XB_N * XB_MAX * NODE_SZ] __attribute__((aligned(64)));
static int s_xb_used;
static u8 *node_at(int i) { return i < POOL_N ? POOL_BASE + i * NODE_SZ : s_xpool + (i - POOL_N) * NODE_SZ; }
static int node_idx(u8 *p)
{
    if (p >= POOL_BASE && p < POOL_BASE + POOL_N * NODE_SZ) { u32 o = (u32)(p - POOL_BASE); return o % NODE_SZ ? -1 : (int)(o / NODE_SZ); }
    if (p >= s_xpool && p < s_xpool + s_xb_used * XB_N * NODE_SZ) { u32 o = (u32)(p - s_xpool); return o % NODE_SZ ? -1 : POOL_N + (int)(o / NODE_SZ); }
    return -1;
}
static int node_known(u8 *p) { return node_idx(p) >= 0 || is_private_node(p); }
static int pool_add_block(const char *why)
{
    if (s_xb_used >= XB_MAX) { Log("bone pool: all %d extra blocks already in use (%s)", XB_MAX, why); return 0; }
    u8 *b = s_xpool + s_xb_used * XB_N * NODE_SZ;
    memset(b, 0, XB_N * NODE_SZ);
    for (int k = 0; k < XB_N - 1; k++) *(u8 **)(b + k * NODE_SZ) = b + (k + 1) * NODE_SZ;
    *(u8 **)(b + (XB_N - 1) * NODE_SZ) = POOL_HEAD;              /* the block goes in front of the game's free nodes */
    POOL_HEAD = b; POOL_FREE += XB_N; s_xb_used++;
    Log("bone pool: added %d extra nodes (block %d, %s) -> free %d", XB_N, s_xb_used, why, POOL_FREE);
    return 1;
}
/* ===================== P29: James 2's bones never come from the game's pool =====================
   Traced in sh2pc.exe (11 Oct): the whole life of a bone chain goes through four places only -
     alloc   0x53ABD0, ONE call site 0x537BA1 (in 0x537B50, run once per work when work+0x220 == 0; reached from the
             spawner 0x48F810 -> 0x53A620 -> 0x507530 and from the "model finished loading" step 0x53A7C0),
     free    0x53AB40, ONE call site 0x53843A (in 0x5383F0, the only work free),
     reserve 0x53AB80 / 0x53ABA0 (counter only, 0x53A921 / 0x53A7FB / 0x538429),
     reset   0x538050 (all 32 works wiped) + 0x53AAF0 (pool rebuilt), always together (0x408347/0x40834C, 0x53A780).
   James's post pass needs his chain to be ONE block (node k = head + k*0xE0). Every "James 2 sinks into the floor"
   so far was a scattered chain. So instead of arranging the game's free list before each join (P13-P28: fails when
   the game holds reservations, P28 log), James 2 and his weapon copies now get their nodes from slots in the mod's
   own memory: always one block, and the game's pool stays exactly as in the unmodded game. */

static int priv_slot_of(u8 *p, int *kind)
{
    for (int i = 0; i < PB_J2_SLOTS; i++) if (p >= s_pbj[i] && p < s_pbj[i] + PB_J2_N * NODE_SZ) { *kind = PRIV_J2; return i; }
    for (int i = 0; i < PB_W_SLOTS; i++) if (p >= s_pbw[i] && p < s_pbw[i] + PB_W_N * NODE_SZ) { *kind = PRIV_WPN; return i; }
    return -1;
}
static int is_private_node(u8 *p) { int k; return priv_slot_of(p, &k) >= 0; }
/* same result as the game's 0x53ABD0: count nodes linked by +0, parent (+4) = node[parents[k]] (0xFF = none),
   +0xB8 = 0, last node's link = 0, first node +0x94 = 0. Nodes are zeroed like the pool init (0x53AAF0) does. */
static u8 *priv_build(u8 *base, int count, const u8 *parents)
{
    memset(base, 0, count * NODE_SZ);
    for (int k = 0; k < count; k++) {
        u8 *n = base + k * NODE_SZ, p = parents ? parents[k] : 0xFF;
        *(u8 **)n = (k + 1 < count) ? n + NODE_SZ : NULL;
        *(u8 **)(n + 4) = (p == 0xFF || p >= count) ? NULL : base + p * NODE_SZ;
        *(u32 *)(n + 0xB8) = 0;
    }
    *(u32 *)(base + 0x94) = 0;
    return base;
}
static int priv_live(u8 *own, u8 *base) { return own && is_in_list(own) && *(u8 **)(own + 0x9C) == base; }
u8 *__cdecl bone_alloc_c(int count, const u8 *parents, u8 *work)
{
    int want = s_priv_want;
    if (want && count > 0 && work) {
        int m = MODEL(work);
        int okm = want == PRIV_J2 ? (m >= 0x100 && m <= 0x10F) : ((m & 0xFF00) == 0x800);
        int cap = want == PRIV_J2 ? PB_J2_N : PB_W_N, ns = want == PRIV_J2 ? PB_J2_SLOTS : PB_W_SLOTS;
        u8 **own = want == PRIV_J2 ? s_pbj_own : s_pbw_own;
        if (!okm) { /* some other model's bones inside the same spawn: the game's pool; keep waiting for ours */ }
        else if (count > cap) { s_priv_want = 0; Log("private bones: model 0x%x needs %d nodes (slot holds %d) - the game's pool is used", m, count, cap); }
        else {
            s_priv_want = 0;
            int pick = -1;
            for (int i = 0; i < ns && pick < 0; i++) if (!own[i]) pick = i;
            for (int i = 0; i < ns && pick < 0; i++) {
                u8 *b = want == PRIV_J2 ? s_pbj[i] : s_pbw[i];
                if (!priv_live(own[i], b)) { Log("private bones: slot %d's old owner %p is gone - slot reused", i, own[i]); pick = i; }
            }
            if (pick >= 0) {
                u8 *b = want == PRIV_J2 ? s_pbj[pick] : s_pbw[pick];
                own[pick] = work; s_priv_hit = 1;
                return priv_build(b, count, parents);
            }
            Log("private bones: no free %s slot - the game's pool is used", want == PRIV_J2 ? "James 2" : "weapon");
        }
    }
    return ((u8 *(__cdecl *)(int, const u8 *))0x0053ABD0)(count, parents);
}
void __cdecl bone_free_c(u8 *chain)
{
    int kind, i = chain ? priv_slot_of(chain, &kind) : -1;
    if (i < 0) { ((void (__cdecl *)(u8 *))0x0053AB40)(chain); return; }    /* the game's own nodes: the game's free */
    u8 *b = kind == PRIV_J2 ? s_pbj[i] : s_pbw[i];
    if (chain != b) Log("private bones: free of %p inside slot %d (not its start) - slot released anyway", chain, i);
    if (kind == PRIV_J2) s_pbj_own[i] = NULL; else s_pbw_own[i] = NULL;    /* never handed to the game's free list */
}
extern void bone_alloc_stub(void);
__asm__(
    ".text\n"
    "_bone_alloc_stub:\n"                /* replaces "call 0x53abd0" at 0x537BA1: (count, parents), work in esi */
    "    pushl %esi\n"
    "    pushl 0xc(%esp)\n"              /* parents */
    "    pushl 0xc(%esp)\n"              /* count */
    "    call _bone_alloc_c\n"
    "    addl $12, %esp\n"
    "    ret\n"
);
static void __cdecl pool_init_hook(void)
{
    ((void (__cdecl *)(void))0x0053AAF0)();
    if (s_xb_used) Log("bone pool: the game re-made its pool (load / new game); the extra blocks are free again");
    s_xb_used = 0;
    /* 0x538050 just wiped every work: nothing holds a private slot any more */
    for (int i = 0; i < PB_J2_SLOTS; i++) s_pbj_own[i] = NULL;
    for (int i = 0; i < PB_W_SLOTS; i++) s_pbw_own[i] = NULL;
    /* P33m: ...and James 2's work and his weapon copies are gone too (a load from the pause menu re-creates James 1,
       possibly in James 2's old work). Forget them now, before anything draws; he rejoins when James 1 can move. */
    if (s_active || s_p2c || s_w2[0] || s_w2[1])
        Log("load / new game: the game wiped every character, James 2 included - he comes back when James 1 can move");
    s_p2c = NULL; s_active = 0; s_w2[0] = s_w2[1] = NULL;
    s_cam2_valid = 0; s_split_now = 0; s_downed = 0;
    s_final_out = 0;                                       /* P33o */
    s_ret_ok = 0;                                          /* P33r */
    if (s_want) s_follow_timer = 60;
}
#define POOL_HEADROOM 24      /* free nodes left for the game's own spawns after James 2 / a weapon copy */
static int pool_prepare(int need, const char *who)
{
    static u8 isfree[IDX_N]; static u8 *order[IDX_N];
    for (int attempt = 0; attempt <= XB_MAX; attempt++) {
        int cnt = POOL_FREE, n = 0;
        if (need <= 0 || cnt > IDX_N) { Log("bone pool: %s needs %d, free %d - not touched", who, need, cnt); return 0; }
        if (cnt - need < POOL_HEADROOM) { if (pool_add_block(who)) continue; if (cnt < need) return -1; }
        memset(isfree, 0, sizeof isfree);
        for (u8 *p = POOL_HEAD; p && n < IDX_N; p = *(u8 **)p) {
            int i = node_idx(p);
            if (i < 0 || isfree[i]) { Log("bone pool: free list looks odd at %p (node %d) - not touched", p, n); return 0; }
            isfree[i] = 1; order[n++] = p;
        }
        /* P28: the counter is "free minus reserved": 0x53AB80 / 0x53ABA0 (called from 0x53A921, 0x53A7FB, 0x538429)
           take nodes off the counter only, for characters placed before their model has loaded (work+0x2B8 == 0).
           So the list can be LONGER than the counter. Only a SHORTER list means something is wrong. */
        if (n < cnt) { Log("bone pool: free list has %d nodes, counter says %d - not touched", n, cnt); return 0; }
        if (n > cnt) Log("bone pool: %d nodes reserved by the game for characters still loading (list %d, counter %d)", n - cnt, n, cnt);
        int ok = 1; for (int k = 1; k < need; k++) if (order[k] != order[0] + k * NODE_SZ) { ok = 0; break; }
        if (ok) { Log("bone pool: %s gets nodes %d..%d (already one block), free %d", who, node_idx(order[0]), node_idx(order[0]) + need - 1, cnt); return 1; }
        int start = -1, run = 0;
        for (int i = 0; i < IDX_N; i++) { if (i == POOL_N) run = 0; if (isfree[i]) { if (++run == need) { start = i - need + 1; break; } } else run = 0; }
        if (start < 0) { if (pool_add_block("no solid block free")) continue; Log("bone pool: no block of %d free nodes for %s (free %d, scattered)", need, who, cnt); return -1; }
        u8 *prev = NULL, *head = NULL;
        for (int k = 0; k < need; k++) { u8 *p = node_at(start + k); if (prev) *(u8 **)prev = p; else head = p; prev = p; }
        for (int k = 0; k < n; k++) { int i = node_idx(order[k]); if (i >= start && i < start + need) continue; *(u8 **)prev = order[k]; prev = order[k]; }
        *(u8 **)prev = NULL;
        POOL_HEAD = head;
        Log("bone pool: %s - moved nodes %d..%d to the front, free %d", who, start, start + need - 1, cnt);
        return 1;
    }
    return -1;
}
static int list_is_block(u8 *head, int need)
{
    int k = 0; u8 *p = head;
    for (; p && k < need; k++, p = *(u8 **)p) if (p != head + k * NODE_SZ) return 0;
    return k == need;
}

/* P27: the game's character spawner 0x48F810 (every script / room spawn goes through it: 88 direct callers incl. the
   0x48F890 wrapper; 0x53A620 is only called from it) loops "evict an enemy and retry" while free bone nodes - need
   <= 0 or all 32 character slots are used (0x48F81C..0x48F852), and never returns if no enemy can be evicted.
   Area scripts run their first step in the event pass 0x48E800, which the main frame calls BEFORE the character
   update (0x4025B9 vs 0x4025C6) where the mod removes James 2 for scripts - so a cutscene's first spawns happen
   while James 2 and his weapon copies still hold 3 slots and ~45 nodes. (The Xbox audit counted >400 nodes in the
   heaviest scenes with James 2 present.) The guard makes room first: extra bone nodes, then James 2's weapon copies,
   then James 2 himself (he rejoins after the scene as usual). The mod's own spawns pass straight through. */
static int bytes_are(u32 addr, const char *hex);
static int patch_jmp_raw(u32 at, const char *expect, int len, void *to)
{
    DWORD old;
    if (!bytes_are(at, expect)) { Log("hook at %08x: bytes differ, not installed", at); return 0; }
    if (!VirtualProtect((void *)at, len, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *(u8 *)at = 0xE9; *(int32_t *)(at + 1) = (int32_t)((u32)to - (at + 5));
    for (int i = 5; i < len; i++) *(u8 *)(at + i) = 0x90;
    VirtualProtect((void *)at, len, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)at, len);
    return 1;
}

extern int __cdecl spawn_tramp(int model, int inst, float *pos, float *rot, int arg);
__asm__(
    ".text\n"
    "_spawn_tramp:\n"
    "    pushl %esi\n"
    "    pushl %edi\n"
    "    movl 0xc(%esp), %edi\n"
    "    pushl $0x48F816\n"
    "    ret\n"
);
static void p2_remove(const char *why);
int __cdecl spawn_guard(int model, int inst, float *pos, float *rot, int arg)
{
    if (!s_own_spawn && s_active && s_p2c) {
        int need = F_BONES_NEEDED(model);
        if (POOL_FREE - need < 8 && pool_add_block("game spawn")) Log("game spawn of model 0x%x: added bone nodes for it", model);
        if (CHARA_CNT >= 30 && (s_w2[0] || s_w2[1])) { w2_drop("game spawn needs a character slot"); }
        if (CHARA_CNT >= 31 || POOL_FREE - need < 1) p2_remove("the game needs his character slot / bones for a scene");
    }
    return spawn_tramp(model, inst, pos, rot, arg);
}
static void coop_spawn(u8 *p1, int quiet)
{
    float pos[4], rot[4];
    int need = F_BONES_NEEDED(MODEL(p1));
    if (s_final_out) {                                          /* P33o */
        if (!s_final_said || !quiet) Log("James 2 stays out: Mary is defeated - the game starts the ending when James 1 HITS her once more (shoot or stomp; examining does nothing)");
        s_final_said = 1;
        if (!quiet) sfx(SND_FAILED);
        return;
    }
    if (CHARA_CNT > 29) {
        Log("JOIN FAILED: no free character slot (characters %d of 32)", CHARA_CNT);
        if (!quiet) sfx(SND_FAILED);
        return;
    }
    int priv = s_priv_ok && s_opt_privbones && need > 0 && need <= PB_J2_N;
    if (!priv) {                                                /* fallback (hooks not installed / switched off): P13-P28 way */
        if (s_xb_used == 0) pool_add_block("James 2 joins");
        if (pool_prepare(need, "James 2") < 0) {
            Log("JOIN FAILED: no room for James 2's bones (free %d, need %d, extra blocks %d)", POOL_FREE, need, s_xb_used);
            if (!quiet) sfx(SND_FAILED);
            return;
        }
    }
    spot_on(p1, pos); memcpy(rot, POSP(p1) + 4, 16);            /* on James 1's spot */
    { int back = s_ret_ok && s_ret_stage == STAGE_IDX && dist2(POSP(p1), s_ret_p1) < 1000.0f * 1000.0f && room_of(p1) == s_ret_room;
      if (back) { memcpy(pos, s_ret_pos, 16); memcpy(rot, s_ret_rot, 16); Log("James 2 comes back where he stood before the scene (James 1 has not moved)"); }
      s_ret_ok = 0; }                                          /* P33r */
    u8 *owner = LIST_OWNER;
    save_state(&s_p1, 1);
    s_priv_hit = 0; s_priv_want = priv ? PRIV_J2 : 0;
    s_own_spawn = 1; F_SPAWN_EV(MODEL(p1), -1, pos, rot, 0); s_own_spawn = 0;
    s_priv_want = 0;
    if (PLAYER == p1) {
        load_state(&s_p1, 1);
        Log("JOIN FAILED: the game refused to spawn model 0x%x", MODEL(p1));
        if (!quiet) sfx(SND_FAILED);
        return;
    }
    s_p2c = PLAYER;                                     /* James's init made the new work "the player" */
    *(float *)(s_p2c + W_HP) = *(float *)(s_p2c + W_HPMAX);
    *(float *)(0x01FB7C80 + BLK_HP) = *(float *)(s_p2c + W_HPMAX);
    save_state(&s_p2, 0);
    /* P33h: never start James 2 inside James 1's grab or death (p1_free already waits for both; this is the backstop) */
    { u8 *b = s_p2.block; float m = *(float *)(b + 0x454);
      if (b[0x497] || b[0x49A] || m != 0.0f || *(u32 *)(b + 0x68) == 0x19 || *(u32 *)(b + 0x64) == 0x19)
          Log("JOIN: James 2's copy of the player block had dying %d, grab phase %d, struggle meter %.2f, state %x/%x - cleared",
              b[0x497], b[0x49A], m, *(u32 *)(b + 0x68), *(u32 *)(b + 0x64));
      b[0x497] = 0; b[0x49A] = 0; *(float *)(b + 0x454) = 0.0f; }
    s_in2 = s_in1;                                      /* James 2's input state starts as a copy (sane prev/timers) */
    *(u32 *)(s_p2c + W_THINK) = (u32)p2_think;
    load_state(&s_p1, 1);
    LIST_OWNER = owner;
    relink_head(p1);
    s_active = 1; s_want = 1; s_spawn_grace = 60; s_flare_j2_ok = 0; s_diag_frames = 181; s_cam2_valid = 0;
    Log("JOINED: James 2 = work %p (model 0x%x inst 0x%x) at %.0f %.0f %.0f, characters %d, bone nodes free %d",
        s_p2c, MODEL(s_p2c), *(u16 *)(s_p2c + W_INST), pos[0], pos[1], pos[2], CHARA_CNT, POOL_FREE);
    Log("  bones: James 2 %p (%s, one block: %s) | James 1 %p (node %d, one block: %s) | need %d",
        *(u8 **)(s_p2c + 0x220), s_priv_hit ? "mod's own slot" : "game's pool", list_is_block(*(u8 **)(s_p2c + 0x220), need) ? "yes" : "NO",
        *(u8 **)(p1 + 0x220), node_idx(*(u8 **)(p1 + 0x220)), list_is_block(*(u8 **)(p1 + 0x220), need) ? "yes" : "NO", need);
    if (!quiet) sfx(SND_SPAWNED);
}
static void p2_remove(const char *why)
{
    if (s_w2[0] || s_w2[1]) { for (int k = 0; k < 2; k++) { if (is_weapon_copy(s_w2[k])) F_REMOVE(s_w2[k], 0, 0); s_w2[k] = NULL; } }
    if (s_active && s_p2c && s_p2c != PLAYER && is_in_list(s_p2c) && *(u32 *)(s_p2c + W_THINK) == (u32)p2_think)
        F_REMOVE(s_p2c, 0, 0);
    if (s_active) Log("James 2 removed: %s", why);
    s_p2c = NULL; s_active = 0; s_cam2_valid = 0;
}
static void bring_p2(u8 *p1, const char *why)
{
    float pos[4]; spot_on(p1, pos);
    F_TELEPORT(s_p2c, pos, YAW(p1));
    s_spawn_grace = 30; s_cam2_valid = 0;            /* P33: his split-screen camera restarts from James 1's */
    Log("James 2 brought to James 1 (%s)", why);
}
static void bring_p1(u8 *p1)
{
    float pos[4]; spot_on(s_p2c, pos);
    F_TELEPORT(p1, pos, YAW(s_p2c));
    memcpy(s_p1_last, pos, 12);
    Log("James 1 brought to James 2");
}
static void room_rule(u8 *p1)
{
    int r1 = room_of(p1), r2 = room_of(s_p2c);
    int c1 = r1 != s_last_r1, c2 = r2 != s_last_r2;
    s_last_r1 = r1; s_last_r2 = r2;
    if (s_room_cool > 0) { s_room_cool--; return; }
    if (!r1 || !r2 || r1 == r2) return;
    /* P33k: James 2 is always the one brought back. Until P33j the "James 2 entered first" case pulled James 1 to
       him (outside James 1-only areas) - a teleport that can carry James 1 past a walk-in story trigger, which only
       ever checks James 1 (see walkin_on_path). The rule never fired in any of the user's logs, so nothing is lost. */
    if (c2 && !c1) { Log("room rule: James 2 entered room %d first - brought back to James 1%s", r2, p2_unsteady(p1) ? " (he was dropping fast)" : ""); bring_p2(p1, "room (James 2 went ahead)"); }
    else if (c1) { Log("room rule: James 1 entered room %d first", r1); bring_p2(p1, "room"); }
    else return;
    sfx(SND_TELEPORT); s_room_cool = 60;
    s_last_r1 = room_of(p1); s_last_r2 = room_of(s_p2c);
}

static void status(const char *tag)
{
    u8 *p1 = PLAYER;
    Log("%s: mode %d/%d ev %d kind %d trig %d area %d stage %08x | J1 %p fl %08x model 0x%x hp %.0f pos %.0f %.0f %.0f room %d | "
        "J2 active %d want %d %p hp %.0f pos %.0f %.0f %.0f | follow %d grace %d chars %d bones %d src %s | J2 flashlight %s",
        tag, MODE_MAIN, MODE_SUB, EV_STATE, EV_KIND, EV_TRIGGER, STAGE_IDX, STAGE_BLOCK, p1, p1 ? FLAGS(p1) : 0, p1 ? MODEL(p1) : 0,
        p1 ? *(float *)(p1 + W_HP) : 0.0f, p1 ? POSP(p1)[0] : 0, p1 ? POSP(p1)[1] : 0, p1 ? POSP(p1)[2] : 0, p1 ? room_of(p1) : -1,
        s_active, s_want, s_p2c, (s_active && s_p2c) ? *(float *)(s_p2c + W_HP) : 0.0f,
        s_p2c ? POSP(s_p2c)[0] : 0, s_p2c ? POSP(s_p2c)[1] : 0, s_p2c ? POSP(s_p2c)[2] : 0,
        s_follow_timer, s_spawn_grace, CHARA_CNT, POOL_FREE, kSrcName[s_asg[1]], s_fl2_why);
}

/* James 2 uses a door / item / save point / message: the game only ever tests "the player" against the trigger
   volumes, so James 2's position and facing are probed with the game's own trigger search (the same probe-only call
   James's input code makes every frame to pick the "search" pose), and on a hit James 1 is pulled to James 2's spot
   and given one Action press; the game then runs the door/pick-up/save on James 1 as usual.  James 2 steps 0.6 m
   back so the two do not share the spot.  Walk-in triggers also need James 2's Action press, so brushing past a
   door never drags James 1 through.  Stages whose triggers are keyed to James 1 alone (lake landing, burning
   stairs, nightmare-hotel finale, the Mary/Maria arenas) are left to James 1. */
static int s_item_probe_ok;
/* P33k: a teleport of James 1 must not carry him past an automatic story trigger.
   The game's walk-in scenes are contact triggers (kind 3/4 in the stage's trigger table, action 3 = run a script, 5 =
   door + script; 51 kind-4 script triggers in the game) that fire only when JAMES 1's probe is inside their box;
   James 2 walks through them unnoticed. Every co-op teleport of James 1 (the pad-1 combo, the pull for a door /
   item / save point James 2 uses) is therefore checked first: the straight line from James 1 to the target is
   sampled every 0.3 m with the game's own trigger search in probe mode (0x48E070(0, 0, 1): contact kinds only, the
   trigger's flag conditions applied, no event bit consumed), and a script trigger found on the way that is not the
   one at the target itself would be skipped - so the teleport is refused. A trigger AT the target is fine: James 1
   lands inside it and it fires for him as it should (that is how James 2 starts a walk-in scene on purpose).
   James 1's position is only borrowed for the probes and put back; nothing else runs in between. */
static int trig_is_script(int idx)
{
    if (idx < 0 || idx >= 0x400 || !STAGE_BLOCK) return 0;
    const u32 *t = (const u32 *)(*(u32 *)STAGE_BLOCK) + idx * 4;
    int act = t[2] >> 28;
    return act == 3 || act == 5;
}
static int walkin_on_path(u8 *p1, const float *to, int dest_idx, int *skipped)
{
    float *pp = POSP(p1); float from[3] = { pp[0], pp[1], pp[2] };
    float dx = to[0] - from[0], dy = to[1] - from[1], dz = to[2] - from[2];
    int n = (int)(sqrtf(dx * dx + dz * dz) / 300.0f) + 1, hit = -1;
    if (n > 80) n = 80;
    pp[0] = to[0]; pp[1] = to[1]; pp[2] = to[2];
    int at_dest = F_TRIG_SEARCH(0, 0, 1);                       /* a scene box James 1 lands IN fires for him: not a skip */
    for (int i = 0; i < n && hit < 0; i++) {
        float t = (float)i / (float)n;
        pp[0] = from[0] + dx * t; pp[1] = from[1] + dy * t; pp[2] = from[2] + dz * t;
        int idx = F_TRIG_SEARCH(0, 0, 1);
        if (idx != dest_idx && idx != at_dest && trig_is_script(idx)) hit = idx;
    }
    pp[0] = from[0]; pp[1] = from[1]; pp[2] = from[2];
    if (skipped) *skipped = hit;
    return hit >= 0;
}
#ifdef SH2COOP_TEST_TOOLS
static int s_test_action, s_test_combo;          /* cloud test build: James 2 "pressed Action" / pad 1 "pressed the combo" */
#endif
static void p2_interact(u8 *p1)
{
    if (!s_opt_interact || !s_p2c || *(float *)(s_p2c + W_HP) <= 0.0f) return;
    int pressed = (s_p2.block[BLK_ACTION] & 1) != 0;
#ifdef SH2COOP_TEST_TOOLS
    if (s_test_action) { pressed = 1; s_test_action = 0; s_p2.block[BLK_ACTION] |= 1; }   /* the probe's own action test reads the byte */
#endif
    if (!pressed) return;                                       /* James 2 did not press Action (from his last think) */
    if (p1_only_area()) return;
    save_state(&s_p1, 0); load_state(&s_p2, 0);                /* PLAYER = James 2's work, his action byte, his state */
    /* P25: in probe mode the search skips the item list (0x48E384: `cmp [probe], 0; jne no_items`), so James 2
       never found health drinks / ammo / keys on the floor. The item branch only reads (item_category_gate 0x48C670,
       ev_item_in_range 0x48B4D0, ch_find 0x537D00, dl_work_in_view 0x5081B0 - whose only writes are its own
       scratch 0xA587F8 / 0xA58A00), so for this one call the jump is turned off. */
    if (s_item_probe_ok) { memcpy((void *)0x0048E388, "\x90\x90\x90\x90\x90\x90", 6); }
    /* P33 split: the item test (dl_work_in_view 0x5081B0) asks "was it on screen" - in split screen that is James 2's
       half, not James 1's */
    int vis_sw = s_split_now && s_vis2_ok; u8 keep_vis[VIS_SZ];
    if (vis_sw) { memcpy(keep_vis, VIS_LIST, VIS_SZ); memcpy(VIS_LIST, s_vis2, VIS_SZ); }
    int idx = F_TRIG_SEARCH(1, 0, 1);
    u32 cc68 = *(u32 *)0x0094CC68, cc6c = *(u32 *)0x0094CC6C, cc70 = *(u32 *)0x0094CC70;   /* P33r: the found trigger's copies */
    if (vis_sw) memcpy(VIS_LIST, keep_vis, VIS_SZ);
    if (s_item_probe_ok) { memcpy((void *)0x0048E388, "\x0f\x85\x8c\x01\x00\x00", 6); }
    load_state(&s_p1, 0);
    if (idx < 0) return;
    if (vis_sw) s_vis_union = 1;          /* James 1 runs it in the next event main: both halves count as on screen */
    int kind = -1, act = -1;
    if (idx < 0x400) { const u32 *t = (const u32 *)(*(u32 *)STAGE_BLOCK) + idx * 4; kind = t[1] >> 28; act = t[2] >> 28; }
    /* P33r (user: "doors that won't open or flag a scene, and first aid / health drinks, won't teleport either
       player"). The event main (0x48E610) maps a found trigger's action to an event state: 1/2 -> 1 (flag),
       3 -> 2 (script), 4 -> 5 (door), 5 -> 3 (door + script), 6 -> 6 (message), 7/8/9 -> 7 (message + sound: locked /
       broken / generic), 10 -> 9 (save point); an item (0x400 + i) -> 8 (pick-up). The flag, message and pick-up
       handlers (0x48BB10, 0x48BB90, 0x48BD10, 0x48BEF0) only lock / animate PLAYER and read the trigger through
       EV_TRIGGER (the copies 0x94CC68/6C/70 are written by the search but read by nobody; set anyway) - none of them
       reads or writes a position. So those are started right here, for James 1
       where he stands, exactly as the event main would start them (same globals), and nobody is moved. Real doors
       (4/5: the search only finds them when their conditions say they open) and save points still bring James 1
       over; scripts (3) - scenes, holes, puzzles, examine scripts - run on James 1 and may move him, so James 2
       cannot start them at all: James 1 walks there himself. */
    { int st = 0;
      if (idx >= 0x400) st = 8;
      else if (act == 1 || act == 2) st = 1;
      else if (act == 6) st = 6;
      else if (act >= 7 && act <= 9) st = 7;
      if (st) {
          if (EV_STATE != 0) return;
          if (*(u32 *)0x01FB7CE4 >= 0x11) return;                 /* the game's own Action gate (0x53AFF0): James 1 mid-attack etc. - James 2 presses again */
          if (idx < 0x400) {                                      /* what the non-probe search does on a hit, kinds 5..10 */
              const u32 *t = (const u32 *)(*(u32 *)STAGE_BLOCK) + idx * 4;
              if (kind >= 5 && kind <= 10 && (t[1] & 8)) F_EVBIT_USE((int)((t[1] >> 4) & 0xFF));
          }
          *(u32 *)0x0094CC68 = cc68; *(u32 *)0x0094CC6C = cc6c; *(u32 *)0x0094CC70 = cc70;
          EV_TRIGGER = idx; EV_STATE = st; EV_KIND = 0; *(int *)0x01F7A948 = 0; *(int *)0x01F7A944 = 0;
          s_room_cool = 60;
          s_map_p2 = 1; s_map_seen = 0; s_map_wait = 0; s_map_armed = 0;   /* the message it shows is James 2's to close */
          s_ui_what = idx >= 0x400 ? "item screen" : "message";
          Log("James 2 used %s %d (kind %d action %d): event %d started where both stand - nobody moved", idx >= 0x400 ? "item" : "trigger",
              idx >= 0x400 ? idx - 0x400 : idx, kind, act, st);
          return;
      }
      if (act != 4 && act != 5 && act != 10) {
          static int last = -2;
          if (idx != last) Log("James 2 at trigger %d (kind %d action %d): a scene / script - only James 1 can start it (nobody moved)", idx, kind, act);
          last = idx; sfx(SND_FAILED); s_room_cool = 30;
          return;
      } }
    float pos[4]; spot_on(s_p2c, pos); float yaw = YAW(s_p2c);
    { int sk;                                                   /* P33k: never carry James 1 past a walk-in scene on the way */
      if (walkin_on_path(p1, pos, idx, &sk)) {
          Log("James 2 used %s %d, but James 1 would be carried past automatic scene trigger %d on the way - not pulled, James 1 has to walk there",
              idx >= 0x400 ? "item" : "trigger", idx >= 0x400 ? idx - 0x400 : idx, sk);
          sfx(SND_FAILED); s_room_cool = 30;
          return;
      } }
    F_TELEPORT(p1, pos, yaw);                                   /* James 1 onto James 2's spot; James 2 stays (no body push between them) */
    memcpy(s_p1_last, pos, 12);
    s_inject_action = 1; s_room_cool = 60;
    s_map_p2 = 1; s_map_seen = 0; s_map_wait = 0; s_map_armed = 0;   /* whatever screen this opens (save menu, message) is James 2's */
    s_ui_what = idx >= 0x400 ? "item screen" : "save / message screen";
    Log("James 2 used %s %d (kind %d action %d): James 1 pulled to %.0f %.0f %.0f", idx >= 0x400 ? "item" : "trigger",
        idx >= 0x400 ? idx - 0x400 : idx, kind, act, pos[0], pos[1], pos[2]);
}

/* enemies pick the nearer James (James 1 or James 2) instead of always James 1; the bosses whose grabs, chases
   and QTEs are keyed to "the player" keep James 1 (Flesh Lips 0x203, Abstract Daddy 0x204, Eddie 0x205, Mary
   0x206, Pyramid Head 0x208/0x209).  Mode 2 = the enemy explicitly wants Maria. */
static u8 *__cdecl select_target_hook(u8 *enemy, int mode)
{
    u8 *t = F_SELECT_TARGET(enemy, mode);
    if (!s_opt_target || !s_active || !s_p2c || mode == 2 || !enemy || !is_in_list(s_p2c)) return t;
    if (*(float *)(s_p2c + W_HP) <= 0.0f || !(FLAGS(s_p2c) & F_DRAWN)) return t;
    int m = MODEL(enemy);
    if (m == 0x203 || m == 0x204 || m == 0x205 || m == 0x206 || m == 0x208 || m == 0x209) return t;
    if (!t) return s_p2c;
    return dist2(POSP(enemy), POSP(s_p2c)) < dist2(POSP(enemy), POSP(t)) ? s_p2c : t;
}
/* friendly fire: the game already rejects a James's gun shots on people (ids 0x1F..0x28 for models <= 0x12E) but lets
   melee through.  This replaces the 8-byte gate at SITE_FF: a James's hit on the other James is dropped, and James 2's
   hits on Maria (0x105/0x106) are dropped, everything else continues exactly as before. */
static int bytes_are(u32 addr, const char *hex);
extern void ff_stub(void);
__asm__(
    ".text\n"
    "_ff_stub:\n"
    "    cmpw $0x12f, 0x10(%ebp)\n"            /* victim model >= 0x12f: enemy/object -> accept as the game does */
    "    jge 1f\n"
    "    movl 0x30(%edi), %edx\n"              /* attacker work */
    "    testl %edx, %edx\n"
    "    jz 2f\n"
    "    movzwl 0x10(%edx), %edx\n"            /* attacker model */
    "    cmpl $0x100, %edx\n"
    "    jb 2f\n"
    "    cmpl $0x103, %edx\n"
    "    ja 2f\n"                              /* attacker is a James */
    "    cmpl $0, _s_active\n"
    "    je 2f\n"
    "    movzwl 0x10(%ebp), %edx\n"            /* victim model */
    "    cmpl $0x100, %edx\n"
    "    jb 3f\n"
    "    cmpl $0x103, %edx\n"
    "    jbe 4f\n"                             /* James on James: drop */
    "3:  cmpl $0x105, %edx\n"
    "    jb 2f\n"
    "    cmpl $0x106, %edx\n"
    "    ja 2f\n"                              /* victim is Maria */
    "    movl 0x30(%edi), %edx\n"
    "    cmpl _s_p2c, %edx\n"
    "    jne 2f\n"                             /* only James 2's hits on her are dropped */
    "4:  movl $0x5450FF, %edx\n"               /* skip this record (next record) */
    "    jmp *%edx\n"
    "1:  movl $0x54514E, %edx\n"               /* accept (damage lookup) */
    "    jmp *%edx\n"
    "2:  movl $0x54513D, %edx\n"               /* continue the game's own id tests */
    "    jmp *%edx\n"
);
static int patch_jmp8(u32 site, const char *expect_hex, void *target)
{
    DWORD old;
    if (!bytes_are(site, expect_hex)) { Log("hook at %08x NOT installed: bytes differ", site); return 0; }
    if (!VirtualProtect((void *)site, 8, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *(u8 *)site = 0xE9; *(int32_t *)(site + 1) = (int32_t)((u32)target - (site + 5));
    *(u8 *)(site + 5) = 0x90; *(u8 *)(site + 6) = 0x90; *(u8 *)(site + 7) = 0x90;
    VirtualProtect((void *)site, 8, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)site, 8);
    return 1;
}

/* ===================== P33d: what the log records for the play-test sections =====================
 * The bosses are the models whose grabs, chases and QTEs are keyed to James 1 (see select_target_hook).  For each one
 * in the character list: "appeared", "hp 0", "gone"; every 5 s, while it is alive and within 30 m of a James, its HP,
 * its target (enemy work +0x138, the field the game's target choice 0x542E80 fills) and its distance to each James;
 * "BOSS TARGETS JAMES 2" at once if one ever picks him; while a boss is in a fight, every HP drop of either James.
 * Plus James 1's dying flag (block +0x497: 2 = dead, the event main 0x48E800 then opens the game-over screen), the
 * boat flag 0x20000 and the area number (stage byte 0x1DBBF80).  Note: the hotel's ordinary Abstract Daddies are the
 * same model as the Labyrinth boss (0x204), so they show up here too. */
static const char *boss_name(int m)
{
    switch (m) {
    case 0x203: return "Flesh Lips";
    case 0x204: return "Abstract Daddy";
    case 0x205: return "Eddie";
    case 0x206: return "Mary";
    case 0x208: case 0x209: return "Pyramid Head";
    }
    return NULL;
}
#define NBOSS 6
static struct { u8 *w; int model, inst, seen, down, shown; float hp; DWORD t2; } s_boss[NBOSS];
static DWORD s_boss_t;
static float s_hp1_last = -1.0f, s_hp2_last = -1.0f;
static const char *who_is(u8 *t, u8 *p1)
{
    if (!t) return "nobody";
    if (t == p1) return "James 1";
    if (s_active && s_p2c && t == s_p2c) return "James 2";
    if (!is_in_list(t)) return "a removed character";
    int m = MODEL(t);
    if (m == 0x105 || m == 0x106) return "Maria";
    return "someone else";
}
static void boss_report(int i, u8 *p1, const char *tag)
{
    u8 *w = s_boss[i].w;
    float d1 = sqrtf(dist2(POSP(w), POSP(p1))) / 1000.0f;
    char j2[96];
    if (s_active && s_p2c) _snprintf(j2, sizeof j2, "%.1f m from James 2 (hp %.0f)", sqrtf(dist2(POSP(w), POSP(s_p2c))) / 1000.0f, *(float *)(s_p2c + W_HP));
    else _snprintf(j2, sizeof j2, "James 2 not in");
    j2[sizeof j2 - 1] = 0;
    Log("%s%s (model 0x%x #%d) hp %.0f | target %s | %.1f m from James 1 (hp %.0f), %s", tag, boss_name(s_boss[i].model),
        s_boss[i].model, s_boss[i].inst, *(float *)(w + W_HP), who_is(*(u8 **)(w + 0x138), p1), d1, *(float *)(p1 + W_HP), j2);
}
/* a boss counts as "in a fight" while it is alive and within 30 m of either James (the Labyrinth's patrolling
   Pyramid Heads far away, and the bodies the game keeps in the list after a fight, are left out of the 5 s lines) */
static int boss_near(int i, u8 *p1)
{
    u8 *w = s_boss[i].w;
    if (*(float *)(w + W_HP) <= 0.0f) return 0;
    if (dist2(POSP(w), POSP(p1)) < 30000.0f * 30000.0f) return 1;
    return s_active && s_p2c && dist2(POSP(w), POSP(s_p2c)) < 30000.0f * 30000.0f;
}
static void boss_watch(u8 *p1)
{
    int n = 0, any = 0;
    DWORD now = GetTickCount();
    for (int i = 0; i < NBOSS; i++) s_boss[i].seen = 0;
    for (u8 *w = LIST_HEAD; w && n < 64; w = *(u8 **)(w + W_NEXT), n++) {
        int m = MODEL(w), k = -1, fr = -1;
        if (!boss_name(m)) continue;
        for (int i = 0; i < NBOSS; i++) { if (s_boss[i].w == w && s_boss[i].model == m) k = i; else if (!s_boss[i].w && fr < 0) fr = i; }
        float hp = *(float *)(w + W_HP);
        if (k < 0) {
            if (fr < 0) continue;
            k = fr; s_boss[k].w = w; s_boss[k].model = m; s_boss[k].inst = *(short *)(w + W_INST); s_boss[k].t2 = 0;
            s_boss[k].down = hp <= 0.0f; s_boss[k].shown = 0; s_boss[k].hp = hp;
        }
        s_boss[k].seen = 1;
        /* "appeared" the first time it is alive: a scene can spawn a boss at 0 HP and give him his HP when the fight
           starts (the stairwell Pyramid Head does); a body left from an earlier fight stays at 0 and gets no lines */
        if (hp > 0.0f && !s_boss[k].shown) { s_boss[k].shown = 1; s_boss[k].down = 0; boss_report(k, p1, "BOSS appeared: "); s_boss_t = now | 1; }
        if (hp <= 0.0f && !s_boss[k].down) { s_boss[k].down = 1; Log("BOSS hp 0: %s (model 0x%x #%d)", boss_name(m), m, s_boss[k].inst);
            if (m == 0x206 && FINAL_AREA() && !s_final_out) { s_final_out = 1; s_final_said = 0; Log("Mary is defeated: James 2 steps out - the ending scenes are James 1's alone (he can join again after a load)"); } }
        if (hp > 0.0f) s_boss[k].down = 0;
        s_boss[k].hp = hp;
        /* the bosses are meant to go for James 1 only: say so at once if one ever picks James 2 (at most every 5 s) */
        if (s_active && s_p2c && *(u8 **)(w + 0x138) == s_p2c && hp > 0.0f && (!s_boss[k].t2 || now - s_boss[k].t2 >= 5000)) {
            s_boss[k].t2 = now | 1;
            boss_report(k, p1, "BOSS TARGETS JAMES 2: ");
        }
    }
    for (int i = 0; i < NBOSS; i++) {
        if (!s_boss[i].w) continue;
        if (!s_boss[i].seen) {
            if (s_boss[i].shown) Log("BOSS gone: %s (model 0x%x #%d), last hp %.0f", boss_name(s_boss[i].model), s_boss[i].model, s_boss[i].inst, s_boss[i].hp);
            s_boss[i].w = NULL; continue;
        }
        if (boss_near(i, p1)) any = 1;
    }
    if (!any) { s_hp1_last = s_hp2_last = -1.0f; return; }
    float hp1 = *(float *)(p1 + W_HP), hp2 = (s_active && s_p2c) ? *(float *)(s_p2c + W_HP) : -1.0f;
    if (s_hp1_last >= 0.0f && hp1 < s_hp1_last - 0.5f) Log("  James 1 hit: hp %.0f -> %.0f", s_hp1_last, hp1);
    if (s_hp2_last >= 0.0f && hp2 >= 0.0f && hp2 < s_hp2_last - 0.5f) Log("  James 2 hit: hp %.0f -> %.0f", s_hp2_last, hp2);
    s_hp1_last = hp1; s_hp2_last = hp2;
    if (s_boss_t && now - s_boss_t < 5000) return;
    s_boss_t = now | 1;
    for (int i = 0; i < NBOSS; i++) if (s_boss[i].w && boss_near(i, p1)) boss_report(i, p1, "boss: ");
}
/* P33g: the game's delayed inventory opening ignores the dying flag (root cause of the user's 12 Oct stairwell lock).
   Event main 0x48E800 opens the inventory two ways when the inventory key is down (0x53AFF0(1): refuses while the
   dying flag 0x1FB8117 or 0x1FB0F4C is set): at once, or - when the sound state 0x9423C0 is 3 - by queueing it
   (0x48EB6F: state = 0, request 0x94CC88 = 1; next frame 1 -> 2; then it opens on the frame the state is 3 again,
   0x48E915..0x48E964). That last step never looks at the dying flag again, so a key press a frame or two before
   James's death opens the inventory anyway. Closing the inventory re-equips the weapon (0x53B540 -> 0x5349C0),
   which clears the dying flag but not the grab (phase byte 0x1FB811A stays 4, the struggle meter 0x1FB80D4 stays
   up): Pyramid Head holds a 0 HP James forever. The gate here replaces the state test 0x476050 at 0x48E91E: when
   the queued opening is due and James is already dying, the request is dropped instead (the same answer the
   immediate path gives). Game code only; nothing is changed for James 2. */
static unsigned long s_inv_gate_drops;
static int __cdecl inv_queue_gate(void)
{
    int busy = ((int (__cdecl *)(void))0x00476050)();
    if (!busy && (*(u8 *)0x01FB8117 || *(u8 *)0x01FB0F4C)) {
        *(int *)0x0094CC88 = 0;
        s_inv_gate_drops++;
        Log("inventory: a queued opening was dropped because James 1 is dying (dying flag %d, grab phase %d, hp %.1f)",
            *(u8 *)0x01FB8117, *(u8 *)0x01FB811A, PLAYER ? *(float *)(PLAYER + W_HP) : -1.0f);
        return 1;
    }
    return busy;
}
/* P33g: safety net for James 1 caught in a grab that can never end (whatever the cause; the inventory gate above
   removes the one seen so far). The grab (James's state 0x19, handler 0x559700): phase byte 0x1FB811A 1 = caught,
   2 = struggling (0x556970 lowers the meter 0x1FB80D4 by one frame time per new button / stick press; meter 0 ->
   phase 3 = free; on any difficulty but Beginner, HP <= 0 -> dying flag 0x1FB8117 = 1), 4 = being killed (only
   entered with the dying flag set; waits for James's animation, then dying = 2 and the event main opens Game Over,
   0x53B0D0 / 0x48EC3D). For Pyramid Head (model 0x208) phase 4 never goes back to 0 by itself.
   Two states the game can't leave, both seen in the user's 12 Oct log (HP 0, dying 0, held for 7 minutes):
     - state 0x19, phase 4, dying flag 0, attacker 0x208: the handler waits for a dying flag that is gone;
     - the dying flag cleared while a grab is still on, then HP at 0.
   After 3 s of play in either, the mod sets the dying flag to 2, the value the game itself gives at the end of
   phase 4 (and the pit death 0x4A6840), so the Game Over screen opens and Continue works as usual. */
static void grab_watch(u8 *p1)
{
    static int last_phase = -1, last_dying = -1, armed; static float stuck_t, still_t, last_hp, last_meter;
    int phase = *(u8 *)0x01FB811A, dying = *(u8 *)0x01FB8117;
    u32 st_up = *(u32 *)0x01FB7CE8, st_lo = *(u32 *)0x01FB7CE4;
    float hp = *(float *)(p1 + W_HP), meter = *(float *)0x01FB80D4, dt = *(float *)0x00932D48;
    u8 *by = *(u8 **)(p1 + 0x138);
    int by_model = by ? (int)(u16)MODEL(by) : 0;
    if (!(dt > 0.0f && dt < 1.0f)) dt = 1.0f / 60.0f;
    if (phase != last_phase) {
        if (last_phase >= 0 || phase) Log("James 1 grab phase %d -> %d (attack 0x%x, by model 0x%x, struggle meter %.2f, hp %.1f, dying %d, state %x/%x)",
            last_phase < 0 ? 0 : last_phase, phase, *(u16 *)(p1 + 0x114), by_model, meter, hp, dying, st_up, st_lo);
        last_phase = phase; still_t = 0;
        if (!phase) armed = 0;
    }
    if (last_dying > 0 && dying == 0 && phase && (st_up == 0x19 || st_lo == 0x19)) {
        armed = 1;
        Log("James 1: dying flag cleared while still held (phase %d, hp %.1f, meter %.2f, state %x/%x) - watching", phase, hp, meter, st_up, st_lo);
    }
    last_dying = dying;
    if (MODE_SUB != 4) return;                                   /* menus pause the game: nothing moves there */
    int grabbed = st_up == 0x19 || st_lo == 0x19;
    int stuck = dying == 0 && phase && grabbed &&
                ((phase == 4 && by_model == 0x208) || (armed && hp <= 0.0f));
    stuck_t = stuck ? stuck_t + dt : 0.0f;
    still_t = (phase && grabbed && hp == last_hp && meter == last_meter) ? still_t + dt : 0.0f;
    last_hp = hp; last_meter = meter;
    if (still_t >= 30.0f) { Log("James 1 grab: nothing has changed for 30 s (phase %d, meter %.2f, hp %.1f, dying %d, state %x/%x)",
        phase, meter, hp, dying, st_up, st_lo); still_t = 0; }
    if (stuck_t >= 3.0f) {
        Log("SAFETY: James 1 is held in a grab the game can't finish (phase %d, by model 0x%x, meter %.2f, hp %.1f, state %x/%x) - "
            "setting the dying flag to 2 so the Game Over screen opens", phase, by_model, meter, hp, st_up, st_lo);
        *(u8 *)0x01FB8117 = 2;
        stuck_t = 0; armed = 0;
    }
}
static void scene_watch(u8 *p1)
{
    static int last_dying = -1, last_boat = -1;
    int dying = *(u8 *)0x01FB8117;
    if (dying != last_dying) {
        if (last_dying >= 0 || dying) Log("James 1 dying flag %d -> %d (hp %.0f)%s", last_dying < 0 ? 0 : last_dying, dying, *(float *)(p1 + W_HP), dying == 2 ? " - dead, game over follows" : "");
        last_dying = dying;
    }
    int boat = (FLAGS(p1) & F_ESCORT) != 0;
    if (boat != last_boat) { if (last_boat >= 0) Log("James 1 boat flag (0x20000) %s", boat ? "ON - in the boat" : "off - out of the boat"); last_boat = boat; }
    boss_watch(p1);
    grab_watch(p1);
}

static void coop_tick(void)
{
    u8 *p1 = PLAYER;
    P2In in; read_p2(&in);
    int join = in.join && !s_prev_join, regroup = in.regroup && !s_prev_regroup, map = in.map && !s_prev_map;
    s_prev_join = in.join; s_prev_regroup = in.regroup; s_prev_map = in.map;
    /* leaving: hold the join key/combo for a second while James 2 is in (or waiting to come back) */
    static int join_hold, join_armed; int leave = 0;
    if (!in.join) { join_hold = 0; join_armed = (s_active || s_want); }      /* must be released after joining before it can mean "leave" */
    else if (join_armed && (s_active || s_want)) { if (++join_hold == 60) leave = 1; }
    int dump = game_focused() && key(VK_F10); if (dump && !s_prev_dump) status("STATUS (F10)"); s_prev_dump = dump;
    { static int prev9; int f9 = game_focused() && key(VK_F9);
      if (f9 && !prev9) { s_fl2_mode = (s_fl2_mode + 1) % 3; sfx(SND_MOVE);
          Log("F9: James 2's flashlight on walls/floors: %s", s_fl2_mode == 0 ? "drawn after James 1's shadows (normal)" : s_fl2_mode == 1 ? "drawn with the map (P29 way, for comparing)" : "off - no James 2 light on walls, enemies or glow (for comparing)"); }
      prev9 = f9; }
#ifdef SH2COOP_TEST_TOOLS
    { /* cloud test build only: F3 lists the stage's triggers (stage+0: 16-byte entries, ends at word1 == word2 == 0;
         stage+4: volume bytes, float x at +0, float z at +6), F4 puts James 1 on the nearest door trigger */
      static int p3, p4; int k3 = game_focused() && key(VK_F3), k4 = game_focused() && key(VK_F4);
      u8 *pl = PLAYER, *st = *(u8 **)0x01F7A950;
      if (((k3 && !p3) || (k4 && !p4)) && pl && st) {
          const u32 *t = *(const u32 **)st; const u8 *vol = *(const u8 **)(st + 4);
          int best = -1; float bd = 1e30f, bx = 0, bz = 0;
          for (int i = 0; i < 400 && (t[i * 4 + 1] || t[i * 4 + 2]); i++) {
              u32 w1 = t[i * 4 + 1], w2 = t[i * 4 + 2]; int kind = w1 >> 28, act = w2 >> 28, shape = (w1 >> 12) & 0xF;
              float x, z; memcpy(&x, vol + ((w1 >> 16) & 0xFFF), 4); memcpy(&z, vol + ((w1 >> 16) & 0xFFF) + 6, 4);
              float dx = x - POSP(pl)[0], dz = z - POSP(pl)[2], d = dx * dx + dz * dz;
              if (k3 && !p3) Log("test: trigger %d kind %d action %d shape %d at %.0f %.0f (%.0f away)%s", i, kind, act, shape, x, z, sqrtf(d), (act == 4 || act == 5) ? " DOOR" : "");
              if ((act == 4 || act == 5) && d < bd) { bd = d; best = i; bx = x; bz = z; }
          }
          if (k4 && !p4 && best >= 0) { float p[4] = { bx, POSP(pl)[1], bz, 1.0f }; F_TELEPORT(pl, p, YAW(pl)); Log("test: James 1 put on door trigger %d at %.0f %.0f", best, bx, bz); }
      }
      p3 = k3; p4 = k4; }
    /* cloud test build only: commands written into s_tcmd by the test harness through the process memory
       1 = James 1 to (v0, v1, v2) facing v3, 2 = James 2 likewise, 3 = status line, 4 = James 1 onto trigger <arg> */
    if (s_tcmd.cmd && !s_tcmd.done) {
        u8 *pl = PLAYER, *st = *(u8 **)0x01F7A950; int c = s_tcmd.cmd;
        float p[4] = { s_tcmd.v[0], s_tcmd.v[1], s_tcmd.v[2], 1.0f };
        if (c == 1 && pl) { F_TELEPORT(pl, p, s_tcmd.v[3]); Log("test: James 1 teleported to %.0f %.0f %.0f", p[0], p[1], p[2]); }
        else if (c == 2 && s_active && s_p2c) { F_TELEPORT(s_p2c, p, s_tcmd.v[3]); s_spawn_grace = 30; s_cam2_valid = 0; Log("test: James 2 teleported to %.0f %.0f %.0f", p[0], p[1], p[2]); }
        else if (c == 3) status("TEST");
        else if (c == 5) s_test_combo = 1;                                  /* pad 1 combo pressed */
        else if (c == 6 && pl) {                                            /* room id at a point */
            float *pp = POSP(pl); float keep[3] = { pp[0], pp[1], pp[2] };
            pp[0] = p[0]; pp[1] = p[1]; pp[2] = p[2]; int r = F_ROOM_ID(); pp[0] = keep[0]; pp[1] = keep[1]; pp[2] = keep[2];
            Log("test: room id at %.0f %.0f %.0f = %d (James 1's room %d)", p[0], p[1], p[2], r, F_ROOM_ID());
        }
        else if (c == 7 && pl) {                                            /* walk-in triggers on the straight line James 1 -> point */
            float *pp = POSP(pl); float from[3] = { pp[0], pp[1], pp[2] };
            float dx = p[0] - from[0], dy = p[1] - from[1], dz = p[2] - from[2];
            int n = (int)(sqrtf(dx * dx + dz * dz) / 300.0f) + 1; if (n > 80) n = 80;
            for (int i = 0; i <= n; i++) {
                float t = (float)i / (float)n;
                pp[0] = from[0] + dx * t; pp[1] = from[1] + dy * t; pp[2] = from[2] + dz * t;
                int idx = F_TRIG_SEARCH(0, 0, 1);
                if (idx >= 0) { const u32 *tt = (const u32 *)(*(u32 *)STAGE_BLOCK) + (idx & 0x3ff) * 4;
                    Log("test: sample %d/%d at %.0f %.0f %.0f: contact trigger %d (kind %d action %d)%s", i, n, pp[0], pp[1], pp[2], idx,
                        idx < 0x400 ? (int)(tt[1] >> 28) : -1, idx < 0x400 ? (int)(tt[2] >> 28) : -1, trig_is_script(idx) ? " SCRIPT" : ""); }
            }
            pp[0] = from[0]; pp[1] = from[1]; pp[2] = from[2];
            int sk; int r = walkin_on_path(pl, p, s_tcmd.arg, &sk);
            Log("test: walkin_on_path to %.0f %.0f %.0f (dest idx %d) = %d (skipped trigger %d)", p[0], p[1], p[2], s_tcmd.arg, r, sk);
        }
        else if (c == 8) s_test_action = 1;                                 /* James 2 pressed Action */
        else if (c == 4 && pl && st) {
            const u32 *t = *(const u32 **)st; const u8 *vol = *(const u8 **)(st + 4); int i = s_tcmd.arg;
            if (i >= 0 && i < 400) { u32 w1 = t[i * 4 + 1]; float x, z; memcpy(&x, vol + ((w1 >> 16) & 0xFFF), 4); memcpy(&z, vol + ((w1 >> 16) & 0xFFF) + 6, 4);
                float q[4] = { x, POSP(pl)[1], z, 1.0f }; F_TELEPORT(pl, q, YAW(pl)); Log("test: James 1 put on trigger %d at %.0f %.0f", i, x, z); }
        }
        s_tcmd.done = 1;
    }
#endif
    { /* P33: F8, or L3+R3 (both sticks pressed) on either controller: next split-screen layout */
      static int prev8, prev_l3r3; int f8 = game_focused() && key(VK_F8), l3r3 = 0;
      if (game_focused()) for (int u = 0; u < 2; u++) { XGP g; if (read_pad(u, &g) && (g.wButtons & 0xC0) == 0xC0) l3r3 = 1; }
      if ((f8 && !prev8) || (l3r3 && !prev_l3r3)) { split_cycle(+1, f8 && !prev8 ? "F8" : "L3+R3"); sfx(SND_MOVE); }
      prev8 = f8; prev_l3r3 = l3r3; }
    { static int prev11, prev12; int f11 = game_focused() && key(VK_F11), f12 = game_focused() && key(VK_F12);
      if (f11 && !prev11) { s_dbg_noshadow = !s_dbg_noshadow; sfx(SND_MOVE); Log("F11: the game's flashlight shadows %s", s_dbg_noshadow ? "OFF (test)" : "on"); }
      if (f12 && !prev12) { s_dbg_noshadow_j2 = !s_dbg_noshadow_j2; sfx(SND_MOVE); Log("F12: James 2 and his weapon %s", s_dbg_noshadow_j2 ? "are NOT shadow casters (P31 default)" : "are shadow casters like James 1 (the game's own two-James behaviour, for comparing)"); }
      prev11 = f11; prev12 = f12; }
    XGP g1; int combo1 = read_pad(0, &g1) && g1.bLT > 64 && g1.bRT > 64 && (g1.wButtons & (XB_LB | XB_RB)) == (XB_LB | XB_RB);
    int combo1_hit = combo1 && !s_prev_combo1; s_prev_combo1 = combo1;
#ifdef SH2COOP_TEST_TOOLS
    if (s_test_combo) { combo1_hit = 1; s_test_combo = 0; }
#endif
    s_frame++;

    /* log the game's own state changes (what started, which mode) - the main debugging aid */
    if (EV_STATE != s_last_ev || EV_KIND != s_last_kind) {
        if (EV_STATE && !s_last_ev) Log("event started: state %d kind %d trigger %d", EV_STATE, EV_KIND, EV_TRIGGER);
        else if (!EV_STATE && s_last_ev) Log("event ended");
        else Log("event state %d kind %d", EV_STATE, EV_KIND);
        s_last_ev = EV_STATE; s_last_kind = EV_KIND;
    }
    if (MODE_MAIN != s_last_mode || MODE_SUB != s_last_sub) {
        Log("game mode %d/%d%s", MODE_MAIN, MODE_SUB, (MODE_MAIN == 13 && MODE_SUB == 13) ? " - GAME OVER screen" : "");
        s_last_mode = MODE_MAIN; s_last_sub = MODE_SUB;
    }
    { static int last_area = -1;                                  /* P33d: which part of the game (stage byte) */
      if (STAGE_IDX != last_area) {
          Log("area %d -> %d%s", last_area, STAGE_IDX, p1_only_area() ? " (a James 1-only area: James 2 can't start anything here and is always brought back to James 1)" : "");
          last_area = STAGE_IDX;
      } }

    /* no live James 1 (title, loading, game over): forget co-op until START is pressed again */
    if (!p1 || !is_in_list(p1) || MODEL(p1) < 0x100 || MODEL(p1) > 0x103) {
        if (s_active || s_want) Log("no James 1 in play: co-op reset");
        p2_remove("no James 1"); s_want = 0; s_follow_timer = 0; s_p1_last_ok = 0; s_downed = 0; s_free_since = 0;
        return;
    }
    scene_watch(p1);
    /* P33o: the ending after Mary is James 1's alone */
    if (s_final_out && !FINAL_AREA()) { s_final_out = 0; Log("left the Mary / Maria arenas: James 2 may join again"); if (s_want) s_follow_timer = 60; }
    if (s_final_out && s_active) { p2_remove("Mary is defeated - the ending scenes are James 1's alone"); s_follow_timer = 0; }
    /* P33o: watch for both Jameses moving as one (user's run after Mary: same position to the unit for 80 s) */
    { static int same, said; static float from[3];
      if (!s_active || !s_p2c || s_p2c == p1) { same = 0; said = 0; }
      else if (dist2(POSP(p1), POSP(s_p2c)) < 1.0f) {
          if (!same++) memcpy(from, POSP(p1), 12);
          if (!said && same >= 120 && dist2(POSP(p1), from) > 1000.0f * 1000.0f) {
              said = 1;
              Log("SAME SPOT: James 1 (work %p think %08x fl %08x) and James 2 (work %p think %08x fl %08x) have moved as one for %d frames | area %d mode %d/%d ev %d kind %d | J1 state %x/%x | p2 block state %x/%x",
                  p1, *(u32 *)(p1 + W_THINK), FLAGS(p1), s_p2c, *(u32 *)(s_p2c + W_THINK), FLAGS(s_p2c), same, STAGE_IDX, MODE_MAIN, MODE_SUB,
                  EV_STATE, EV_KIND, *(u32 *)0x01FB7CE8, *(u32 *)0x01FB7CE4, *(u32 *)(s_p2.block + 0x68), *(u32 *)(s_p2.block + 0x64));
          }
      } else same = 0; }
    if (s_active && !(s_p2c && s_p2c != p1 && is_in_list(s_p2c) && *(u32 *)(s_p2c + W_THINK) == (u32)p2_think && MODEL(s_p2c) == MODEL(p1))) {
        Log("James 2's work was freed or reused by the game"); s_active = 0; s_p2c = NULL;
        if (s_want) s_follow_timer = 15;
    }
    if (s_active && s_downed) { p2_remove("downed (0 HP) - back in 3 seconds"); s_downed = 0; s_follow_timer = 180; sfx(SND_FAILED); }
    if (s_active && CHARA_CNT >= 31) { p2_remove("character slots nearly full"); s_follow_timer = 120; }
    /* short interactions (event states 6/7 message, 8 item pick-up, 9 save point, 10/11 "too dark"/flashlight
       messages) only lock James 1 for a moment: James 2 stands still.  Scripts (kind 2), doors (3/5), the boat and
       anything else that takes James 1's control still make him step out and come back after. */
    { static int hold; int ev = EV_STATE;
      /* P33r: the sub-step (EV_KIND) is 2 while a message is up / an item is being taken, so the old "EV_KIND != 2"
         test let these freeze for one frame only and then took James 2 out and put him back on James 1. Messages,
         pick-ups and the "too dark" / flashlight messages freeze him whatever the sub-step; the save point (9) still
         takes him out (he comes back where he stood). */
      int shortlock = ev == 6 || ev == 7 || ev == 8 || ev == 10 || ev == 11;
      if (shortlock) hold = 10; else if (hold > 0) hold--;                     /* a few frames of slack while the handler unlocks */
      s_freeze = s_active && (shortlock || (hold > 0 && ev == 0 && (FLAGS(p1) & F_LOCKED))); }
    if (s_active && !s_freeze && (EV_KIND == 2 || (FLAGS(p1) & F_ESCORT) ||
                     (!(FLAGS(p1) & F_GRABBED) && (!(FLAGS(p1) & F_CONTROL) || (FLAGS(p1) & F_LOCKED))))) {
        if (s_p2c && *(float *)(s_p2c + W_HP) > 0.0f && !p1_only_area() && !(FLAGS(p1) & F_ESCORT)) {   /* P33r */
            memcpy(s_ret_pos, POSP(s_p2c), 12); s_ret_pos[3] = 1.0f; memcpy(s_ret_rot, POSP(s_p2c) + 4, 16);
            memcpy(s_ret_p1, POSP(p1), 12); s_ret_stage = STAGE_IDX; s_ret_room = room_of(s_p2c); s_ret_ok = 1;
        } else s_ret_ok = 0;
        p2_remove(EV_KIND == 2 ? "scene / script" : (FLAGS(p1) & F_ESCORT) ? "James 1 is in the boat" : "James 1 lost control");
        s_follow_timer = 30;
    }
    if (s_spawn_grace > 0) s_spawn_grace--;
    if (p1_free(p1)) { if (!s_free_since) s_free_since = GetTickCount() | 1; } else s_free_since = 0;
    if (s_active && EV_STATE == 0 && EV_KIND == 0) FLAGS(s_p2c) |= F_DRAWN;

    s_phase = "co-op rules";
    if (map && s_active && p1_free(p1)) s_map_pending = 1;      /* opened after the update (James 1's map byte) */

    if (s_active && p1_free(p1)) {
        room_rule(p1);
        if (regroup) { bring_p2(p1, "regroup key"); sfx(SND_TELEPORT); }
        if (combo1_hit && !p1_only_area()) {                  /* pad 1 LT+RT+LB+RB: James 1 to James 2 */
            int sk;
            if (walkin_on_path(p1, POSP(s_p2c), -1, &sk)) {      /* P33k: not past a walk-in scene - James 2 comes to him instead */
                Log("James 1 to James 2 (combo): automatic scene trigger %d lies on the way - James 2 brought to James 1 instead", sk);
                bring_p2(p1, "combo (a scene trigger was on the way)");
            } else bring_p1(p1);
            sfx(SND_TELEPORT);
        }
        if (s_spawn_grace == 0 && s_follow_timer == 0 && s_room_cool == 0) p2_interact(p1);
    }
    s_phase = "weapon copy";
    w2_sync(p1);
    /* James 1 jumped far in one frame (door, load, event teleport): bring James 2 along shortly after */
    if (s_p1_last_ok && dist2(POSP(p1), s_p1_last) > 400.0f * 400.0f) { s_follow_timer = 15; Log("James 1 moved far (door / teleport)"); }
    memcpy(s_p1_last, POSP(p1), 12); s_p1_last_ok = 1;
    if (s_follow_timer > 0 && --s_follow_timer == 0 && s_want) {
        if (!SETTLED) s_follow_timer = 1;
        else if (s_active) { bring_p2(p1, "follow"); sfx(SND_TELEPORT); }
        else { static int loud; extern int s_join_loud; loud = s_join_loud; s_join_loud = 0;
               coop_spawn(p1, !loud); if (!s_active) s_follow_timer = 120; }
    }
    if (leave) {
        if (s_active) p2_remove("James 2 left the game (join key held)"); else Log("James 2 will not come back (join key held)");
        s_want = 0; s_follow_timer = 0; s_downed = 0; sfx(SND_TELEPORT);
    }
    else if (join && !s_active && s_want) {                     /* pressed again while a join is pending: answer it (P19: it was silent) */
        s_join_loud = 1; if (s_follow_timer <= 0 || s_follow_timer > 1) s_follow_timer = 1;
        Log("join pressed again: retrying now");
    }
    else if (join && !s_active && !s_want) {
        assign_from_join();
        if (!SETTLED) { s_want = 1; if (s_follow_timer <= 0) s_follow_timer = 1; Log("join pressed: waiting until James 1 can move"); }
        else coop_spawn(p1, 0);
    }
}

static void __cdecl frame_hook(void)
{
    static int first = 1;
    if (first) {
        first = 0; Log("first game frame"); if (hook_port0()) Log("input hook in place");
        if (s_split_ok) split_attach(D3D_DEVICE);        /* viewport keeper + depth-bias save from the start */
        /* P33: the Enhanced Edition applies some patches after it loads plugins - make sure the frame hook is still ours */
        if (s_split_ok && !(*(u8 *)SITE_FRAME == 0xE8 && SITE_FRAME + 5 + *(int32_t *)(SITE_FRAME + 1) == (u32)split_frame_hook)) {
            s_split_ok = 0; Log("split screen: the frame call at %08x was changed by another module after start-up - split screen off", SITE_FRAME);
        }
    }
    s_in_hook = 1;
    s_phase = "co-op tick";
    coop_tick();
    s_phase = "game character update (all characters)";
    F_UPDATE_ALL();
    if (s_inject_action) { ACTION_EDGE = 1; s_inject_action = 0; }   /* seen by next frame's event main, before James 1's think recomputes it */
    s_phase = "tether";
    tether_both();
    s_phase = "James 2 checks";
    fall_guard(PLAYER);
    p2_bones_watch();
    s_fl2_j2 = s_active && s_p2c && is_in_list(s_p2c) && (FLAGS(s_p2c) & F_DRAWN) && fl2_dir_ok();
    spawn_diag(0);
    if (s_active && (s_frame % 300) == 150) spawn_diag(1);
    if (s_active && (s_frame % 300) == 0)
        Log("flashlight 2: %s | last 5 s: %lu objects, %lu extra world draws (%lu map re-runs after the shadows), %lu characters (%lu offered James 2's light), %lu glows | J2 light at %.0f %.0f %.0f dir %.2f %.2f %.2f",
            s_fl2_why, (unsigned long)s_fl2_objs, (unsigned long)s_fl2_draws, (unsigned long)s_fl2_replays, (unsigned long)s_fl2_chars, (unsigned long)s_fl2_charsk,
            (unsigned long)s_flare_j2_draws, FL2_POS[0], FL2_POS[1], FL2_POS[2], FL2_DIR[0], FL2_DIR[1], FL2_DIR[2]),
        Log("  shadow casters last 5 s: James 1 & others %lu, James 2 + weapon copies %lu | F11 shadows %s, F12 James 2 shadows %s, F9 mode %d",
            (unsigned long)s_casts_j1, (unsigned long)s_casts_j2, s_dbg_noshadow ? "OFF" : "on", s_dbg_noshadow_j2 ? "OFF" : "on", s_fl2_mode),
        shd_log(),
        s_fl2_objs = s_fl2_draws = s_fl2_chars = s_fl2_charsk = s_fl2_replays = s_flare_j2_draws = s_casts_j1 = s_casts_j2 = 0,
        split_log5s();
    if (s_active && (s_frame % 60) == 0) {
        u8 *p1 = PLAYER;
        { u8 *b1 = *(u8 **)(p1 + 0x9C), *b2 = *(u8 **)(s_p2c + 0x9C);
          Log("bones: J1 root y %.0f | J2 root y %.0f (pos y %.0f)", b1 ? *(float *)(b1 + 8 + 0x34) : 0.0f, b2 ? *(float *)(b2 + 8 + 0x34) : 0.0f, POSP(s_p2c)[1]); }
        Log("tick: J1 fl %08x pos %.0f %.0f %.0f drawn y %.0f | J2 fl %08x state %d pos %.0f %.0f %.0f drawn y %.0f hp %.0f wpn %d | chars %d bones %d",
            FLAGS(p1), POSP(p1)[0], POSP(p1)[1], POSP(p1)[2], *(float *)(p1 + 0x90), FLAGS(s_p2c), *(int *)(s_p2c + W_STATE),
            POSP(s_p2c)[0], POSP(s_p2c)[1], POSP(s_p2c)[2], *(float *)(s_p2c + 0x90), *(float *)(s_p2c + W_HP), s_p2.block[BLK_WEAPON], CHARA_CNT, POOL_FREE);
    }
    s_phase = "game (outside the co-op frame hook)";
    s_in_hook = 0;
    if (s_map_pending) { if (s_active && p1_free(PLAYER)) { MAP_PRESSED = 1; s_map_p2 = 1; s_map_seen = 0; s_map_wait = 0; s_map_armed = 0; s_ui_what = "map"; Log("map opened by James 2"); } s_map_pending = 0; }
}

/* the save writer records every James / enemy work: James 2 hides behind a model id it skips */
static void __cdecl save_stats_hook(void *buf)
{
    short keep = 0;
    if (s_active && s_p2c && is_in_list(s_p2c)) { keep = MODEL(s_p2c); MODEL(s_p2c) = 0x0FFF; }
    F_SAVE_STATS(buf);
    if (keep) MODEL(s_p2c) = keep;
    Log("save written%s", keep ? " (James 2 left out)" : "");
}

/* ===================== crash report ===================== */
static void describe_addr(u32 a, char *out, int n)
{
    HMODULE m = NULL; char name[MAX_PATH] = "?";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m) && m) {
        GetModuleFileNameA(m, name, MAX_PATH);
        char *b = strrchr(name, '\\'); _snprintf(out, n, "%08x (%s+0x%lx)", a, b ? b + 1 : name, (unsigned long)(a - (u32)m));
    } else _snprintf(out, n, "%08x", a);
}
static LONG WINAPI crash_handler(EXCEPTION_POINTERS *e)
{
    static int done;   /* counts reports */
    DWORD code = e->ExceptionRecord->ExceptionCode;
    if (code < 0xC0000000 || done >= 4) return EXCEPTION_CONTINUE_SEARCH;   /* real faults only, first 4 */
    done++;
    CONTEXT *c = e->ContextRecord; char where[160];
    describe_addr((u32)e->ExceptionRecord->ExceptionAddress, where, sizeof where);
    Log("==================== CRASH ====================");
    Log("exception %08lx at %s while: %s%s", code, where, (const char *)s_phase, s_inP2 ? " [inside James 2's turn]" : "");
    if (code == 0xC0000005) Log("  access violation: %s address %08lx",
        e->ExceptionRecord->ExceptionInformation[0] == 0 ? "read" : e->ExceptionRecord->ExceptionInformation[0] == 1 ? "write" : "execute",
        (unsigned long)e->ExceptionRecord->ExceptionInformation[1]);
    Log("  eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx",
        c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
    u32 *sp = (u32 *)c->Esp; int k = 0;
    for (int i = 0; i < 256 && k < 12; i++) {
        u32 v = 0;
        if (IsBadReadPtr(sp + i, 4)) break;
        v = sp[i];
        if ((v >= 0x00401000 && v < 0x00629000) || (v >= 0x10000000 && v < 0x80000000)) {
            char d[160]; describe_addr(v, d, sizeof d);
            if (strstr(d, "+0x") || (v >= 0x00401000 && v < 0x00629000)) { Log("  stack[%d] %s", i, d); k++; }
        }
    }
    u8 *p1 = PLAYER;
    Log("  state: mode %d/%d ev %d kind %d | J1 %p | J2 active %d %p | chars %d bones %d", MODE_MAIN, MODE_SUB, EV_STATE, EV_KIND,
        p1, s_active, s_p2c, CHARA_CNT, POOL_FREE);
    return EXCEPTION_CONTINUE_SEARCH;
}

/* ===================== install ===================== */
static int bytes_are(u32 addr, const char *hex)
{
    const u8 *p = (const u8 *)addr;
    for (int i = 0; hex[i * 2]; i++) { unsigned v; sscanf(hex + i * 2, "%2x", &v); if (p[i] != (u8)v) return 0; }
    return 1;
}
static int patch_call(u32 site, u32 expect, void *target)
{
    DWORD old;
    if (*(u8 *)site != 0xE8 || site + 5 + *(int32_t *)(site + 1) != expect) {
        const u8 *p = (const u8 *)site;
        Log("hook at %08x NOT installed: found %02x %02x %02x %02x %02x (expected a call to %08x; another mod may have patched it)", site, p[0], p[1], p[2], p[3], p[4], expect);
        return 0;
    }
    if (!VirtualProtect((void *)site, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *(int32_t *)(site + 1) = (int32_t)((u32)target - (site + 5));
    VirtualProtect((void *)site, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)site, 5);
    return 1;
}
/* the Enhanced Edition replaces the same call (its d-pad / controller tweaks), so chain to whatever is there */
static int hook_port0(void)
{
    u32 site = SITE_PORT0;
    if (*(u8 *)site != 0xE8) { Log("input hook: no call at %08x", site); return 0; }
    u32 cur = site + 5 + *(int32_t *)(site + 1);
    if (cur == (u32)port0_conv) return 1;
    s_conv_orig = (ConvFn)cur;
    Log("input hook: chaining to %08x%s", cur, cur == 0x00458060 ? " (the game's own)" : " (another mod's hook, probably the Enhanced Edition)");
    return patch_call(site, cur, (void *)port0_conv);
}
/* P33d: the previous session's log is kept as "<name> - YYYY-MM-DD HH-MM-SS.log" (its last write time), so a new
   start of the game no longer wipes it.  Returns 1 and the kept file's name when it was renamed. */
/* P33l: is this a clean Enhanced Edition install?  Lists other .asi plugins and DLLs the Enhanced Edition does not
   ship (the game's own and the EE's: d3d8, dinput8 = Xidi, SDL3, SDL.XidiPlugin.32, Xidi.32, binkw32, msvcp70,
   msvcr70).  Only logged - the mod runs either way - but it is the first thing to look at in a bug report. */
static void log_other_code(void)
{
    static const char *known[] = { "d3d8.dll", "dinput8.dll", "SDL3.dll", "SDL.XidiPlugin.32.dll", "Xidi.32.dll", "binkw32.dll", "msvcp70.dll", "msvcr70.dll" };
    static const char *where[] = { "", "scripts\\", "plugins\\" };
    static const char *pats[]  = { "*.asi", "*.dll" };
    char list[900] = "", pat[MAX_PATH + 32]; int n = 0;
    for (int w = 0; w < 3; w++)
        for (int k = 0; k < 2; k++) {
            WIN32_FIND_DATAA fd; HANDLE h;
            _snprintf(pat, sizeof pat, "%s%s%s", s_dir, where[w], pats[k]); pat[sizeof pat - 1] = 0;
            h = FindFirstFileA(pat, &fd);
            if (h == INVALID_HANDLE_VALUE) continue;
            do {
                int skip = 0;
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                if (!_strnicmp(fd.cFileName, "sh2coop", 7)) continue;                       /* this mod (and its test builds) */
                if (k == 1 && w == 0) for (int i = 0; i < 8; i++) if (!_stricmp(fd.cFileName, known[i])) skip = 1;
                if (skip) continue;
                if (strlen(list) + strlen(where[w]) + strlen(fd.cFileName) + 3 < sizeof list) { if (n) strcat(list, ", "); strcat(list, where[w]); strcat(list, fd.cFileName); }
                n++;
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    if (n) Log("OTHER CODE in the game folder (%d): %s - NOT a supported setup: the co-op is made for a clean Enhanced Edition install and may not work correctly with other plugins or wrapper DLLs. Move them out before reporting a problem.", n, list);
    else Log("game folder: no other plugins or wrapper DLLs - a clean Enhanced Edition install");
}
static int keep_old_log(const char *path, char *kept, int n)
{
    WIN32_FILE_ATTRIBUTE_DATA a; FILETIME lt; SYSTEMTIME t;
    kept[0] = 0;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &a)) return 0;              /* no old log */
    if (a.nFileSizeHigh == 0 && a.nFileSizeLow == 0) return 0;                          /* empty: nothing to keep */
    if (!FileTimeToLocalFileTime(&a.ftLastWriteTime, &lt) || !FileTimeToSystemTime(&lt, &t)) return 0;
    char stem[MAX_PATH]; lstrcpynA(stem, path, sizeof stem);
    char *dot = strrchr(stem, '.'); if (dot) *dot = 0;
    char dst[MAX_PATH + 64];
    for (int i = 1; i <= 20; i++) {
        if (i == 1) _snprintf(dst, sizeof dst, "%s - %04d-%02d-%02d %02d-%02d-%02d.log", stem, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
        else _snprintf(dst, sizeof dst, "%s - %04d-%02d-%02d %02d-%02d-%02d (%d).log", stem, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, i);
        dst[sizeof dst - 1] = 0;
        if (MoveFileA(path, dst)) {
            const char *b = strrchr(dst, '\\'); lstrcpynA(kept, b ? b + 1 : dst, n);
            return 1;
        }
        DWORD e = GetLastError();
        if (e != ERROR_ALREADY_EXISTS && e != ERROR_FILE_EXISTS) return 0;              /* in use / read-only: overwrite as before */
    }
    return 0;
}
static void install(HMODULE self)
{
    char path[MAX_PATH]; GetModuleFileNameA(self, path, MAX_PATH);
    strcpy(s_dir, path); { char *sl = strrchr(s_dir, '\\'); if (sl) sl[1] = 0; else s_dir[0] = 0; }
    chr_set_dir(s_dir);
    char *dot = strrchr(path, '.'); if (dot) strcpy(dot, ".log");
    char kept[MAX_PATH + 64]; int kept_ok = keep_old_log(path, kept, sizeof kept);
    s_log = fopen(path, "w");
    Log("sh2coop PC build P33v loaded (P33u + a James playing as another character is plain James in cutscenes)");
    if (kept_ok) Log("the previous log was kept as \"%s\"", kept);
    log_other_code();
    AddVectoredExceptionHandler(1, crash_handler);
    const char *xi[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    for (int i = 0; i < 3 && !pXInputGetState; i++) {
        HMODULE h = LoadLibraryA(xi[i]);
        if (h) { pXInputGetState = (XInputGetStateFn)GetProcAddress(h, "XInputGetState"); if (pXInputGetState) Log("using %s for controllers", xi[i]); }
    }
    if (!pXInputGetState) Log("no XInput DLL found: James 2 can only use the keyboard");
    int ok = bytes_are(0x00538C40, "a18469fb0185c07424") && bytes_are(0x00536380, "83ec205657")
          && bytes_are(0x0048F810, "56578b7c240c57e8") && bytes_are(0x0053ADC0, "d905f080fb01")
          && bytes_are(0x00446950, "e8db1f0100a0742c9300") && bytes_are(0x005396C0, "8b4424048b4c2408");
    if (!ok) { Log("this sh2pc.exe is not the Enhanced Edition v1.0 exe this mod was made for: doing nothing"); return; }
    int n = 0;
    for (int i = 0; i < 2; i++) n += patch_call(kUpdateSites[i], 0x00538C40, (void *)frame_hook);
    n += patch_call(SITE_SAVE_STATS, 0x0053ADC0, (void *)save_stats_hook);
    n += hook_port0();
    n += patch_call(SITE_INPUT_FRAME, 0x00458930, (void *)input_frame_hook);
    n += patch_call(SITE_TARGET_A, 0x00542E80, (void *)select_target_hook);
    n += patch_call(SITE_TARGET_B, 0x00542E80, (void *)select_target_hook);
    n += patch_jmp8(SITE_FF, "66817d102f017d11", (void *)ff_stub);
    n += patch_call(SITE_POST_ALL, 0x00538B40, (void *)post_update_all_hook);
    n += patch_call(SITE_ATK, 0x00543310, (void *)attack_volumes_hook);
    n += patch_call(SITE_DRAW_WORK, 0x0050B3D0, (void *)draw_work_hook);
    if (patch_call(0x0050BA1F, 0x00509E50, (void *)chr_skin_hook)) n++;   /* CS: weapons into the character's hand */
    if (patch_call(0x005386DF, 0x00536D70, (void *)chr_torch_hook)) n++;  /* CS: flashlight light + glow onto her chest */
    { int a = patch_call(0x0054B3AB, 0x00516E90, (void *)step_sfx_a), b = a ? patch_call(0x0054B355, 0x00516E90, (void *)step_sfx_b) : 0;
      if (a && !b) patch_call(0x0054B3AB, (u32)step_sfx_a, (void *)0x00516E90);
      if (a && b) n++; }                                                  /* CS: Maria's heel steps for Maria / Angela */
    n += hook_tex_lookup();
    n += patch_jmp_raw(0x0048F810, "56578b7c240c", 6, (void *)spawn_guard);
    if (patch_call(0x0048E91E, 0x00476050, (void *)inv_queue_gate)) n++;
    else Log("inventory gate NOT installed: the queued inventory opening can still happen while James 1 is dying");
    if (patch_call(0x0052826E, 0x00527F10, (void *)grab_sweep_stub)) n++;
    else Log("grab pass-through NOT installed: James 2 can be caught by grabs (P33i lets him go)");
    if (bytes_are(0x0052839B, "e8c0b0ffff") && patch_call(0x0052839B, 0x00523460, (void *)hold_rec_stub)) n++;
    else Log("hold pass-through NOT installed: a boss's hold can still hit James 2");
    { DWORD o; if (bytes_are(0x0048E384, "396c24500f858c010000") && VirtualProtect((void *)0x0048E388, 6, PAGE_EXECUTE_READWRITE, &o)) { s_item_probe_ok = 1; n++; }
      else Log("James 2 item pick-up: search code differs, not installed"); }
    n += patch_call(SITE_BODY_PAIR, 0x00529C50, (void *)body_pair_stub);
    n += patch_call(0x0040834C, 0x0053AAF0, (void *)pool_init_hook);
    n += patch_call(0x0053A785, 0x0053AAF0, (void *)pool_init_hook);
    /* P29: private bone slots need BOTH the alloc and the free site, or none (a private chain handed to the game's
       free list would be used twice) */
    if (bytes_are(0x00537B9D, "83c40450e82a300000") && bytes_are(0x00538439, "51e801270000")) {
        int a = patch_call(0x00537BA1, 0x0053ABD0, (void *)bone_alloc_stub);
        int f = a ? patch_call(0x0053843A, 0x0053AB40, (void *)bone_free_c) : 0;
        if (a && !f) { patch_call(0x00537BA1, (u32)bone_alloc_stub, (void *)0x0053ABD0); a = 0; }
        s_priv_ok = a && f; n += 2 * s_priv_ok;
    }
    if (!s_priv_ok) Log("private bone slots NOT installed (code differs): James 2 uses the game's pool with the P28 checks");
    /* P29: James 2's flashlight - all three world sites or none; the character part needs the world part */
    if (bytes_are(0x005B28B0, "81eca000000053558bac24ac000000") && bytes_are(0x005B4C58, "51ff921c010000") && bytes_are(0x005B062F, "50ff9218010000")) {
#ifdef SH2COOP_TEST_TOOLS
        patch_call6(0x00501F21, "ff911c010000", (void *)chr_dip_cap);
#endif
        int a = patch_call6(0x005B4C59, "ff921c010000", (void *)dip_hook);
        int b = patch_call6(0x005B0630, "ff9218010000", (void *)dp_hook);
        int c = (a && b) ? patch_jmp_raw(0x005B28B0, "81eca0000000", 6, (void *)upl_hook_c) : 0;
        s_fl2_ok = a && b && c; n += a + b + c;
        if (s_fl2_ok && patch_call(0x0050DCED, 0x0050D1C0, (void *)sel_hook)) { s_fl2_chars_ok = 1; n++; }
        if (s_fl2_ok) {   /* P30: the map's James 2 light after James 1's shadows - both shadow-pass calls or neither */
            int sa = patch_call(0x004766C5, 0x005A81A0, (void *)shadow_pass_hook);
            int sb = sa ? patch_call(0x004766DA, 0x005A81A0, (void *)shadow_pass_hook) : 0;
            if (sa && !sb) { patch_call(0x004766C5, (u32)shadow_pass_hook, (void *)0x005A81A0); sa = 0; }
            s_fl2_defer = sa && sb; n += 2 * s_fl2_defer;
            if (patch_call(0x00476752, 0x00511F60, (void *)flare_draw_hook)) { s_flare_hook_ok = 1; n++; }
            { int ca = patch_call(0x0050BAD0, 0x00509C20, (void *)shadow_cast_hook);
              int cb = ca ? patch_call(0x0050BBD6, 0x00509C20, (void *)shadow_cast_hook) : 0;
              if (ca && !cb) patch_call(0x0050BAD0, (u32)shadow_cast_hook, (void *)0x00509C20);
              n += (ca && cb) ? 2 : 0; }
        }
    }
    /* P33: split screen - the frame's call site, the per-call effects inside 0x476680, James 2's camera guard, the
       flash decay of the character pass */
    {
        int f = patch_call(SITE_FRAME, 0x00476B40, (void *)split_frame_hook);
        int e = patch_call(SITE_ETIMERS, 0x00519C50, (void *)enemy_timers_hook);
        int fl = patch_call(SITE_FLASHDEC, 0x0050A2A0, (void *)flash_decay_hook);
        int fx = 0;
        for (int i = 0; i < 2; i++)
            fx += patch_call(kMapFxSites[i], 0x004C7650, (void *)mapfx_hook) + patch_call(kTaskFxSites[i], 0x004EF9F0, (void *)taskfx_hook)
                + patch_call(kNoiseSites[i], 0x00478210, (void *)noisetex_hook);
        s_split_ok = f && e && fl && fx == 6;              /* each hook alone behaves exactly like the game outside split frames */
        n += f + e + fl + fx;
        Log("split screen: %s", s_split_ok ? "hooks installed" : "NOT installed (code differs) - one shared screen as before");
    }
    Log("James 2's flashlight: world %s (%s), enemies/NPCs %s, glow %s", s_fl2_ok ? "installed" : "NOT installed (code differs)",
        s_fl2_defer ? "drawn after James 1's shadows" : "drawn with the map", s_fl2_chars_ok ? "installed" : "NOT installed", s_flare_hook_ok ? "installed" : "NOT installed");
    /* options menu rows */
    DWORD old;
    s_rows_ok = bytes_are(0x0045FAC6, "83c305") && bytes_are(0x0045FCD3, "66b80500") && bytes_are(0x0045FCD9, "663d0500")
             && bytes_are(0x0045FCE4, "66b80400") && bytes_are(0x0045FCEA, "663d0400");
    int menu_ok = s_rows_ok && bytes_are(0x004613A0, "83ec08535556") && bytes_are(0x0045FA90, "51a000169400");
    if (menu_ok && VirtualProtect((void *)0x0045FA00, 0x400, PAGE_EXECUTE_READWRITE, &old)) {
        n += patch_call(SITE_OPT_DRAW, 0x004613A0, (void *)opt_draw_hook);
        n += patch_call(SITE_OPT_HANDLER, 0x00462E50, (void *)opt_handler_hook);
        n += patch_call(SITE_CURSOR_A, 0x0045FA90, (void *)cursor_hook);
        n += patch_call(SITE_CURSOR_B, 0x0045FA90, (void *)cursor_hook);
    } else { s_rows_ok = 0; Log("options menu: code mismatch, not installed"); }
    Log("%d of 47 hooks installed", n);
    load_ini(self);
    title_install(Log, s_opt_title);
    Log("James 2 uses the game's own control layout on his assigned device. G or BACK+START on any pad = join (auto: that device becomes James 2's), Z or BACK alone on his pad = regroup, his Map button = map, F6 = swap devices, Pad 1 LT+RT+LB+RB = James 1 to James 2, F9 = James 2's flashlight mode (after shadows / with the map / off), F10 = status line, F11 / F12 = shadow tests (all flashlight shadows off / James 2 as a shadow caster).");
}
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r)
{
    (void)r;
    if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(h); install(h); }
    return TRUE;
}
