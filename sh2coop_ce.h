/* sh2coop_ce.h - character editor (experimental build): colours per James, the title-screen preview, ini storage */
#ifndef SH2COOP_CE_H
#define SH2COOP_CE_H
#include <windows.h>
#include <d3d8.h>
#include <stdint.h>
typedef uint8_t ce_u8; typedef uint32_t ce_u32;

enum { CE_JACKET, CE_SHIRT, CE_JEANS, CE_BOOTS, CE_HAIR, CE_PARTS };
extern const char *const ce_part_name[CE_PARTS];
typedef struct { const char *name; float m[3]; float b[3]; } CeColor;   /* out = L * m + b per channel (L = texel luminance) */
extern const CeColor ce_palette[];
extern const int ce_palette_n;

extern int ce_color[2][CE_PARTS];      /* palette index per James (0 = James 1, 1 = James 2) and part; 0 = original */
extern int ce_gen[2];                  /* bumped when a James's colours change: texture copies are remade by whoever notices */

/* which part a texel of James's texture `id` belongs to (-1 = none: skin, badges, eyes...).  Rules shared by the
   in-game recolour and the preview. */
int ce_part_of(ce_u32 id, float fx, float fy, int r, int g, int b);
void ce_apply(int ci, ce_u8 *r, ce_u8 *g, ce_u8 *b);                /* recolour one texel with palette entry ci */
int ce_any(int who);                                                /* 1 if that James has any non-original part */
void ce_cycle(int who, int part, int dir);                          /* menu: next / previous colour, saves the ini */
void ce_load(const char *inipath, int james2_red_jacket);           /* [look] section; defaults = the old fixed look */
void ce_save(void);

/* preview: James's own model file drawn with the game's Direct3D device inside a rectangle of the menu */
int  ce_preview_load(const char *gamedir);                          /* 1 = ready (loads data\chr\jms\hhh_jms.mdl once) */
void ce_preview_draw(IDirect3DDevice8 *dev, int who, int px, int py, int pw, int ph, float yaw);
void ce_preview_release(void);                                      /* device lost / reset: drop the textures */
const char *ce_preview_status(void);

/* provided by sh2coop.c: recolour one DXT5 level for James `who` (uses the same rules as the in-game copies) */
int look_recolor_dxt5(int who, ce_u32 id, int w, int h, const ce_u8 *src, ce_u8 *dst, int dpitch);
void ce_log(const char *fmt, ...);
#endif
