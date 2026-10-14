/*
 * sh2coop_title.c  --  P33e/P33f: the co-op version's title screen.
 *
 *   1. "CO-OP" under "ENHANCED EDITION", in the same hanging-letter style: C from ENHANCED's C, O from EDITION's O,
 *      P from ENHANCED's D (top and bottom of the bowl stacked, its stem below), the dash from EDITION's T cross-bar.
 *      The letters are built at run time from the Enhanced Edition's own title atlas (sh2e\pic\etc\start01e.tex)
 *      into an empty corner of that atlas, and drawn as two extra sprites right after the logo, through the game's
 *      own sprite path (0x495C20's steps) with the logo's own animated colour - so they pulse with the logo.
 *      Like the logo, they are also baked into the background as a black silhouette with the logo's glow.
 *   2. The background (sh2e\pic\etc\start00.tex, 2732x2048 DXT3) recoloured from blue to a faded kelly green:
 *      the two colour end-points of every 4x4 block go through a luminance ramp.
 *   3. The title's lettering dark red: the logo, ENHANCED EDITION, CO-OP (logo colour, title colour table 0x88BEC8
 *      entry 0) and the copyright line (background).  P33f: the menu words (the buttons: LOAD ... EXIT and the New
 *      Game screen's) keep the game's own colours - white when selected, grey otherwise, NEW GAME's yellow - as the
 *      player asked; P33e had made them dark red too.
 *
 * Code path (traced on the PC exe + EE, title module 0x495000-0x499000):
 *   0x496F1B.. title init: file start00 -> buffer [0x94D608], start01 (EE: start01e) -> [0x94D60C] (async loads)
 *   0x496950 draws the background: 0x49F9C0(sprite 0x94D580, [0x94D608], 0, 0) at 0x496969 binds the texture -
 *            the D3D texture is made from the buffer the first time (0x457670), so the buffer is patched just
 *            before that call; then the logo: 0x4960F0 animates entry 0's colour (table 0x88BEC8 normal <-> alt),
 *            0x495C20(entry 0) at 0x496A4A (x on the stack, y in ebx, entry in di).
 *   0x495C20 draws atlas entry di: 0x49F9C0(0x94D5C4, [0x94D60C], 0, 0) at 0x495C3E, colour, 0x49EF20 alpha,
 *            coordinates (12.4) and UVs (512-space x16), 0x49EF70 queues a copy of the sprite.
 *   0x4961A0 copies each menu word's normal / selected colour from 0x88BEC8 to the sprite table every frame;
 *            word 3 (NEW GAME) is drawn at 1.5x red/green of its colour when 0x1DBBF85 is set.
 * Nothing of the Enhanced Edition's or Konami's art is in this file: only positions, sizes and colours.
 */
#include <windows.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef void (*TitleLogFn)(const char *fmt, ...);
static TitleLogFn s_tlog;
#define TLOG(...) do { if (s_tlog) s_tlog(__VA_ARGS__); } while (0)

/* ---------------- the Enhanced Edition's title files (checked before anything is changed) ---------------- */
#define TEX_MAGIC     0x19990901u
#define AT_W 2048
#define AT_H 2048
#define AT_DATA 80                    /* start01e.tex: 80-byte header, then 2048x2048 BGRA, top row first */
#define BG_W 2732
#define BG_H 2048
#define BG_BW (BG_W / 4)              /* 683 blocks per row */
#define BG_BH (BG_H / 4)              /* 512 block rows */
#define BG_DATA 96                    /* start00.tex: 96-byte header, then DXT3 blocks, 16 zero bytes after them */

/* ---------------- CO-OP: glyph sources and layout, in atlas texels (the logo sprite's own texture space) ------- */
#define GY0 224                       /* glyph rows 224..355: strand from 224, letter body ~272..354 */
#define GH 132
#define R_(ay) ((ay) - GY0)
#define CO_OFF 112                    /* CO-OP's rows = ENHANCED EDITION's + 112 */
#define CX0 736                       /* canvas: atlas x 736..1323, rows 216..475 */
#define CX1 1324
#define CY0 216
#define CY1 476
#define CW (CX1 - CX0)
#define CH (CY1 - CY0)
static u8 s_canvas[CH][CW];           /* CO-OP alpha in atlas space */
/* two pieces of the line are copied into the atlas' empty corner (x >= 1536, y >= 768: no sprite uses it) */
typedef struct { int ax0, ax1, ay0, ay1, dx, dy; } Piece;
static const Piece kPieces[2] = {
    { 740,  968, 220, 472, 1540,  772 },          /* "CO"  */
    { 968, 1320, 220, 472, 1540, 1032 },          /* "-OP" */
};
#define MARK_X 2047                   /* marker texel (alpha stays 0): RGB 43 4F 4F = this atlas carries CO-OP */
#define MARK_Y 2047

/* ---------------- background: where the logo is baked into it (measured: corr 0.91 at full resolution) -------- */
#define BK_OX 334.0f                  /* texture x = atlas x + 334 (texels counted from the first block at file offset 96) */
#define BK_OY 566.0f                  /* texture y = 566 + 1.4 * atlas y */
#define BK_SY 1.4f
/* the baked letters' glow, added to the texture around them (measured around ENHANCED EDITION, dark side):
   distance from the letter in atlas texels -> added brightness */
static const float kGlowD[] = { 0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 22, 28, 35, 42, 55, 70 };
static const float kGlowE[] = { 55, 70, 78, 80, 73, 66, 58, 47, 39, 33, 28, 23, 18, 12.5f, 9.3f, 7, 1.6f, 0 };
static const float kSil[3] = { 9, 10, 10 };       /* the baked letters' colour */
/* faded kelly green: luminance -> colour */
static const float kStopL[4] = { 0.0f, 0.08f, 0.45f, 1.0f };
static const float kStopC[4][3] = { { 0, 0, 0 }, { 10, 19, 9 }, { 86, 122, 74 }, { 225, 236, 220 } };
/* copyright line -> dark red (texture rows 1948..2003, x 760..1979) */
#define CR_BX0 190
#define CR_BX1 495
#define CR_BY0 487
#define CR_BY1 501
static const float kCopyRed[3] = { 112, 14, 14 };

/* ---------------- the title's colours (0x88BEC8: per entry normal RGBA, selected/alt RGBA, speed, phase) -------- */
#define T_COLTAB 0x0088BEC8
#define T_SPRTAB 0x0088BC40
static const int kLogoPeak[4] = { 64, 8, 8, 128 }, kLogoLow[4] = { 10, 1, 1, 8 };

static int s_enabled, s_atlas_bad, s_bg_bad, s_coop_ok, s_bg_patches, s_atlas_patches;
static u32 s_bg_fp_orig, s_bg_fp_done;

/* ============================== CO-OP line (atlas) ============================== */
static inline u8 at_a(const u8 *atl, int x, int y) { return atl[AT_DATA + ((size_t)y * AT_W + x) * 4 + 3]; }
static int atlas_ok(const u8 *b)
{
    return b && *(const u32 *)b == TEX_MAGIC && *(const u32 *)(b + 16) == 0x6a && *(const u32 *)(b + 20) == 0x08000800;
}
static int has_mark(const u8 *atl)
{
    const u8 *p = atl + AT_DATA + ((size_t)MARK_Y * AT_W + MARK_X) * 4;
    return p[0] == 0x43 && p[1] == 0x4F && p[2] == 0x4F && p[3] == 0;
}
/* python's round() (half to even), so the strand positions are the mock-up's */
static int round_half_even(double v)
{
    double f = floor(v), d = v - f;
    if (d > 0.5) return (int)f + 1;
    if (d < 0.5) return (int)f;
    return ((long long)f % 2 == 0) ? (int)f : (int)f + 1;
}
static int strand_center(u8 g[GH][72], int w)
{
    int c0 = -1, c1 = -1;
    for (int c = 0; c < w; c++) if (g[R_(240)][c] > 127) { if (c0 < 0) c0 = c; c1 = c; }
    return c0 < 0 ? w / 2 : round_half_even((c0 + c1) / 2.0);
}
static void crop(const u8 *atl, int x0, int w, u8 g[GH][72])
{
    memset(g, 0, GH * 72);
    for (int r = 0; r < GH; r++) for (int c = 0; c < w; c++) g[r][c] = at_a(atl, x0 + c, GY0 + r);
}
/* the line, exactly as the approved mock-up (mock.py compose, offset 112, centres 773/900/1027/1154/1281).
   Returns 0 (nothing built) when the atlas' letters are not where they were measured. */
static int build_canvas(const u8 *atl)
{
    static u8 C[GH][72], O[GH][72], D[GH][72], T[GH][72], P[GH][72], DS[GH][72];
    crop(atl, 742, 64, C); crop(atl, 1740, 69, O); crop(atl, 983, 68, D); crop(atl, 1530, 56, T);
    /* P: D's strand and drop, the top and the bottom of D's bowl stacked, D's stem below, a rounded foot */
    memset(P, 0, sizeof P);
    for (int r = 0; r < R_(276); r++) memcpy(P[r], D[r], 72);
    int top0 = R_(276), ntop = R_(302) - R_(276), bot0 = R_(333), nbot = R_(354) - R_(333);
    for (int r = 0; r < ntop; r++) memcpy(P[R_(276) + r], D[top0 + r], 72);
    for (int r = 0; r < nbot; r++) memcpy(P[R_(276) + ntop + r], D[bot0 + r], 72);
    int bowl_end = R_(276) + ntop + nbot, nseg = R_(354) - bowl_end;
    for (int r = 0; r < nseg; r++) for (int c = 0; c < 1005 - 983; c++) {
        u8 v = D[R_(302) + r][c]; u8 *d = &P[bowl_end - 4 + r][c]; if (v > *d) *d = v; }
    static const int taper[5] = { 11, 10, 9, 7, 5 };
    for (int k = 0; k < 5; k++) {
        int r = R_(354) - 5 + k, c0 = -1, c1 = -1;
        for (int c = 0; c < 22; c++) if (P[r][c] > 127) { if (c0 < 0) c0 = c; c1 = c; }
        if (c0 < 0) continue;
        double mid = (c0 + c1) / 2.0;
        for (int c = 0; c < 22; c++) if (fabs(c - mid) > taper[k] / 2.0) P[r][c] = 0;
    }
    /* dash: T's cross-bar mirrored into a pill at mid-height, with T's drop above it and T's strand up to the top */
    memset(DS, 0, sizeof DS);
    int bar_top = R_(279), bar_mid = R_(286), nhalf = bar_mid - bar_top, py0 = R_(313) - nhalf;
    int drop0 = R_(262), ndrop = bar_top - drop0;
    for (int r = 0; r < ndrop; r++) memcpy(DS[py0 - ndrop + r], T[drop0 + r], 72);
    for (int r = 0; r < nhalf; r++) { memcpy(DS[py0 + r], T[bar_top + r], 72); memcpy(DS[py0 + 2 * nhalf - 1 - r], T[bar_top + r], 72); }
    for (int r = 0; r < py0 - ndrop; r++) memcpy(DS[r], T[R_(240)], 72);
    /* compose */
    u8 (*glyph[5])[72] = { C, O, DS, O, P };
    static const int widths[5] = { 64, 69, 56, 69, 68 }, centres[5] = { 773, 900, 1027, 1154, 1281 };
    static const int want_sx[5] = { 31, 34, 27, 34, 31 }, want_top[5] = { 352, 353, 350, 222, 222 };
    for (int i = 0; i < 5; i++) {                                                  /* the letters this was fitted to */
        int sx = strand_center(glyph[i], widths[i]), x = centres[i], top = 0;
        for (int r = 0; r < GY0 + CO_OFF + R_(272) - 6; r++) if (at_a(atl, x, r) > 127) top = r;
        if (sx != want_sx[i] || top < want_top[i] - 2 || top > want_top[i] + 2) return 0;
    }
    memset(s_canvas, 0, sizeof s_canvas);
    for (int i = 0; i < 5; i++) {
        static u8 g[GH][72];
        int w = widths[i], sx = strand_center(glyph[i], w), x0 = centres[i] - sx, y0 = GY0 + CO_OFF, x = x0 + sx;
        int y_body = GY0 + CO_OFF + R_(272), top = 0;
        for (int r = 0; r < y_body - 6; r++) if (at_a(atl, x, r) > 127) top = r;   /* letter / logo straight above */
        memcpy(g, glyph[i], sizeof g);
        int cut = top - 1 - y0; if (cut < 0) cut = 0;
        for (int r = 0; r < cut && r < GH; r++) memset(g[r], 0, 72);              /* no strand inside that letter */
        for (int r = 0; r < GH; r++) for (int c = 0; c < w; c++) {
            u8 *d = &s_canvas[y0 + r - CY0][x0 + c - CX0]; if (g[r][c] > *d) *d = g[r][c]; }
        for (int r = top - 1; r < y0 + 1; r++) for (int k = -1; k <= 1; k++) {      /* thread up to it */
            u8 v = g[2][sx + k], *d = &s_canvas[r - CY0][x + k - CX0]; if (v > *d) *d = v; }
    }
    return 1;
}
static int pieces_empty(const u8 *atl)
{
    for (int p = 0; p < 2; p++) {
        const Piece *q = &kPieces[p];
        for (int y = q->dy - 4; y < q->dy + (q->ay1 - q->ay0) + 4; y++)
            for (int x = q->dx - 4; x < q->dx + (q->ax1 - q->ax0) + 4; x++) if (at_a(atl, x, y)) return 0;
    }
    return 1;
}
/* make sure the atlas buffer carries the CO-OP pieces (once per load; cheap check otherwise) */
static void title_atlas_prepare(u8 *atl)
{
    if (!s_enabled || s_atlas_bad || !atl) return;
    if (!atlas_ok(atl)) { s_atlas_bad = 1; s_coop_ok = 0; TLOG("title: the title atlas is not the Enhanced Edition's 2048x2048 start01e - CO-OP line left out"); return; }
    if (has_mark(atl)) return;
    if (!pieces_empty(atl)) { s_atlas_bad = 1; s_coop_ok = 0; TLOG("title: the atlas corner the CO-OP line needs is in use (another mod?) - CO-OP line left out"); return; }
    if (!build_canvas(atl)) { s_atlas_bad = 1; s_coop_ok = 0; TLOG("title: the atlas' letters are not where they were measured (another Enhanced Edition version?) - CO-OP line left out"); return; }
    for (int p = 0; p < 2; p++) {
        const Piece *q = &kPieces[p];
        for (int y = q->ay0; y < q->ay1; y++) for (int x = q->ax0; x < q->ax1; x++)
            atl[AT_DATA + ((size_t)(q->dy + y - q->ay0) * AT_W + (q->dx + x - q->ax0)) * 4 + 3] = s_canvas[y - CY0][x - CX0];
    }
    u8 *m = atl + AT_DATA + ((size_t)MARK_Y * AT_W + MARK_X) * 4; m[0] = 0x43; m[1] = 0x4F; m[2] = 0x4F; m[3] = 0;
    s_coop_ok = 1;
    if (s_atlas_patches++ == 0) TLOG("title: CO-OP line built from the atlas' own letters (C, O, D->P, T->dash) into its free corner");
    else if (s_atlas_patches <= 4 || s_atlas_patches % 25 == 0) TLOG("title: atlas loaded again - CO-OP line added again (load %d)", s_atlas_patches);
}

/* ============================== background (DXT3) ============================== */
static int bg_ok(const u8 *b)
{
    return b && *(const u32 *)b == TEX_MAGIC && *(const u32 *)(b + 16) == 0x66 && *(const u32 *)(b + 20) == 0x08000aac && b[48] == 0x33;
}
static u32 bg_fingerprint(const u8 *b)
{
    u32 h = 2166136261u;
    for (int k = 0; k < 32; k++) {
        const u8 *blk = b + BG_DATA + ((size_t)(150 + k * 4) * BG_BW + 300 + k * 8) * 16;
        for (int i = 0; i < 16; i++) { h ^= blk[i]; h *= 16777619u; }
    }
    return h;
}
static inline void dec565(u16 c, float *o) { int r = c >> 11, g = (c >> 5) & 63, b = c & 31; o[0] = (float)((r << 3) | (r >> 2)); o[1] = (float)((g << 2) | (g >> 4)); o[2] = (float)((b << 3) | (b >> 2)); }
static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline u16 enc565(const float *c)
{
    int r = clampi((int)lrintf(c[0] * 31.0f / 255.0f), 0, 31), g = clampi((int)lrintf(c[1] * 63.0f / 255.0f), 0, 63), b = clampi((int)lrintf(c[2] * 31.0f / 255.0f), 0, 31);
    return (u16)((r << 11) | (g << 5) | b);
}
static void green(const float *c, float *o)
{
    float L = (0.299f * c[0] + 0.587f * c[1] + 0.114f * c[2]) / 255.0f;
    int i = 0; L = L < 0 ? 0 : L > 1 ? 1 : L;
    while (i < 2 && L > kStopL[i + 1]) i++;
    float t = (L - kStopL[i]) / (kStopL[i + 1] - kStopL[i]);
    for (int k = 0; k < 3; k++) o[k] = kStopC[i][k] + (kStopC[i + 1][k] - kStopC[i][k]) * t;
}
static void dxt_decode(const u8 *blk, float px[16][3])
{
    u16 c0 = (u16)(blk[8] | blk[9] << 8), c1 = (u16)(blk[10] | blk[11] << 8);
    u32 idx = blk[12] | blk[13] << 8 | blk[14] << 16 | (u32)blk[15] << 24;
    float p[4][3]; dec565(c0, p[0]); dec565(c1, p[1]);
    for (int k = 0; k < 3; k++) { p[2][k] = (2 * p[0][k] + p[1][k]) / 3.0f; p[3][k] = (p[0][k] + 2 * p[1][k]) / 3.0f; }   /* DXT3: always 4 colours */
    for (int i = 0; i < 16; i++) memcpy(px[i], p[(idx >> (2 * i)) & 3], sizeof(float) * 3);
}
/* colour half of a block from 16 colours (principal axis fit); the alpha half is left alone */
static void dxt_encode(u8 *blk, float px[16][3])
{
    float m[3] = { 0, 0, 0 };
    for (int i = 0; i < 16; i++) for (int k = 0; k < 3; k++) m[k] += px[i][k] / 16.0f;
    float cv[6] = { 0 };
    for (int i = 0; i < 16; i++) {
        float d0 = px[i][0] - m[0], d1 = px[i][1] - m[1], d2 = px[i][2] - m[2];
        cv[0] += d0 * d0; cv[1] += d0 * d1; cv[2] += d0 * d2; cv[3] += d1 * d1; cv[4] += d1 * d2; cv[5] += d2 * d2;
    }
    float ax[3] = { 0.577f, 0.577f, 0.577f };
    for (int it = 0; it < 8; it++) {
        float n0 = cv[0] * ax[0] + cv[1] * ax[1] + cv[2] * ax[2], n1 = cv[1] * ax[0] + cv[3] * ax[1] + cv[4] * ax[2], n2 = cv[2] * ax[0] + cv[4] * ax[1] + cv[5] * ax[2];
        float l = sqrtf(n0 * n0 + n1 * n1 + n2 * n2); if (l < 1e-6f) break;
        ax[0] = n0 / l; ax[1] = n1 / l; ax[2] = n2 / l;
    }
    float tmin = 1e9f, tmax = -1e9f;
    for (int i = 0; i < 16; i++) { float t = (px[i][0] - m[0]) * ax[0] + (px[i][1] - m[1]) * ax[1] + (px[i][2] - m[2]) * ax[2]; if (t < tmin) tmin = t; if (t > tmax) tmax = t; }
    float in = (tmax - tmin) / 32.0f, e0[3], e1[3];
    for (int k = 0; k < 3; k++) { e0[k] = m[k] + ax[k] * (tmax - in); e1[k] = m[k] + ax[k] * (tmin + in); }
    u16 q0 = enc565(e0), q1 = enc565(e1);
    if (q0 < q1) { u16 t = q0; q0 = q1; q1 = t; }
    u32 idx = 0;
    if (q0 != q1) {
        float p[4][3]; dec565(q0, p[0]); dec565(q1, p[1]);
        for (int k = 0; k < 3; k++) { p[2][k] = (2 * p[0][k] + p[1][k]) / 3.0f; p[3][k] = (p[0][k] + 2 * p[1][k]) / 3.0f; }
        for (int i = 0; i < 16; i++) {
            int best = 0; float be = 1e30f;
            for (int j = 0; j < 4; j++) { float e = 0; for (int k = 0; k < 3; k++) { float d = px[i][k] - p[j][k]; e += d * d; } if (e < be) { be = e; best = j; } }
            idx |= (u32)best << (2 * i);
        }
    }
    blk[8] = (u8)q0; blk[9] = (u8)(q0 >> 8); blk[10] = (u8)q1; blk[11] = (u8)(q1 >> 8);
    blk[12] = (u8)idx; blk[13] = (u8)(idx >> 8); blk[14] = (u8)(idx >> 16); blk[15] = (u8)(idx >> 24);
}
/* exact 1-D squared distance transform (Felzenszwalb-Huttenlocher), unit spacing, finite f */
static void edt1d(const double *f, int n, double *d, int *v, double *z)
{
    int k = 0; v[0] = 0; z[0] = -1e30; z[1] = 1e30;
    for (int q = 1; q < n; q++) {
        double s = ((f[q] + (double)q * q) - (f[v[k]] + (double)v[k] * v[k])) / (2.0 * q - 2.0 * v[k]);
        while (s <= z[k]) { k--; s = ((f[q] + (double)q * q) - (f[v[k]] + (double)v[k] * v[k])) / (2.0 * q - 2.0 * v[k]); }
        k++; v[k] = q; z[k] = s; z[k + 1] = 1e30;
    }
    k = 0;
    for (int q = 0; q < n; q++) { while (z[k + 1] < q) k++; d[q] = (double)(q - v[k]) * (q - v[k]) + f[v[k]]; }
}
#define BK_TX0 992                    /* baked region in texture texels (block aligned): x 992..1727, y 764..1335 */
#define BK_TX1 1728
#define BK_TY0 764
#define BK_TY1 1336
#define BKW (BK_TX1 - BK_TX0)
#define BKH (BK_TY1 - BK_TY0)
static float s_bm[BKH][BKW], s_bd[BKH][BKW];
static float canvas_at(float ax, float ay)            /* bilinear, 0..1 (pixel centres on integers) */
{
    float fx = ax - CX0, fy = ay - CY0;
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy); float tx = fx - x0, ty = fy - y0, v = 0;
    for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) {
        int x = x0 + i, y = y0 + j;
        if (x < 0 || y < 0 || x >= CW || y >= CH) continue;
        v += s_canvas[y][x] / 255.0f * (i ? tx : 1 - tx) * (j ? ty : 1 - ty);
    }
    return v;
}
static float glow_at(float d)
{
    int n = (int)(sizeof kGlowD / sizeof kGlowD[0]);
    if (d >= kGlowD[n - 1]) return 0;
    int i = 0; while (i < n - 2 && d > kGlowD[i + 1]) i++;
    return kGlowE[i] + (kGlowE[i + 1] - kGlowE[i]) * (d - kGlowD[i]) / (kGlowD[i + 1] - kGlowD[i]);
}
static int bake_coop(u8 *bg)
{
    static double f[BKW], dd[BKW], z[BKW + 1];
    static int v[BKW];
    for (int y = 0; y < BKH; y++) for (int x = 0; x < BKW; x++)
        s_bm[y][x] = canvas_at(BK_TX0 + x - BK_OX, (BK_TY0 + y - BK_OY) / BK_SY);
    /* distance (atlas texels) from the letters; texture rows are 1/1.4 atlas texel apart.  Columns first (nearest
       letter texel above / below), then the exact transform along the rows. */
    for (int x = 0; x < BKW; x++) {
        int last = -1000000;
        for (int y = 0; y < BKH; y++) { if (s_bm[y][x] > 0.5f) last = y; s_bd[y][x] = (float)(y - last); }
        last = 1000000;
        for (int y = BKH - 1; y >= 0; y--) { if (s_bm[y][x] > 0.5f) last = y; if (last - y < s_bd[y][x]) s_bd[y][x] = (float)(last - y); }
        for (int y = 0; y < BKH; y++) { float dy = s_bd[y][x] / BK_SY; s_bd[y][x] = s_bd[y][x] > 100000 ? 1e12f : dy * dy; }
    }
    for (int y = 0; y < BKH; y++) {
        for (int x = 0; x < BKW; x++) f[x] = s_bd[y][x];
        edt1d(f, BKW, dd, v, z);
        for (int x = 0; x < BKW; x++) s_bd[y][x] = (float)sqrt(dd[x]);
    }
    int changed = 0;
    for (int by = BK_TY0 / 4; by < BK_TY1 / 4; by++) for (int bx = BK_TX0 / 4; bx < BK_TX1 / 4; bx++) {
        int any = 0;
        for (int i = 0; i < 16 && !any; i++) { int y = by * 4 + i / 4 - BK_TY0, x = bx * 4 + i % 4 - BK_TX0; any = s_bm[y][x] > 0.002f || glow_at(s_bd[y][x]) > 0.25f; }
        if (!any) continue;
        u8 *blk = bg + BG_DATA + ((size_t)by * BG_BW + bx) * 16;
        float px[16][3]; dxt_decode(blk, px);
        for (int i = 0; i < 16; i++) {
            int y = by * 4 + i / 4 - BK_TY0, x = bx * 4 + i % 4 - BK_TX0;
            float m = s_bm[y][x], e = glow_at(s_bd[y][x]);
            for (int k = 0; k < 3; k++) { float c = px[i][k] + e; if (c > 255) c = 255; px[i][k] = c * (1 - m) + kSil[k] * m; }
        }
        dxt_encode(blk, px); changed++;
    }
    return changed;
}
/* the background this bake was fitted to: ENHANCED's C and D strands, baked black between two bright texels, at
   texture row 909 (C: x 1106-1107, D: x 1347-1348) */
static int bg_landmarks_ok(const u8 *bg)
{
    static const int bx[4] = { 276, 277, 336, 337 }; float L[16 * 4];
    for (int b = 0; b < 4; b++) {
        float px[16][3]; dxt_decode(bg + BG_DATA + ((size_t)227 * BG_BW + bx[b]) * 16, px);
        for (int c = 0; c < 4; c++) L[b * 4 + c] = (px[4 + c][0] + px[4 + c][1] + px[4 + c][2]) / 3.0f;   /* row 909 = 2nd row */
    }
    /* L[i] = texture x (1104 + i) for i < 8, x (1344 + i - 8) for i >= 8 */
    return L[2] < 25 && L[3] < 25 && L[0] > 70 && L[5] > 70 && L[11] < 25 && L[12] < 25 && L[9] > 60 && L[14] > 60;
}
static void title_bg_prepare_with(u8 *bg, u8 *atl)
{
    if (!s_enabled || s_bg_bad || !bg) return;
    if (!bg_ok(bg)) { s_bg_bad = 1; TLOG("title: the background is not the Enhanced Edition's 2732x2048 start00 - left as it is"); return; }
    u32 fp = bg_fingerprint(bg);
    if (s_bg_fp_done && fp == s_bg_fp_done) return;                         /* already done for this load */
    if (s_bg_fp_orig && fp != s_bg_fp_orig) { s_bg_bad = 1; TLOG("title: the background data changed (another mod?) - left as it is from now on"); return; }
    s_bg_fp_orig = fp;
    DWORD t0 = GetTickCount();
    title_atlas_prepare(atl);                                               /* the CO-OP letters, if not made yet */
    int lm = bg_landmarks_ok(bg);
    if (!lm && s_coop_ok && s_bg_patches == 0) TLOG("title: the background is not the one the CO-OP lettering was fitted to - CO-OP not baked into it (drawn over it only)");
    int baked = s_coop_ok && lm ? bake_coop(bg) : 0;
    /* copyright line: keep its original colours for the red pass below */
    static float cr[CR_BY1 - CR_BY0][CR_BX1 - CR_BX0][16][3];
    for (int by = CR_BY0; by < CR_BY1; by++) for (int bx = CR_BX0; bx < CR_BX1; bx++)
        dxt_decode(bg + BG_DATA + ((size_t)by * BG_BW + bx) * 16, cr[by - CR_BY0][bx - CR_BX0]);
    /* every other block: the two end-points through the green ramp (same order kept: c0 > c1, indices swapped) */
    for (int by = 0; by < BG_BH; by++) for (int bx = 0; bx < BG_BW; bx++) {
        if (by >= CR_BY0 && by < CR_BY1 && bx >= CR_BX0 && bx < CR_BX1) continue;
        u8 *blk = bg + BG_DATA + ((size_t)by * BG_BW + bx) * 16;
        float a[3], b[3], ga[3], gb[3];
        dec565((u16)(blk[8] | blk[9] << 8), a); dec565((u16)(blk[10] | blk[11] << 8), b);
        green(a, ga); green(b, gb);
        u16 q0 = enc565(ga), q1 = enc565(gb);
        u32 idx = blk[12] | blk[13] << 8 | blk[14] << 16 | (u32)blk[15] << 24;
        if (q0 < q1) { u16 t = q0; q0 = q1; q1 = t; idx ^= 0x55555555u; }
        if (q0 == q1) idx = 0;
        blk[8] = (u8)q0; blk[9] = (u8)(q0 >> 8); blk[10] = (u8)q1; blk[11] = (u8)(q1 >> 8);
        blk[12] = (u8)idx; blk[13] = (u8)(idx >> 8); blk[14] = (u8)(idx >> 16); blk[15] = (u8)(idx >> 24);
    }
    for (int by = CR_BY0; by < CR_BY1; by++) for (int bx = CR_BX0; bx < CR_BX1; bx++) {
        float (*px)[3] = cr[by - CR_BY0][bx - CR_BX0], out[16][3];
        for (int i = 0; i < 16; i++) {
            float L = (px[i][0] + px[i][1] + px[i][2]) / 3.0f, e = (L - 16.0f) / 24.0f, g[3];
            e = e < 0 ? 0 : e > 1 ? 1 : e;
            green(px[i], g);
            for (int k = 0; k < 3; k++) out[i][k] = g[k] * (1 - e) + kCopyRed[k] * e;
        }
        dxt_encode(bg + BG_DATA + ((size_t)by * BG_BW + bx) * 16, out);
    }
    s_bg_fp_done = bg_fingerprint(bg);
    if (s_bg_patches++ == 0)
        TLOG("title: background recoloured blue -> green, copyright line dark red, %s - %lu ms", baked ? "CO-OP baked in like the logo" : "CO-OP NOT baked in",
             (unsigned long)(GetTickCount() - t0));
    else if (s_bg_patches <= 4 || s_bg_patches % 25 == 0)
        TLOG("title: background loaded again - recoloured again (load %d, %lu ms)", s_bg_patches, (unsigned long)(GetTickCount() - t0));
}
static void title_bg_prepare(u8 *bg) { if (s_enabled && !s_bg_bad && bg) title_bg_prepare_with(bg, *(u8 **)0x0094D60C); }

/* ============================== drawing CO-OP (after the logo) ============================== */
#define T_SPR 0x0094D5C4
typedef int (__cdecl *SetTexFn)(void *, void *, int, int);
typedef void (__cdecl *SetAlphaFn)(void *, int, int, int, int, int, int);
typedef void (__cdecl *QueueFn)(void *);
static void draw_piece(const Piece *q, int lx, int ly, const int *col)
{
    u8 *s = (u8 *)T_SPR;
    const short *e0 = (const short *)T_SPRTAB;                              /* entry 0: u0 v0 u1 v1 w h */
    float sx = (float)e0[4] / (4.0f * (e0[2] - e0[0])), sy = (float)e0[5] / (4.0f * (e0[3] - e0[1]));   /* virtual units per atlas texel */
    *(u16 *)(s + 0x2e) &= 0x101;
    ((SetTexFn)0x0049F9C0)(s, *(void **)0x0094D60C, 0, 0);
    *(u32 *)(s + 0x38) = ((u32)col[0] << 16 | (u32)col[1] << 8 | (u32)col[2]) | (*(u32 *)(s + 0x38) & 0xff000000u);
    ((SetAlphaFn)0x0049EF20)(s, col[3], 0, 1, 0, 1, 0x80);
    *(u16 *)(s + 0x2c) = 5;
    *(short *)(s + 0x0c) = (short)lrintf((lx + q->ax0 * sx) * 16.0f);
    *(short *)(s + 0x0e) = (short)lrintf((ly + q->ay0 * sy) * 16.0f);
    *(short *)(s + 0x10) = (short)lrintf((lx + q->ax1 * sx) * 16.0f);
    *(short *)(s + 0x12) = (short)lrintf((ly + q->ay1 * sy) * 16.0f);
    *(float *)(s + 0x1c) = q->dx * 4.0f;
    *(float *)(s + 0x20) = q->dy * 4.0f;
    *(float *)(s + 0x24) = (q->dx + q->ax1 - q->ax0) * 4.0f;
    *(float *)(s + 0x28) = (q->dy + q->ay1 - q->ay0) * 4.0f;
    *(u16 *)(s + 0x2e) |= 6;
    ((QueueFn)0x0049EF70)(s);
}
void __cdecl title_after_logo(int x, int y)
{
    u8 *atl = *(u8 **)0x0094D60C;
    if (!s_enabled || !s_coop_ok || !atl || !atlas_ok(atl) || !has_mark(atl)) return;
    const int *c = (const int *)(T_SPRTAB + 12);                            /* the logo's colour this frame */
    for (int p = 0; p < 2; p++) draw_piece(&kPieces[p], x, y, c);
}
extern void title_logo_stub(void);
__asm__(
    ".text\n"
    "_title_logo_stub:\n"                       /* replaces "call 0x495C20" for the logo: x on the stack, y in ebx */
    "    pushl 4(%esp)\n"
    "    movl $0x495C20, %eax\n"
    "    call *%eax\n"
    "    addl $4, %esp\n"
    "    pushal\n"
    "    pushl %ebx\n"
    "    pushl 40(%esp)\n"
    "    call _title_after_logo\n"
    "    addl $8, %esp\n"
    "    popal\n"
    "    ret\n"
);
static int __cdecl bg_settex_hook(void *spr, void *buf, int a, int b) { title_bg_prepare((u8 *)buf); return ((SetTexFn)0x0049F9C0)(spr, buf, a, b); }
static int __cdecl atlas_settex_hook(void *spr, void *buf, int a, int b) { title_atlas_prepare((u8 *)buf); return ((SetTexFn)0x0049F9C0)(spr, buf, a, b); }

/* ============================== install ============================== */
static int t_call_to(u32 site, u32 expect)
{
    return *(const u8 *)site == 0xE8 && site + 5 + *(const int32_t *)(site + 1) == expect;
}
static int t_patch_call(u32 site, void *target)
{
    DWORD old;
    if (!VirtualProtect((void *)site, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *(int32_t *)(site + 1) = (int32_t)((u32)target - (site + 5));
    VirtualProtect((void *)site, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)site, 5);
    return 1;
}
static int t_bytes(u32 a, const u8 *b, int n) { return memcmp((const void *)a, b, n) == 0; }
int title_install(TitleLogFn log, int enable)
{
    s_tlog = log;
    if (!enable) { TLOG("title screen: original (sh2coop.ini [coop] title_screen = 0)"); return 0; }
    /* the three call sites and the code around them must be the ones traced */
    static const u8 bgsite[] = { 0x6a, 0x00, 0x6a, 0x00, 0x50, 0x68, 0x80, 0xd5, 0x94, 0x00 };            /* 0x49695F */
    static const u8 atsite[] = { 0x6a, 0x00, 0x6a, 0x00, 0x50, 0x68, 0xc4, 0xd5, 0x94, 0x00 };            /* 0x495C34 */
    static const u8 logo[] = { 0xbe, 0xd8, 0xbe, 0x88, 0x00, 0xb8, 0xc8, 0xbe, 0x88, 0x00, 0xbb, 0x4c, 0xbc, 0x88, 0x00 };  /* 0x496A2A */
    static const u8 drawspr[] = { 0xa1, 0x0c, 0xd6, 0x94, 0x00, 0x66, 0x81, 0x25, 0xf2, 0xd5, 0x94, 0x00, 0x01, 0x01 };   /* 0x495C20 */
    if (!t_call_to(0x00496969, 0x0049F9C0) || !t_call_to(0x00495C3E, 0x0049F9C0) || !t_call_to(0x00496A4A, 0x00495C20)
        || !t_bytes(0x0049695F, bgsite, sizeof bgsite) || !t_bytes(0x00495C34, atsite, sizeof atsite)
        || !t_bytes(0x00496A2A, logo, sizeof logo) || !t_bytes(0x00495C20, drawspr, sizeof drawspr)) {
        TLOG("title screen: the title code differs from the traced one - left original");
        return 0;
    }
    /* colour: only the logo's (entry 0: it pulses dark red <-> black like the white <-> black original); the menu
       words (entries 1-22) and the cursor glow keep the game's own colours (P33f) */
    DWORD old;
    if (VirtualProtect((void *)T_COLTAB, 0x28, PAGE_READWRITE, &old)) {
        int *e = (int *)T_COLTAB;
        memcpy(e, kLogoPeak, 16); memcpy(e + 4, kLogoLow, 16);
        VirtualProtect((void *)T_COLTAB, 0x28, old, &old);
    }
    s_enabled = 1;
    int n = t_patch_call(0x00496969, (void *)bg_settex_hook) + t_patch_call(0x00495C3E, (void *)atlas_settex_hook)
          + t_patch_call(0x00496A4A, (void *)title_logo_stub);
    TLOG("title screen: CO-OP under the logo, green background, dark red logo and copyright, menu words as the game's (%d of 3 hooks)", n);
    return n;
}

#ifdef TITLE_OFFLINE
/* offline check: the same code on the real files; writes the canvas, the patched atlas and the patched background,
   then runs both prepares again to show they leave patched data alone */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
static void plog(const char *fmt, ...) { va_list a; va_start(a, fmt); vprintf(fmt, a); va_end(a); printf("\n"); }
static u8 *readf(const char *p, long *n) { FILE *f = fopen(p, "rb"); fseek(f, 0, SEEK_END); *n = ftell(f); fseek(f, 0, SEEK_SET); u8 *b = malloc(*n); if (fread(b, 1, *n, f) != (size_t)*n) exit(1); fclose(f); return b; }
static void writef(const char *p, const void *b, long n) { FILE *f = fopen(p, "wb"); fwrite(b, 1, n, f); fclose(f); }
int main(int argc, char **argv)
{
    long na, nb; u8 *atl = readf(argv[1], &na), *bg = readf(argv[2], &nb);
    (void)argc; s_tlog = plog; s_enabled = 1;
    title_bg_prepare_with(bg, atl);
    writef(argv[3], s_canvas, sizeof s_canvas);
    writef(argv[4], atl, na); writef(argv[5], bg, nb);
    u32 a = s_bg_fp_done; int pa = s_atlas_patches, pb = s_bg_patches;
    title_bg_prepare_with(bg, atl); title_atlas_prepare(atl);
    printf("second pass: atlas patches %d -> %d, background patches %d -> %d, fingerprint %08x -> %08x, coop_ok %d\n",
           pa, s_atlas_patches, pb, s_bg_patches, a, s_bg_fp_done, s_coop_ok);
    return 0;
}
#endif
