/* sh2coop_chr.h - play as Maria, Eddie or Angela (experimental build): the mod draws the character in James's place */
#ifndef SH2COOP_CHR_H
#define SH2COOP_CHR_H
#include <windows.h>
#include <d3d8.h>
enum { CHR_JAMES, CHR_MARIA, CHR_EDDIE, CHR_ANGELA, CHR_COUNT };
const char *chr_name(int k);
void chr_set_dir(const char *dir);
int  chr_ready(int k);
/* draw character k in place of the James work (his live bones, his hand / flashlight parts); 1 = drawn (skip James) */
int  chr_draw_world(IDirect3DDevice8 *dev, int k, const unsigned char *work, const unsigned char *weapon, int who);
/* a weapon held by a James who is drawn as character k: move the weapon's bone matrices from James's hand into the
   character's hand (call before the weapon is drawn; chr_weapon_end puts James's values back right after) */
typedef struct { unsigned char *node[4]; float m[4][16]; int n; } ChrWeaponSave;
int  chr_weapon_begin(int k, const unsigned char *james, unsigned char *weapon, ChrWeaponSave *sv);
void chr_weapon_end(ChrWeaponSave *sv);
/* how far the flashlight (light and lens glow) moves from James's pocket to the character's (world units) */
int  chr_torch_delta(int k, const unsigned char *james, const unsigned char *weapon, float *d);
void chr_draw_preview(IDirect3DDevice8 *dev, int k, int who, int px, int py, int pw, int ph, float yaw);
/* Maria's colours per James (palette indices of ce_palette; 0 = original), saved in sh2coop.ini [look] */
#define CHR_MPARTS 4
extern int chr_mcolor[2][CHR_MPARTS];
extern const char *const chr_mpart_name[CHR_MPARTS];
void chr_mcolor_load(const char *ini);
void chr_mcolor_cycle(int who, int part, int dir);
#endif
