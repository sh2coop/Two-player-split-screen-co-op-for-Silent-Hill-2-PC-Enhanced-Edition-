/* sh2coop_chr.c - play as Maria, Eddie or Angela (experimental build only)
 *
 * The James work stays James: his own code, animation, collision, weapon and voice. Each frame his 41 animated bone
 * nodes (work+0x9C list, 4x4 at node+8, character space, row vectors) are retargeted onto the chosen character's
 * skeleton from her own model file (data\chr\<x>\*.mdl): every mapped body bone takes James's rotation change from
 * his bind pose, bone lengths stay the character's own, the hips follow James's hips scaled by leg length; unmapped
 * bones (face, hair, skirt) ride rigidly on their parent. The mesh is skinned on the CPU (up to 4 bones per vertex)
 * into world space with the work's matrix (work+0x5C) and drawn with the game's view/projection (0x1F80E00 /
 * 0x1F80E40) instead of James, while James's own draw is skipped.
 *
 * Hands (CS2). James's in-game model (hll_jms.mdl) has no finger bones: each hand is one of 18 modelled hand shapes
 * (parts 3..13 right, 14..20 left, part id = u16 at primitive header +0x26), picked every frame by the part mask at
 * (*(work+0x2B0))+0x10 (0x50E370: bit 0 + right hand + left hand + 21 = the flashlight in his pocket). The characters'
 * models have real finger bones instead, so the mask is turned into finger curls (table below, measured from James's
 * hand shapes). The hand bones are lined up by a "grip frame" made from each skeleton's own knuckles (palm centre,
 * finger direction, knuckle line), so the character's palm sits exactly as James's does; whatever James holds is
 * moved from his grip frame to hers (chr_weapon_begin), and when his left hand holds the weapon too, her left arm is
 * bent (two-bone IK) to put her left grip where his is relative to the weapon.
 * Mapping tables were made from the bind poses (positions + hierarchy) of hhh_jms / hhh_mar / hhh_edi / agl. */
#include <windows.h>
#include <d3d8.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "sh2coop_chr.h"
#include "sh2coop_ce.h"

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
void ce_log(const char *fmt, ...);

#define NLAB 26
/* pelvis hips spine1 spine2 neck head clavL clavR armL armR elbowL elbowR foreL foreR handL handR
   thighL thighR kneeL kneeR shinL shinR ankleL ankleR toeL toeR */
enum { L_PELVIS, L_HIPS, L_SPINE1, L_SPINE2, L_NECK, L_HEAD, L_CLAVL, L_CLAVR, L_ARML, L_ARMR, L_ELBOWL, L_ELBOWR,
       L_FOREL, L_FORER, L_HANDL, L_HANDR };
static const int MAP_JMS[NLAB] = { 0,1,2,3,8,15,9,10,27,28,31,32,33,34,40,35,4,5,6,7,13,14,25,26,29,30 };
static const struct { const char *name, *file; int map[NLAB]; } kChars[CHR_COUNT] = {
    { "James",  NULL, {0} },
    { "Maria",  "data\\chr\\mar\\hhh_mar.mdl", { 0,1,2,4,6,9,11,16,29,32,34,36,37,38,39,41,3,5,7,8,10,15,27,30,33,35 } },
    { "Eddie",  "data\\chr\\edi\\hhh_edi.mdl", { 0,1,2,5,6,13,9,10,17,22,23,26,27,28,30,31,3,4,7,8,14,15,18,19,24,25 } },
    { "Angela", "data\\chr\\agl\\agl.mdl",     { 0,1,2,3,6,9,12,15,31,32,33,35,37,39,42,44,4,7,10,13,16,17,34,36,38,40 } },
};
const char *chr_name(int k) { return k >= 0 && k < CHR_COUNT ? kChars[k].name : "?"; }

/* finger curl per James hand shape (part id 3..20): thumb, index, middle, ring, little; 0 = the model's relaxed
   bind hand, 1 = closed fist. Measured from the reach of each finger in James's 18 hand meshes (hll_jms.mdl) and
   checked against renders of them: 3/14 relaxed, 4/15 fist, 5/16 round grip, 7 = handgun (index on the trigger)... */
static const float kCurl[21][5] = {
    {0}, {0}, {0},
    { 0.15f, 0.28f, 0.32f, 0.36f, 0.40f },   /*  3 R relaxed (the modelled hand is softly bent) */
    { 0.70f, 1.00f, 1.00f, 1.00f, 1.00f },   /*  4 R fist */
    { 0.55f, 0.80f, 0.75f, 0.72f, 0.80f },   /*  5 R round grip */
    { 0.30f, 0.55f, 0.50f, 0.85f, 1.00f },   /*  6 R half open */
    { 0.60f, 0.35f, 0.75f, 0.75f, 0.90f },   /*  7 R handgun */
    { 0.30f, 0.55f, 0.45f, 0.60f, 1.00f },   /*  8 R */
    { 0.65f, 0.92f, 0.90f, 1.00f, 1.00f },   /*  9 R tight grip */
    { 0.60f, 0.85f, 0.80f, 0.80f, 1.00f },   /* 10 R grip */
    { 0.05f, 0.30f, 0.45f, 0.25f, 0.25f },   /* 11 R spread */
    { 0.50f, 0.60f, 0.60f, 0.70f, 0.82f },   /* 12 R loose grip */
    { 0.65f, 0.85f, 0.80f, 0.90f, 0.95f },   /* 13 R grip */
    { 0.15f, 0.28f, 0.32f, 0.36f, 0.40f },   /* 14 L relaxed / open support hand */
    { 0.70f, 1.00f, 1.00f, 1.00f, 1.00f },   /* 15 L fist */
    { 0.55f, 0.80f, 0.75f, 0.72f, 0.80f },   /* 16 L round grip */
    { 0.30f, 0.47f, 0.41f, 0.59f, 0.65f },   /* 17 L half open */
    { 0.65f, 0.92f, 0.90f, 1.00f, 1.00f },   /* 18 L tight grip */
    { 0.60f, 0.85f, 0.80f, 0.80f, 1.00f },   /* 19 L grip */
    { 0.50f, 0.60f, 0.60f, 0.70f, 0.82f },   /* 20 L loose grip */
};
/* joint angles (radians) at curl 1: fingers knuckle / middle / end joint, thumb base / middle / end */
static const float kFingerMax[3] = { 1.40f, 1.66f, 1.05f };
static const float kThumbMax[3]  = { 0.45f, 0.60f, 0.70f };

typedef struct { float m[16]; } M4;                 /* row vectors: v' = v * M ; translation in m[12..14] */
static void m_id(M4 *o) { memset(o, 0, sizeof *o); o->m[0] = o->m[5] = o->m[10] = o->m[15] = 1.0f; }
static void m_mul(M4 *o, const M4 *a, const M4 *b)
{
    M4 r;
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
        r.m[i * 4 + j] = a->m[i * 4] * b->m[j] + a->m[i * 4 + 1] * b->m[4 + j] + a->m[i * 4 + 2] * b->m[8 + j] + a->m[i * 4 + 3] * b->m[12 + j];
    *o = r;
}
static void m_rigid_inv(M4 *o, const M4 *a)                 /* inverse of rotation + translation */
{
    M4 r; memset(&r, 0, sizeof r);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r.m[i * 4 + j] = a->m[j * 4 + i];
    for (int j = 0; j < 3; j++) r.m[12 + j] = -(a->m[12] * r.m[j] + a->m[13] * r.m[4 + j] + a->m[14] * r.m[8 + j]);
    r.m[15] = 1.0f; *o = r;
}
static void v_mul(float *o, const float *v, const M4 *a)    /* point */
{
    float x = v[0], y = v[1], z = v[2];
    o[0] = x * a->m[0] + y * a->m[4] + z * a->m[8] + a->m[12];
    o[1] = x * a->m[1] + y * a->m[5] + z * a->m[9] + a->m[13];
    o[2] = x * a->m[2] + y * a->m[6] + z * a->m[10] + a->m[14];
}
static void n_mul(float *o, const float *v, const M4 *a)    /* direction */
{
    float x = v[0], y = v[1], z = v[2];
    o[0] = x * a->m[0] + y * a->m[4] + z * a->m[8];
    o[1] = x * a->m[1] + y * a->m[5] + z * a->m[9];
    o[2] = x * a->m[2] + y * a->m[6] + z * a->m[10];
}
static void m_rot_only(M4 *o, const M4 *a) { *o = *a; o->m[12] = o->m[13] = o->m[14] = 0; o->m[3] = o->m[7] = o->m[11] = 0; o->m[15] = 1; }
static float v_len(const float *v) { return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
static void v_norm(float *v) { float l = v_len(v); if (l < 1e-9f) l = 1; v[0] /= l; v[1] /= l; v[2] /= l; }
static float v_dot(const float *a, const float *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static void v_cross(float *o, const float *a, const float *b)
{ float r[3] = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] }; memcpy(o, r, 12); }
static void v_sub(float *o, const float *a, const float *b) { o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; o[2] = a[2] - b[2]; }
/* rotation by t about unit axis u, row-vector form (v * R turns v about u by +t, right-hand rule) */
static void m_axis(M4 *o, const float *u, float t)
{
    float c = cosf(t), s = sinf(t), k = 1.0f - c, x = u[0], y = u[1], z = u[2];
    m_id(o);
    o->m[0] = c + k * x * x;     o->m[1] = k * x * y + s * z; o->m[2] = k * x * z - s * y;
    o->m[4] = k * x * y - s * z; o->m[5] = c + k * y * y;     o->m[6] = k * y * z + s * x;
    o->m[8] = k * x * z + s * y; o->m[9] = k * y * z - s * x; o->m[10] = c + k * z * z;
}
/* rotation (row-vector form) turning direction a onto direction b, by fraction w of the angle */
static void m_from_to(M4 *o, const float *a0, const float *b0, float w)
{
    float a[3] = { a0[0], a0[1], a0[2] }, b[3] = { b0[0], b0[1], b0[2] }, ax[3];
    v_norm(a); v_norm(b); v_cross(ax, a, b);
    float s = v_len(ax), c = v_dot(a, b);
    if (s < 1e-6f) { m_id(o); return; }
    ax[0] /= s; ax[1] /= s; ax[2] /= s;
    m_axis(o, ax, atan2f(s, c) * w);
}
/* rotation part as axis-angle, scaled by w */
static void m_scale_rot(M4 *o, const M4 *r, float w)
{
    float tr = r->m[0] + r->m[5] + r->m[10], c = (tr - 1.0f) * 0.5f;
    if (c > 1.0f) c = 1.0f;
    if (c < -1.0f) c = -1.0f;
    float t = acosf(c);
    if (t < 1e-5f) { m_id(o); return; }
    /* row-vector matrix = transpose of the column form: axis from the antisymmetric part with flipped sign */
    float ax[3] = { r->m[6] - r->m[9], r->m[8] - r->m[2], r->m[1] - r->m[4] };
    if (v_len(ax) < 1e-6f) { if (w >= 0.999f) { *o = *r; o->m[12] = o->m[13] = o->m[14] = 0; } else m_id(o); return; }
    v_norm(ax); m_axis(o, ax, t * w);
}
/* W := W * (T(-p) R T(p)) : turn a bone (world matrix) about point p */
static void m_turn_about(M4 *W, const M4 *R, const float *p)
{
    M4 t; m_id(&t); t.m[12] = -p[0]; t.m[13] = -p[1]; t.m[14] = -p[2];
    M4 r = *R; r.m[12] = r.m[13] = r.m[14] = 0;
    M4 b; m_id(&b); b.m[12] = p[0]; b.m[13] = p[1]; b.m[14] = p[2];
    M4 a; m_mul(&a, &t, &r); m_mul(&a, &a, &b); m_mul(W, W, &a);
}

typedef struct { u16 b[4]; float w[4]; float q[4][3]; float n[4][3]; int nb; float uv[2]; } Vtx;
typedef struct { int tex; int start, length; int atest, blend; const u16 *ix; } Prim;   /* blend = transparent layer (hair strands, lashes) */
typedef struct { u32 id; int w, h; const u8 *dxt5; IDirect3DTexture8 *tex; IDirect3DDevice8 *dev; } Tex;
typedef struct { float x, y, z; DWORD c; float u, v; } PV;
#define PV_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

/* one hand of a skeleton: bone, grip frame (hand-local), finger chains (thumb first) with their curl axes */
typedef struct {
    int ok, hb;
    M4 G;                    /* grip frame in hand-local coordinates (rows x y z origin) */
    int f[5][3];             /* thumb, index, middle, ring, little: 3 joints each (-1 = none) */
    float ax[5][3][3];       /* curl axis per joint, in that joint bone's own frame */
    float tip[5][3];         /* fingertip, in the last joint's frame */
} Hand;

/* the flashlight in James's breast pocket, moved onto the character's chest */
#define FL_MAXV 64
#define FL_MAXI 128
typedef struct { int nv, ni; float q[FL_MAXV][3], n[FL_MAXV][3], uv[FL_MAXV][2]; u16 idx[FL_MAXI]; int prim_start[4], prim_len[4], nprim; } Torch;

typedef struct {
    int ok, tried;
    u8 *file;
    int nb; u8 parent[128]; M4 B[128];               /* bind (model space) */
    int map_j[128];                                 /* target bone -> James bone, -1 = rigid on parent */
    M4 P[128];                                      /* mapped bones: rotation taking James's rotation to this bone's */
    float t_local[128][3]; M4 L[128];               /* offset from parent in the parent's bind frame; rigid child matrix */
    int order[128];
    int lab[NLAB];                                  /* label -> bone */
    float sc;                                       /* hip height ratio */
    Hand hand[2];                                   /* 0 = right, 1 = left */
    float twist[2];                                 /* degrees the hand lining-up turns about the forearm */
    int nhelp, help_b[8], help_arm[8]; M4 help_L[8];  /* unmapped bones skinning the shoulder caps: half on the upper arm */
    float torch_c[3];                               /* pocket flashlight centre, model space */
    float ccent[2][21][3];                          /* centre of her hand with the fingers in each of James's shapes */
    float belly, belly_z;                           /* how much further out her front is than James's at the waist; her front */
    int nv; Vtx *v; PV *pv; const u16 *idx; u16 *tix; int nprim, nopaque; Prim prim[64];
    int nvo; int *lnear;                            /* opaque vertex count; per transparent vertex, the head vertex whose light it takes */ Tex tex[8]; int ntex;
    Torch torch; PV tpv[FL_MAXV]; int sp2;
    IDirect3DTexture8 *rtex[2][8]; IDirect3DDevice8 *rdev[2][8]; int rgen[2][8];   /* Maria's recoloured textures per James */
} Chr;
static Chr s_chr[CHR_COUNT];
/* James: bind from his model file, his grip frames, the pocket flashlight geometry and its texture */
static M4 s_jB[128]; static int s_jnb, s_jok;
static Hand s_jh[2];
static float s_jcent[2][21][3]; static u8 s_jcent_ok[21];   /* centre of each of James's in-game hand shapes, hand frame */
static u8 *s_jfile; static Tex s_jtorch_tex; static int s_jtorch_ok;
static float s_jtorch_v[FL_MAXV][3], s_jtorch_n[FL_MAXV][3], s_jtorch_uv[FL_MAXV][2]; static int s_jtorch_nv;
static u16 s_jtorch_idx[FL_MAXI]; static int s_jtorch_ni, s_jtorch_pstart[4], s_jtorch_plen[4], s_jtorch_np;
static float s_jtorch_c[3];                          /* its centre, James's model space */
float chr_test_offset[3]; int chr_hair_ref = 0x80;   /* hair layers: alpha from which a texel is solid */ int chr_test_hide = -1;   /* cloud test: hide one part */                           /* cloud test build: draw the character this far from James */
static char s_dir[MAX_PATH];
void chr_set_dir(const char *dir) { strncpy(s_dir, dir, MAX_PATH - 1); }

static u32 rd32(const u8 *p) { return *(const u32 *)p; }
static unsigned rd16(const u8 *p) { return *(const u16 *)p; }
static u8 *read_file(const char *rel, int *n)
{
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s%s", s_dir, rel);
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(path, sizeof path, "%s..\\%s", s_dir, rel); f = fopen(path, "rb"); }
    if (!f) { ce_log("chr: %s not found", rel); return NULL; }
    fseek(f, 0, SEEK_END); *n = (int)ftell(f); fseek(f, 0, SEEK_SET);
    u8 *b = (u8 *)malloc(*n);
    if (!b || fread(b, 1, *n, f) != (size_t)*n) { fclose(f); free(b); return NULL; }
    fclose(f); return b;
}
/* model section header fields we use */
typedef struct { const u8 *m; u32 T, M, nb, imat, skel, nprim, prims, ntexblk, texblk, ntexid, texid, nv, voff, ioff, tioff, ntexf; } Hdr;
static int header(const u8 *d, int n, Hdr *h)
{
    h->T = rd32(d + 12); h->M = rd32(d + 20); h->ntexf = rd32(d + 8);
    if (h->M + 0xA0 > (u32)n || rd32(d + h->M) != 0xFFFF0003u) return 0;
    const u8 *m = d + h->M; h->m = m;
    h->imat = rd32(m + 8); h->nb = rd32(m + 12); h->skel = rd32(m + 16); h->nprim = rd32(m + 32); h->prims = rd32(m + 36);
    h->ntexblk = rd32(m + 48); h->texblk = rd32(m + 52); h->ntexid = rd32(m + 56); h->texid = rd32(m + 60);
    h->nv = rd32(m + 128); h->voff = rd32(m + 132); h->ioff = rd32(m + 144); h->tioff = rd32(m + 148);
    return h->nb > 0 && h->nb <= 128 && h->nprim <= 48;
}
/* DXT5 textures of a model file: containers at T+16 */
static int load_textures(const u8 *d, int n, const Hdr *h, Tex *tex, int max)
{
    int cnt = 0; const u8 *t = d + h->T + 16;
    for (u32 i = 0; i < h->ntexf && cnt < max; i++) {
        if (t + 32 > d + n) break;
        u32 id = rd32(t); int w = rd16(t + 4), hh = rd16(t + 6), ns = rd16(t + 12);
        const u8 *sh = t + 32; u32 dsz = 0; int fmt = 0;
        for (int s = 0; s < ns; s++) { fmt = sh[12]; dsz = rd32(sh + 16); sh += 32; }
        if (sh + dsz > d + n) break;
        if (fmt == 4 && dsz == (u32)(w * hh)) { memset(&tex[cnt], 0, sizeof tex[cnt]); tex[cnt].id = id; tex[cnt].w = w; tex[cnt].h = hh; tex[cnt].dxt5 = sh; cnt++; }
        t = sh + dsz;
    }
    return cnt;
}
static u32 prim_tex_id(const u8 *m, const Hdr *h, const u8 *p)
{
    u32 ntx = rd32(p + 24), txoff = rd32(p + 28);
    if (!ntx) return 0;
    u32 ti = rd16(p + txoff); if (ti >= h->ntexid) return 0;
    u32 blk = rd32(m + h->texid + ti * 8); if (blk >= h->ntexblk) return 0;
    return rd32(m + h->texblk + blk * 4);
}

/* ---- hands: finger chains, grip frame, curl axes (shared by James and the characters) ---- */
static void bone_pos(float *o, const M4 *B, int b) { o[0] = B[b].m[12]; o[1] = B[b].m[13]; o[2] = B[b].m[14]; }
static int make_hand(Hand *H, int nb, const u8 *parent, const M4 *B, int hb, int with_axes)
{
    memset(H, 0, sizeof *H); H->hb = hb;
    for (int i = 0; i < 5; i++) for (int j = 0; j < 3; j++) H->f[i][j] = -1;
    int ch[8][3], nch = 0;
    for (int b = 0; b < nb && nch < 8; b++) {
        if (parent[b] != hb) continue;
        int c = b, k = 0;
        while (c >= 0 && k < 3) {
            ch[nch][k++] = c;
            int kid = -1, nk = 0;
            for (int x = 0; x < nb; x++) if (parent[x] == c) { kid = x; nk++; }
            c = nk == 1 ? kid : -1;
        }
        if (k < 3) continue;                                         /* not a 3-joint finger */
        nch++;
    }
    if (nch != 5) return 0;
    M4 iH; m_rigid_inv(&iH, &B[hb]);
    float m[5][3];
    for (int i = 0; i < 5; i++) { float p[3]; bone_pos(p, B, ch[i][0]); v_mul(m[i], p, &iH); }
    int th = 0; for (int i = 1; i < 5; i++) if (v_len(m[i]) < v_len(m[th])) th = i;
    /* index = the knuckle nearest the thumb, then each next nearest to the previous one */
    int ord[4], used[5] = {0}; used[th] = 1;
    int prev = th;
    for (int k = 0; k < 4; k++) {
        int best = -1; float bd = 1e30f;
        for (int i = 0; i < 5; i++) if (!used[i]) { float d[3]; v_sub(d, m[i], m[prev]); float l = v_len(d); if (l < bd) { bd = l; best = i; } }
        ord[k] = best; used[best] = 1; prev = best;
    }
    memcpy(H->f[0], ch[th], sizeof H->f[0]);
    for (int k = 0; k < 4; k++) memcpy(H->f[k + 1], ch[ord[k]], sizeof H->f[0]);
    float mean[3] = {0, 0, 0};
    for (int k = 0; k < 4; k++) for (int j = 0; j < 3; j++) mean[j] += m[ord[k]][j] * 0.25f;
    float x[3] = { mean[0], mean[1], mean[2] }; v_norm(x);
    float y[3]; v_sub(y, m[ord[0]], m[ord[3]]); float d = v_dot(y, x); y[0] -= d * x[0]; y[1] -= d * x[1]; y[2] -= d * x[2]; v_norm(y);
    float z[3]; v_cross(z, x, y);
    m_id(&H->G);
    memcpy(&H->G.m[0], x, 12); memcpy(&H->G.m[4], y, 12); memcpy(&H->G.m[8], z, 12);
    H->G.m[12] = mean[0] * 0.6f; H->G.m[13] = mean[1] * 0.6f; H->G.m[14] = mean[2] * 0.6f;
    H->ok = 1;
    if (!with_axes) return 1;
    /* palm side: the way the model's own relaxed fingers bend (model space) */
    float zm[3]; n_mul(zm, z, &B[hb]);
    float side = 0;
    for (int k = 1; k < 5; k++) {
        float p0[3], p1[3], p2[3], d1[3], d2[3];
        bone_pos(p0, B, H->f[k][0]); bone_pos(p1, B, H->f[k][1]); bone_pos(p2, B, H->f[k][2]);
        v_sub(d1, p1, p0); v_sub(d2, p2, p1); v_norm(d1); v_norm(d2);
        float t[3]; v_sub(t, d2, d1); side += v_dot(t, zm);
    }
    float palm[3] = { side < 0 ? -zm[0] : zm[0], side < 0 ? -zm[1] : zm[1], side < 0 ? -zm[2] : zm[2] };
    for (int k = 0; k < 5; k++) for (int j = 0; j < 3; j++) {
        int b = H->f[k][j];
        float p0[3], p1[3], dir[3];
        if (j < 2) { bone_pos(p0, B, b); bone_pos(p1, B, H->f[k][j + 1]); }
        else       { bone_pos(p0, B, H->f[k][1]); bone_pos(p1, B, b); }     /* end joint: the last segment's direction */
        v_sub(dir, p1, p0); v_norm(dir);
        float a[3]; v_cross(a, dir, palm); v_norm(a);                    /* turning dir about a moves it toward the palm */
        /* into the joint bone's own frame: a_local = a * rot(B)^T */
        float al[3] = { a[0] * B[b].m[0] + a[1] * B[b].m[1] + a[2] * B[b].m[2],
                        a[0] * B[b].m[4] + a[1] * B[b].m[5] + a[2] * B[b].m[6],
                        a[0] * B[b].m[8] + a[1] * B[b].m[9] + a[2] * B[b].m[10] };
        memcpy(H->ax[k][j], al, 12);
    }
    return 1;
}

static int load_james(void)
{
    if (s_jok) return s_jok > 0;
    s_jok = -1;
    int n; u8 *d = read_file("data\\chr\\jms\\hhh_jms.mdl", &n); Hdr h;
    if (!d || !header(d, n, &h)) { free(d); return 0; }
    s_jnb = (int)h.nb; memcpy(s_jB, h.m + h.imat, s_jnb * 64);
    u8 par[128]; memcpy(par, h.m + h.skel, s_jnb);
    for (int b = 0; b < s_jnb; b++) if (par[b] >= s_jnb) par[b] = 0xFF;
    int okR = make_hand(&s_jh[0], s_jnb, par, s_jB, 35, 0), okL = make_hand(&s_jh[1], s_jnb, par, s_jB, 40, 0);
    /* the pocket flashlight: the primitives with part id 21 (bone 3), vertices kept in James's model space */
    Tex tx[16]; int ntx = load_textures(d, n, &h, tx, 16);
    const u8 *m = h.m, *p = m + h.prims;
    unsigned vmap[FL_MAXV]; int vmn = 0;
    float cmin[3] = { 1e9f, 1e9f, 1e9f }, cmax[3] = { -1e9f, -1e9f, -1e9f };
    for (u32 i = 0; i < h.nprim; i++) {
        u32 sz = rd32(p);
        if (sz < 0x80 || sz > 0x400) break;
        if (rd16(p + 0x26) == 21 && s_jtorch_np < 4) {
            u32 pnb = rd32(p + 8), boff = rd32(p + 12), start = rd32(p + 0x74), len = rd32(p + 0x78);
            u32 tid = prim_tex_id(m, &h, p);
            for (int s = 0; s < ntx; s++) if (tx[s].id == tid) { s_jtorch_tex = tx[s]; s_jtorch_ok = 1; }
            const u16 *ix = (const u16 *)(m + h.ioff);
            s_jtorch_pstart[s_jtorch_np] = s_jtorch_ni;
            for (u32 kk = start; kk < start + len && s_jtorch_ni < FL_MAXI; kk++) {
                unsigned vi = ix[kk]; int slot = -1;
                for (int q = 0; q < vmn; q++) if (vmap[q] == vi) slot = q;     /* file vertex -> our small table */
                if (slot < 0 && vmn < FL_MAXV) {
                    const u8 *v = m + h.voff + vi * 48;
                    const float *pos = (const float *)v, *uv = (const float *)(v + 36); const short *n16 = (const short *)(v + 28);
                    u32 b0 = v[44] < pnb ? rd16(p + boff + v[44] * 2) : 3; if (b0 >= (u32)s_jnb) b0 = 3;
                    float nr[3] = { n16[0] / 32767.0f, n16[1] / 32767.0f, n16[2] / 32767.0f };
                    slot = vmn; vmap[vmn++] = vi;
                    v_mul(s_jtorch_v[slot], pos, &s_jB[b0]); n_mul(s_jtorch_n[slot], nr, &s_jB[b0]);
                    s_jtorch_uv[slot][0] = uv[0]; s_jtorch_uv[slot][1] = uv[1];
                    for (int j = 0; j < 3; j++) { if (s_jtorch_v[slot][j] < cmin[j]) cmin[j] = s_jtorch_v[slot][j]; if (s_jtorch_v[slot][j] > cmax[j]) cmax[j] = s_jtorch_v[slot][j]; }
                    s_jtorch_nv = vmn;
                }
                if (slot >= 0) s_jtorch_idx[s_jtorch_ni++] = (u16)slot;
            }
            s_jtorch_plen[s_jtorch_np] = s_jtorch_ni - s_jtorch_pstart[s_jtorch_np];
            s_jtorch_np++;
        }
        p += sz;
    }
    for (int j = 0; j < 3; j++) s_jtorch_c[j] = (cmin[j] + cmax[j]) * 0.5f;
    s_jfile = d;                                          /* kept: the flashlight texture points into it */
    {   /* James's in-game hand shapes (hll_jms.mdl, part id at prim +0x26, rigid on hand bone 35 / 40): the centre of
           each shape is where whatever he holds sits in his hand - the character's hand centre is matched to it */
        int n2; u8 *d2 = read_file("data\\chr\\jms\\hll_jms.mdl", &n2); Hdr h2;
        if (d2 && header(d2, n2, &h2)) {
            const u8 *m2 = h2.m, *p2 = m2 + h2.prims; const u16 *ix2 = (const u16 *)(m2 + h2.ioff);
            double acc[21][3] = {{0}}; int cnt[21] = {0};
            for (u32 i = 0; i < h2.nprim; i++) {
                u32 sz = rd32(p2); if (sz < 0x80 || sz > 0x400) break;
                int part = (int)rd16(p2 + 0x26);
                if (part >= 3 && part <= 20) {
                    u32 pnb = rd32(p2 + 8), boff = rd32(p2 + 12), st = rd32(p2 + 0x74), ln = rd32(p2 + 0x78), hb = part <= 13 ? 35 : 40;
                    for (u32 kk = st; kk < st + ln; kk++) {
                        const u8 *v = m2 + h2.voff + ix2[kk] * 48;
                        if (v[44] >= pnb || rd16(p2 + boff + v[44] * 2) != hb) continue;
                        const float *q = (const float *)v; acc[part][0] += q[0]; acc[part][1] += q[1]; acc[part][2] += q[2]; cnt[part]++;
                    }
                }
                p2 += sz;
            }
            for (int pt = 3; pt <= 20; pt++) if (cnt[pt] > 20) {
                int sd = pt <= 13 ? 0 : 1;
                for (int j = 0; j < 3; j++) s_jcent[sd][pt][j] = (float)(acc[pt][j] / cnt[pt]);
                s_jcent_ok[pt] = 1;
            }
            ce_log("chr: James's hand shapes: right fist centre %.0f %.0f %.0f, handgun %.0f %.0f %.0f", s_jcent[0][4][0], s_jcent[0][4][1], s_jcent[0][4][2], s_jcent[0][7][0], s_jcent[0][7][1], s_jcent[0][7][2]);
        }
        free(d2);
    }
    s_jok = 1;
    ce_log("chr: James's grip frames %s/%s, pocket flashlight %d vertices %d parts (texture %s)",
           okR ? "ok" : "MISSING", okL ? "ok" : "MISSING", s_jtorch_nv, s_jtorch_np, s_jtorch_ok ? "ok" : "missing");
    return 1;
}
/* bind-space position of a character vertex */
static void vtx_bind(const Chr *c, const Vtx *v, float *o) { v_mul(o, v->q[0], &c->B[v->b[0]]); }
/* the front of a body at (x, y) in model space: the largest z of the torso vertices nearby (all four models face +z) */
static float body_front_l(const Chr *c, float x, float y, float r, float fallback, const int *labs, int nl)
{
    float best = -1e30f;
    for (int i = 0; i < c->nv; i++) {
        const Vtx *v = &c->v[i]; if (!v->nb) continue;
        int b = v->b[0], ok = 0;
        for (int k = 0; k < nl; k++) if (b == c->lab[labs[k]]) ok = 1;
        if (!ok) continue;
        float p[3]; vtx_bind(c, v, p);
        if (fabsf(p[0] - x) < r && fabsf(p[1] - y) < r && p[2] > best) best = p[2];
    }
    return best > -1e29f ? best : fallback;
}
static float body_front(const Chr *c, float x, float y, float r, float fallback)
{
    static const int chest[] = { L_SPINE1, L_SPINE2, L_NECK, L_CLAVL, L_CLAVR };
    return body_front_l(c, x, y, r, fallback, chest, 5);
}
/* James's body front at (x, y) from his model file's vertices on the given bones */
static float james_body_front(float x, float y, float r, const int *bones, int nbn, float fallback)
{
    Hdr h; const u8 *d = s_jfile;
    if (!d || !header(d, 0x7FFFFFFF, &h)) return fallback;
    const u8 *m = h.m, *p = m + h.prims; float best = -1e30f;
    for (u32 i = 0; i < h.nprim; i++) {
        u32 sz = rd32(p); if (sz < 0x80 || sz > 0x400) break;
        if (rd16(p + 0x26) == 0) {
            u32 pnb = rd32(p + 8), boff = rd32(p + 12), start = rd32(p + 0x74), len = rd32(p + 0x78);
            const u16 *ix = (const u16 *)(m + h.ioff);
            for (u32 kk = start; kk < start + len; kk++) {
                const u8 *v = m + h.voff + ix[kk] * 48;
                u32 b0 = v[44] < pnb ? rd16(p + boff + v[44] * 2) : 0; int ok = 0;
                for (int k = 0; k < nbn; k++) if ((int)b0 == bones[k]) ok = 1;
                if (!ok) continue;
                float q[3]; v_mul(q, (const float *)v, &s_jB[b0]);
                if (fabsf(q[0] - x) < r && fabsf(q[1] - y) < r && q[2] > best) best = q[2];
            }
        }
        p += sz;
    }
    return best > -1e29f ? best : fallback;
}
static float s_jfront_cache = -1e30f;
static float james_front(float x, float y)
{
    /* James's own chest surface where the flashlight sits: from his model's body vertices (bones 2, 3, 8) */
    if (s_jfront_cache > -1e29f) return s_jfront_cache;
    int n; u8 *d = s_jfile; Hdr h;
    s_jfront_cache = s_jtorch_c[2] - 9.0f;
    if (!d) return s_jfront_cache;
    n = 0x7FFFFFFF; if (!header(d, n, &h)) return s_jfront_cache;
    const u8 *m = h.m, *p = m + h.prims; float best = -1e30f;
    for (u32 i = 0; i < h.nprim; i++) {
        u32 sz = rd32(p); if (sz < 0x80 || sz > 0x400) break;
        if (rd16(p + 0x26) != 21) {
            u32 pnb = rd32(p + 8), boff = rd32(p + 12), start = rd32(p + 0x74), len = rd32(p + 0x78);
            const u16 *ix = (const u16 *)(m + h.ioff);
            for (u32 kk = start; kk < start + len; kk++) {
                const u8 *v = m + h.voff + ix[kk] * 48;
                u32 b0 = v[44] < pnb ? rd16(p + boff + v[44] * 2) : 0;
                if (b0 != 2 && b0 != 3 && b0 != 8) continue;
                float q[3]; v_mul(q, (const float *)v, &s_jB[b0]);
                if (fabsf(q[0] - x) < 25.0f && fabsf(q[1] - y) < 25.0f && q[2] > best) best = q[2];
            }
        }
        p += sz;
    }
    if (best > -1e29f) s_jfront_cache = best;
    return s_jfront_cache;
}

static int load_chr(int k)
{
    Chr *c = &s_chr[k];
    if (c->tried) return c->ok;
    c->tried = 1;
    if (!load_james()) return 0;
    int n; u8 *d = read_file(kChars[k].file, &n); Hdr h;
    if (!d || !header(d, n, &h)) { ce_log("chr: %s is not a PC model file", kChars[k].file); free(d); return 0; }
    c->file = d; const u8 *m = h.m;
    c->nb = (int)h.nb; memcpy(c->B, m + h.imat, c->nb * 64); memcpy(c->parent, m + h.skel, c->nb);
    for (int b = 0; b < c->nb; b++) { c->map_j[b] = -1; if (c->parent[b] >= c->nb) c->parent[b] = 0xFF; }
    for (int i = 0; i < NLAB; i++) { c->map_j[kChars[k].map[i]] = MAP_JMS[i]; c->lab[i] = kChars[k].map[i]; }
    /* topological order, local offsets, rigid child matrices, retarget rotations */
    { int done[128] = {0}, cnt = 0, guard = 0;
      while (cnt < c->nb && guard++ < 256) for (int b = 0; b < c->nb; b++) if (!done[b] && (c->parent[b] == 0xFF || done[c->parent[b]])) { done[b] = 1; c->order[cnt++] = b; } }
    for (int b = 0; b < c->nb; b++) {
        int p = c->parent[b];
        if (c->map_j[b] >= 0) { M4 rb, jb, ij; m_rot_only(&rb, &c->B[b]); m_rot_only(&jb, &s_jB[c->map_j[b]]); m_rigid_inv(&ij, &jb); m_mul(&c->P[b], &rb, &ij); }
        if (p == 0xFF) continue;
        M4 ip; m_rigid_inv(&ip, &c->B[p]);
        float d3[3] = { c->B[b].m[12] - c->B[p].m[12], c->B[b].m[13] - c->B[p].m[13], c->B[b].m[14] - c->B[p].m[14] };
        n_mul(c->t_local[b], d3, &ip);
        m_mul(&c->L[b], &c->B[b], &ip);
    }
    c->sc = s_jB[1].m[13] != 0 ? c->B[1].m[13] / s_jB[1].m[13] : 1.0f;
    /* limbs and spine: lined up by the direction of each segment rather than by the whole bind rotation, so a model
       whose bind pose holds the forearms forward or the head tilted still takes James's exact limb directions (only
       the twist about the segment keeps the bind difference) */
    { /* segment, the point it aims at, and the segment above it (whose turn it inherits first, so neighbouring
         segments keep their twist relative to each other and hard-skinned seams such as Angela's elbows stay closed) */
      static const int seg[][3] = { {L_SPINE1, L_SPINE2, -1}, {L_SPINE2, L_NECK, L_SPINE1}, {L_NECK, L_HEAD, L_SPINE2},
          {L_CLAVL, L_ARML, L_SPINE2}, {L_CLAVR, L_ARMR, L_SPINE2}, {L_ARML, L_ELBOWL, L_CLAVL}, {L_ARMR, L_ELBOWR, L_CLAVR},
          {L_ELBOWL, L_HANDL, L_ARML}, {L_ELBOWR, L_HANDR, L_ARMR}, {L_FOREL, L_HANDL, L_ELBOWL}, {L_FORER, L_HANDR, L_ELBOWR},
          {16, 18, -1}, {17, 19, -1}, {18, 22, 16}, {19, 23, 17}, {20, 22, 18}, {21, 23, 19} };   /* thighs, knees, shins */
      M4 fixL[NLAB]; for (int i = 0; i < NLAB; i++) m_id(&fixL[i]);
      for (unsigned i = 0; i < sizeof seg / sizeof seg[0]; i++) {
          int lb = seg[i][0], b = c->lab[lb], cb = c->lab[seg[i][1]], jb = MAP_JMS[lb], jc = MAP_JMS[seg[i][1]];
          float dc[3], dj[3], p0[3], p1[3], dr[3];
          bone_pos(p0, c->B, b); bone_pos(p1, c->B, cb); v_sub(dc, p1, p0);
          bone_pos(p0, s_jB, jb); bone_pos(p1, s_jB, jc); v_sub(dj, p1, p0);
          if (v_len(dc) < 1 || v_len(dj) < 1) continue;
          const M4 *up = seg[i][2] >= 0 ? &fixL[seg[i][2]] : NULL;
          M4 base; if (up) base = *up; else m_id(&base);
          n_mul(dr, dc, &base);
          M4 arc, fix, rb, jr, ij, t; m_from_to(&arc, dr, dj, 1.0f); m_mul(&fix, &base, &arc); fixL[lb] = fix;
          m_rot_only(&rb, &c->B[b]); m_rot_only(&jr, &s_jB[jb]); m_rigid_inv(&ij, &jr);
          m_mul(&t, &rb, &fix); m_mul(&c->P[b], &t, &ij);
      } }
    /* hands: grip frames, and the hand bones lined up with James's by them instead of by the bind poses */
    /* (fingertips: the last segment once more, 80 %, in the last joint's frame - used to keep fingers out of weapons) */
    for (int s = 0; s < 2; s++) {
        int hb = c->lab[s == 0 ? L_HANDR : L_HANDL];
        if (make_hand(&c->hand[s], c->nb, c->parent, c->B, hb, 1) && s_jh[s].ok) {
            /* rot(Gc) * X = rot(Gj) * rot(Wj)  ->  X = rot(Gc)^-1 * rot(Gj) * rot(Wj) */
            M4 gc, igc, gj; m_rot_only(&gc, &c->hand[s].G); m_rigid_inv(&igc, &gc); m_rot_only(&gj, &s_jh[s].G);
            m_mul(&c->P[hb], &igc, &gj);
            /* the turn of the hand about the forearm that this lining-up adds is shared with the forearm's twist bone
               (half each), so sleeves and cuffs don't wring at the wrist */
            int fb = c->lab[s == 0 ? L_FORER : L_FOREL], jh = MAP_JMS[s == 0 ? L_HANDR : L_HANDL];
            M4 rbh, ibh, rjh, Cm, t;
            m_rot_only(&rbh, &c->B[hb]); m_rigid_inv(&ibh, &rbh); m_rot_only(&rjh, &s_jB[jh]);
            m_mul(&t, &c->P[hb], &rjh); m_mul(&Cm, &ibh, &t);       /* hand-local correction: C * rot(B_hand) = aligned bind */
            float um[3], p0[3], p1[3]; bone_pos(p0, c->B, fb); bone_pos(p1, c->B, hb); v_sub(um, p1, p0); v_norm(um);
            float uh[3] = { v_dot(um, &rbh.m[0]), v_dot(um, &rbh.m[4]), v_dot(um, &rbh.m[8]) };   /* forearm axis, hand-local */
            float pp[3] = { 1, 0, 0 }; if (fabsf(uh[0]) > 0.8f) { pp[0] = 0; pp[1] = 1; }
            float d = v_dot(pp, uh); pp[0] -= d * uh[0]; pp[1] -= d * uh[1]; pp[2] -= d * uh[2]; v_norm(pp);
            float q[3]; n_mul(q, pp, &Cm); d = v_dot(q, uh); q[0] -= d * uh[0]; q[1] -= d * uh[1]; q[2] -= d * uh[2]; v_norm(q);
            float cr[3]; v_cross(cr, pp, q);
            float tw = atan2f(v_dot(cr, uh), v_dot(pp, q));          /* twist of the correction about the forearm */
            M4 rbf; m_rot_only(&rbf, &c->B[fb]);
            float uf[3] = { v_dot(um, &rbf.m[0]), v_dot(um, &rbf.m[4]), v_dot(um, &rbf.m[8]) };
            M4 Q, Pf; m_axis(&Q, uf, tw * 0.5f); m_mul(&Pf, &Q, &c->P[fb]); c->P[fb] = Pf;
            c->twist[s] = tw * 57.29578f;
        } else c->hand[s].ok = 0;
        if (c->hand[s].ok) for (int f = 0; f < 5; f++) {
            int lb = c->hand[s].f[f][2], mb = c->hand[s].f[f][1];
            float d3[3] = { (c->B[lb].m[12] - c->B[mb].m[12]) * 0.8f, (c->B[lb].m[13] - c->B[mb].m[13]) * 0.8f, (c->B[lb].m[14] - c->B[mb].m[14]) * 0.8f };
            M4 r, ir; m_rot_only(&r, &c->B[lb]); m_rigid_inv(&ir, &r); n_mul(c->hand[s].tip[f], d3, &ir);
        }
    }
    /* textures and vertices: influences as bind-space positions in each influencing bone's local frame */
    c->nhelp = 0;
    c->ntex = load_textures(d, n, &h, c->tex, 8);
    c->nv = (int)h.nv; c->v = (Vtx *)calloc(c->nv, sizeof(Vtx)); c->pv = (PV *)calloc(c->nv, sizeof(PV));
    c->idx = (const u16 *)(m + h.ioff);
    int nidx = (int)((h.tioff > h.ioff ? h.tioff - h.ioff : h.T - h.M - h.ioff) / 2);
    u8 *seen = (u8 *)calloc(c->nv, 1);
    const u8 *p = m + h.prims;
    u32 npairs = rd32(m + 20), pairs = rd32(m + 24);
    for (u32 i = 0; i < h.nprim; i++) {
        u32 sz = rd32(p), pnb = rd32(p + 8), boff = rd32(p + 12), ppn = rd32(p + 16), poff = rd32(p + 20);
        u32 start = rd32(p + 0x74), length = rd32(p + 0x78);
        if (sz < 0x80 || sz > 0x400 || (int)(start + length) > nidx) break;
        int slot = -1; u32 tid = prim_tex_id(m, &h, p);
        for (int s = 0; s < c->ntex; s++) if (c->tex[s].id == tid) slot = s;
        for (u32 kk = start; kk < start + length; kk++) {
            unsigned vi = c->idx[kk]; if (vi >= (unsigned)c->nv || seen[vi]) continue;
            seen[vi] = 1;
            const u8 *v = m + h.voff + vi * 48;
            const float *pos = (const float *)v, *wt = (const float *)(v + 12), *uv = (const float *)(v + 36);
            const short *n16 = (const short *)(v + 28);
            Vtx *o = &c->v[vi]; o->uv[0] = uv[0]; o->uv[1] = uv[1];
            unsigned b0i = v[44]; u32 b0 = b0i < pnb ? rd16(p + boff + b0i * 2) : 0; if (b0 >= (u32)c->nb) b0 = 0;
            float pb[3], nr[3] = { n16[0] / 32767.0f, n16[1] / 32767.0f, n16[2] / 32767.0f }, nb[3];
            v_mul(pb, pos, &c->B[b0]); n_mul(nb, nr, &c->B[b0]);
            float tw = 0;
            for (int s = 0; s < 4; s++) {
                if (wt[s] <= 0.0f) continue;
                unsigned bi = v[44 + s]; u32 b = b0;
                if (bi < pnb) b = rd16(p + boff + bi * 2);
                else if (bi - pnb < ppn) {
                    /* past the bone list: a bone PAIR (parent, child) from the model's pair table - its matrix takes a
                       point from the parent's frame to the child's (default_pcms), i.e. the vertex follows the other
                       bone of the pair; this is how the joints (elbows, knees, knuckles) are blended */
                    unsigned pi = rd16(p + poff + (bi - pnb) * 2);
                    if (pi < npairs) { u32 pa = m[pairs + pi * 2], ch = m[pairs + pi * 2 + 1]; b = pa == b0 ? ch : ch == b0 ? pa : ch; }
                }
                if (b >= (u32)c->nb) b = b0;
                M4 inv; m_rigid_inv(&inv, &c->B[b]);
                int q = o->nb++;
                o->b[q] = (u16)b; o->w[q] = wt[s]; v_mul(o->q[q], pb, &inv); n_mul(o->n[q], nb, &inv); tw += wt[s];
            }
            if (!o->nb) { o->nb = 1; o->b[0] = (u16)b0; o->w[0] = 1; M4 inv; m_rigid_inv(&inv, &c->B[b0]); v_mul(o->q[0], pb, &inv); n_mul(o->n[0], nb, &inv); tw = 1; }
            for (int s = 0; s < o->nb; s++) o->w[s] /= tw;
        }
        /* material (u16 at +0x24): 2 = plain, alpha cuts holes (hair cards, the foot shadow cards); 4 = specular, the
           alpha channel is the shine map and must not be used as transparency (Maria's skirt and top) */
        if (c->nprim < 48) { Prim *pr = &c->prim[c->nprim++]; pr->tex = slot; pr->start = (int)start; pr->length = (int)length; pr->atest = (rd16(p + 0x24) & 4) == 0; pr->blend = 0; pr->ix = c->idx; }
        p += sz;
    }
    free(seen);
    c->nopaque = c->nprim;
    if (k == CHR_ANGELA) {   /* (Angela only - Maria's and Eddie's hair is fine as it is) head pieces whose normals point the other way from the rest of the head (Angela's back-of-head hair piece,
           part 11: half its normals were reversed, so half of it was lit as if it faced the light - the light reddish
           block). The model's own convention is measured over all head vertices; vertices that disagree are flipped. */
        u8 head[128] = {0}; head[c->lab[L_HEAD]] = 1;
        for (int i2 = 0; i2 < c->nb; i2++) { int b = c->order[i2]; if (c->parent[b] != 0xFF && head[c->parent[b]] && c->map_j[b] < 0) head[b] = 1; }
        float hc[3]; bone_pos(hc, c->B, c->lab[L_HEAD]); hc[1] -= 40.0f;
        double sum = 0; int flipped = 0;
        for (int pass = 0; pass < 2; pass++)
            for (int i = 0; i < c->nv; i++) {
                Vtx *v = &c->v[i]; if (!v->nb || !head[v->b[0]]) continue;
                float pt[3], n3[3], o3[3]; vtx_bind(c, v, pt); n_mul(n3, v->n[0], &c->B[v->b[0]]); v_sub(o3, pt, hc); v_norm(o3); v_norm(n3);
                float dd = v_dot(n3, o3);
                if (pass == 0) { sum += dd; continue; }
                if ((sum >= 0 && dd < -0.2f) || (sum < 0 && dd > 0.2f)) {
                    for (int q = 0; q < v->nb; q++) { v->n[q][0] = -v->n[q][0]; v->n[q][1] = -v->n[q][1]; v->n[q][2] = -v->n[q][2]; }
                    flipped++;
                }
            }
        ce_log("chr: %s head normals: %d of the head's vertices turned to match the rest", kChars[k].name, flipped);
    }
    /* transparent layers (header +40 count / +44 offset): Angela's fringe and the back of her hair, Maria's and Eddie's
       hair strands, lashes. Primitive header 0x90 bytes: texture list count/offset +8/+0xC, strip start/length +0x64 /
       +0x68 into their own index list (+148); vertices (+136 count / +140 offset) are 72 bytes: position (local to the
       first bone) +0, weights 3f +16, normal 3f +32, uv +48, then 4 s16 at +64 = a bone, then bone-PAIR indices
       (-1 = none) whose other bone carries the next weights. */
    {
        u32 ntp = rd32(m + 40), tpo = rd32(m + 44), ntv = rd32(m + 136), tvo = rd32(m + 140);
        if (ntp && ntv && ntv < 4096 && tpo && tvo && h.tioff) {
            int base = c->nv, maxi = 0;
            const u8 *tp = m + tpo;
            for (u32 i = 0; i < ntp; i++) { u32 st = rd32(tp + 0x64), ln = rd32(tp + 0x68); if ((int)(st + ln) > maxi) maxi = (int)(st + ln); tp += rd32(tp); }
            c->v = (Vtx *)realloc(c->v, (base + ntv) * sizeof(Vtx)); memset(c->v + base, 0, ntv * sizeof(Vtx));
            c->pv = (PV *)realloc(c->pv, (base + ntv) * sizeof(PV));
            c->tix = (u16 *)malloc(maxi * 2 + 2);
            const u16 *src = (const u16 *)(m + h.tioff);
            for (int i = 0; i < maxi; i++) c->tix[i] = (u16)(src[i] < ntv ? src[i] + base : base);
            for (u32 i = 0; i < ntv; i++) {
                const u8 *v = m + tvo + i * 72;
                const float *pos = (const float *)v, *wt = (const float *)(v + 16), *nr = (const float *)(v + 32), *uv = (const float *)(v + 48);
                const short *bi = (const short *)(v + 64);
                Vtx *o = &c->v[base + i]; o->uv[0] = uv[0]; o->uv[1] = uv[1];
                int b0 = bi[0] >= 0 && bi[0] < c->nb ? bi[0] : 0;
                float pb[3], nb[3]; v_mul(pb, pos, &c->B[b0]); n_mul(nb, nr, &c->B[b0]);
                {   /* hair strand cards: their own normals are not usable for lighting (cards point every way, so single
                       strands came out lighter than the hair under them - Angela's patch, Maria's light tips). They take the
                       normal of the hair shell under them: the direction from the middle of the head (a point a little
                       above the head bone, where the shell is centred), like the opaque hair around it */
                    float hc[3], dv[3]; bone_pos(hc, c->B, c->lab[L_HEAD]); hc[1] -= 40.0f;
                    v_sub(dv, pb, hc); v_norm(dv); memcpy(nb, dv, 12);
                }
                float tw = 0; int prev = b0;
                for (int s2 = 0; s2 < 3; s2++) {
                    if (wt[s2] <= 0.0f) continue;
                    int b = b0;
                    if (s2 > 0) {
                        int pi = bi[s2];
                        if (pi < 0 || (u32)pi >= npairs) continue;
                        int pa = m[pairs + pi * 2], ch = m[pairs + pi * 2 + 1];
                        b = pa == prev ? ch : ch == prev ? pa : ch;
                        if (b >= c->nb) b = b0;
                        prev = b;
                    }
                    M4 inv; m_rigid_inv(&inv, &c->B[b]);
                    int q = o->nb++;
                    o->b[q] = (u16)b; o->w[q] = wt[s2]; v_mul(o->q[q], pb, &inv); n_mul(o->n[q], nb, &inv); tw += wt[s2];
                }
                if (!o->nb) { o->nb = 1; o->b[0] = (u16)b0; o->w[0] = 1; M4 inv; m_rigid_inv(&inv, &c->B[b0]); v_mul(o->q[0], pb, &inv); n_mul(o->n[0], nb, &inv); tw = 1; }
                for (int s2 = 0; s2 < o->nb; s2++) o->w[s2] /= tw;
            }
            c->nv = base + (int)ntv; c->nvo = base;
            /* strand cards take the light of the nearest vertex of the head (hair shell, scalp), so they always match the
               hair under them (their own normals point every way: Angela's light patch, Maria's light tips) */
            {   u8 head[128] = {0}; head[c->lab[L_HEAD]] = 1;
                for (int i2 = 0; i2 < c->nb; i2++) { int b = c->order[i2]; if (c->parent[b] != 0xFF && head[c->parent[b]] && c->map_j[b] < 0) head[b] = 1; }
                c->lnear = (int *)malloc(ntv * sizeof(int));
                for (u32 i2 = 0; i2 < ntv; i2++) {
                    float pt[3]; vtx_bind(c, &c->v[base + i2], pt); float best = 1e30f; int bi = -1;
                    for (int j = 0; j < base; j++) {
                        if (!c->v[j].nb || !head[c->v[j].b[0]]) continue;
                        float q[3], dd[3]; vtx_bind(c, &c->v[j], q); v_sub(dd, q, pt); float l2 = v_dot(dd, dd);
                        if (l2 < best) { best = l2; bi = j; }
                    }
                    c->lnear[i2] = bi;
                }
            }
            tp = m + tpo;
            for (u32 i = 0; i < ntp && c->nprim < 64; i++) {
                u32 ntx = rd32(tp + 8), txoff = rd32(tp + 12);
                int slot = -1;
                if (ntx) { u32 ti = rd16(tp + txoff); if (ti < h.ntexid) { u32 blk = rd32(m + h.texid + ti * 8); if (blk < h.ntexblk) { u32 tid = rd32(m + h.texblk + blk * 4); for (int s2 = 0; s2 < c->ntex; s2++) if (c->tex[s2].id == tid) slot = s2; } } }
                Prim *pr = &c->prim[c->nprim++];
                pr->tex = slot; pr->start = (int)rd32(tp + 0x64); pr->length = (int)rd32(tp + 0x68); pr->atest = 1; pr->blend = 1; pr->ix = c->tix;
                tp += rd32(tp);
            }
            ce_log("chr: %s transparent layers: %u parts, %u vertices", kChars[k].name, ntp, ntv);
        }
    }
    /* her hand centre for each of James's hand shapes: fingers bent by the shape's curl, bind hand frame */
    for (int sd = 0; sd < 2; sd++) {
        Hand *H = &c->hand[sd]; if (!H->ok) continue;
        u8 inhand[128] = {0}; inhand[H->hb] = 1;
        for (int f = 0; f < 5; f++) for (int j = 0; j < 3; j++) if (H->f[f][j] >= 0) inhand[H->f[f][j]] = 1;
        for (int pt = 3; pt <= 20; pt++) {
            if ((pt <= 13) != (sd == 0)) continue;
            static M4 Wl[128];
            m_id(&Wl[H->hb]);                                          /* hand frame */
            for (int f = 0; f < 5; f++) for (int j = 0; j < 3; j++) {
                int b = H->f[f][j]; if (b < 0) continue;
                float ang = kCurl[pt][f] * (f == 0 ? kThumbMax[j] : kFingerMax[j]);
                M4 Q, t; m_axis(&Q, H->ax[f][j], ang); m_mul(&t, &Q, &c->L[b]); m_mul(&Wl[b], &t, &Wl[c->parent[b]]);
            }
            double acc[3] = {0}; int cnt = 0;
            for (int i = 0; i < c->nv; i++) {
                Vtx *v = &c->v[i]; if (!v->nb || !inhand[v->b[0]]) continue;
                float p3[3] = {0, 0, 0};
                for (int q = 0; q < v->nb; q++) {
                    float t3[3]; M4 W; if (inhand[v->b[q]]) W = Wl[v->b[q]]; else { M4 ih; m_rigid_inv(&ih, &c->B[H->hb]); m_mul(&W, &c->B[v->b[q]], &ih); }
                    v_mul(t3, v->q[q], &W); p3[0] += t3[0] * v->w[q]; p3[1] += t3[1] * v->w[q]; p3[2] += t3[2] * v->w[q];
                }
                acc[0] += p3[0]; acc[1] += p3[1]; acc[2] += p3[2]; cnt++;
            }
            if (cnt) for (int j = 0; j < 3; j++) c->ccent[sd][pt][j] = (float)(acc[j] / cnt);
        }
    }
    ce_log("chr: %s hand centre: fist %.0f %.0f %.0f, handgun %.0f %.0f %.0f", kChars[k].name, c->ccent[0][4][0], c->ccent[0][4][1], c->ccent[0][4][2], c->ccent[0][7][0], c->ccent[0][7][1], c->ccent[0][7][2]);
    /* shoulder-cap helpers (Angela's 11 / 14): unmapped bones on the spine or a clavicle whose vertices sit around an
       arm joint; left rigid on the spine they would stay behind when the arm lifts (spikes at the armpit) */
    for (int b = 0; b < c->nb && c->nhelp < 8; b++) {
        int p = c->parent[b];
        if (c->map_j[b] >= 0 || p == 0xFF) continue;
        if (p != c->lab[L_SPINE2] && p != c->lab[L_SPINE1] && p != c->lab[L_CLAVL] && p != c->lab[L_CLAVR]) continue;
        float cen[3] = {0, 0, 0}; int n = 0;
        for (int i = 0; i < c->nv; i++) for (int q = 0; q < c->v[i].nb; q++)
            if (c->v[i].b[q] == b && c->v[i].w[q] > 0.3f) { float pb[3]; v_mul(pb, c->v[i].q[q], &c->B[b]); cen[0] += pb[0]; cen[1] += pb[1]; cen[2] += pb[2]; n++; }
        if (n < 10) continue;
        cen[0] /= n; cen[1] /= n; cen[2] /= n;
        for (int s2 = 0; s2 < 2; s2++) {
            int arm = c->lab[s2 ? L_ARMR : L_ARML]; float ap[3], d[3]; bone_pos(ap, c->B, arm); v_sub(d, cen, ap);
            if (v_len(d) < 60.0f) { M4 ia; m_rigid_inv(&ia, &c->B[arm]); m_mul(&c->help_L[c->nhelp], &c->B[b], &ia); c->help_b[c->nhelp] = b; c->help_arm[c->nhelp] = arm; c->nhelp++; break; }
        }
    }
    /* the pocket flashlight on the chest: James's flashlight shape, put at the same spot on her chest (height between
       spine and neck, width by shoulder width, depth over the body's front) and carried by her upper spine */
    c->sp2 = c->lab[L_SPINE2];
    if (s_jtorch_nv) {
        const float *jc = s_jtorch_c;
        float ty = (jc[1] - s_jB[3].m[13]) / (s_jB[8].m[13] - s_jB[3].m[13]);
        float cy = c->B[c->sp2].m[13] + ty * (c->B[c->lab[L_NECK]].m[13] - c->B[c->sp2].m[13]);
        float jw = fabsf(s_jB[27].m[12]) > 1 ? fabsf(s_jB[27].m[12]) : 100.0f;
        float cx = jc[0] * fabsf(c->B[c->lab[L_ARML]].m[12]) / jw;
        float depth = jc[2] - james_front(jc[0], jc[1]);
        float cz = body_front(c, cx, cy, 22.0f, c->B[c->sp2].m[14] + 80.0f) + depth;
        float cc[3] = { cx, cy, cz }; memcpy(c->torch_c, cc, 12);
        M4 isp; m_rigid_inv(&isp, &c->B[c->sp2]);
        Torch *t = &c->torch; t->nv = s_jtorch_nv; t->ni = s_jtorch_ni; t->nprim = s_jtorch_np;
        for (int i = 0; i < t->nv; i++) {
            float q[3] = { s_jtorch_v[i][0] - jc[0] + cc[0], s_jtorch_v[i][1] - jc[1] + cc[1], s_jtorch_v[i][2] - jc[2] + cc[2] };
            v_mul(t->q[i], q, &isp); n_mul(t->n[i], s_jtorch_n[i], &isp);
            t->uv[i][0] = s_jtorch_uv[i][0]; t->uv[i][1] = s_jtorch_uv[i][1];
        }
        memcpy(t->idx, s_jtorch_idx, sizeof t->idx);
        memcpy(t->prim_start, s_jtorch_pstart, sizeof t->prim_start); memcpy(t->prim_len, s_jtorch_plen, sizeof t->prim_len);
        ce_log("chr: %s's pocket flashlight at %.0f %.0f %.0f (James's at %.0f %.0f %.0f, %.0f in front of his chest)",
               kChars[k].name, cx, cy, cz, jc[0], jc[1], jc[2], depth);
    }
    /* waist fronts: hers and James's at the height of the lower spine */
    { static const int waist[] = { L_PELVIS, L_HIPS, L_SPINE1, L_SPINE2 }; static const int jw[] = { 0, 1, 2, 3 };
      float yc = c->B[c->lab[L_SPINE1]].m[13], yj = s_jB[2].m[13];
      c->belly_z = body_front_l(c, 0, yc, 40.0f, c->B[c->lab[L_SPINE1]].m[14] + 60.0f, waist, 4);
      float jz = james_body_front(0, yj, 40.0f, jw, 4, s_jB[2].m[14] + 60.0f);
      c->belly = c->belly_z - jz; }
    c->ok = c->nprim > 0 && c->ntex > 0;
    ce_log("chr: %s waist front %.0f (James %.0f further back by %.0f)", kChars[k].name, c->belly_z, c->belly_z - c->belly, c->belly);
    ce_log("chr: %s loaded from %s - %d bones, %d parts, %d vertices, %d textures, hip scale %.3f, hands %s/%s (wrist turn %.0f / %.0f degrees, half on the forearm)",
           kChars[k].name, kChars[k].file, c->nb, c->nprim, c->nv, c->ntex, c->sc,
           c->hand[0].ok ? "ok" : "rigid", c->hand[1].ok ? "ok" : "rigid", c->twist[0], c->twist[1]);
    return c->ok;
}
int chr_ready(int k) { return k > 0 && k < CHR_COUNT && load_chr(k); }

/* ---- Maria's colours (Appearance page, James 1 / James 2 each): top, skirt, boots, hair ----
   Same method as James's: every texel of the part keeps its brightness and takes the colour (out = L*m + b,
   ce_apply), so folds, the leopard print and the hair strands stay. The parts by texture area (measured on her
   textures): 0x4DE right half = the cardigan / top, its left half above 60 % = hair, below it on the left = boots;
   0x4DD top 44 % = the skirt (the gold belt below is left alone); 0x4DF = hair cards (all of it). */
int chr_mcolor[2][CHR_MPARTS]; static int s_mgen[2] = { 1, 1 };
const char *const chr_mpart_name[CHR_MPARTS] = { "Top", "Skirt", "Boots", "Hair" };
static const char *const kMKey[2][CHR_MPARTS] = { { "james1_maria_top", "james1_maria_skirt", "james1_maria_boots", "james1_maria_hair" },
                                                  { "james2_maria_top", "james2_maria_skirt", "james2_maria_boots", "james2_maria_hair" } };
static char s_mini[MAX_PATH];
void chr_mcolor_load(const char *ini)
{
    strncpy(s_mini, ini, MAX_PATH - 1);
    for (int w = 0; w < 2; w++) for (int p = 0; p < CHR_MPARTS; p++) {
        char v[64]; chr_mcolor[w][p] = 0;
        if (GetPrivateProfileStringA("look", kMKey[w][p], "", v, sizeof v, s_mini) == 0) { WritePrivateProfileStringA("look", kMKey[w][p], "Original", s_mini); continue; }
        for (int i = 0; i < ce_palette_n; i++) if (!_stricmp(v, ce_palette[i].name)) chr_mcolor[w][p] = i;
    }
    s_mgen[0]++; s_mgen[1]++;
}
void chr_mcolor_cycle(int who, int part, int dir)
{
    if (who < 0 || who > 1 || part < 0 || part >= CHR_MPARTS) return;
    chr_mcolor[who][part] = (chr_mcolor[who][part] + dir + ce_palette_n) % ce_palette_n;
    s_mgen[who]++;
    if (s_mini[0]) WritePrivateProfileStringA("look", kMKey[who][part], ce_palette[chr_mcolor[who][part]].name, s_mini);
    ce_log("characters: James %d's Maria %s = %s", who + 1, chr_mpart_name[part], ce_palette[chr_mcolor[who][part]].name);
}
static int maria_part(u32 id, float fx, float fy)
{
    switch (id) {
    case 0x4DE: if (fx >= 0.5f) return 0; if (fy < 0.6f) return 3; if (fx < 0.36f) return 2; if (fy < 0.76f) return 3; return -1;
    case 0x4DD: return fy < 0.44f ? 1 : -1;
    case 0x4DF: return 3;
    }
    return -1;
}
static void c565(unsigned c, int *o) { o[0] = ((c >> 11) & 31) * 255 / 31; o[1] = ((c >> 5) & 63) * 255 / 63; o[2] = (c & 31) * 255 / 31; }
/* DXT5 -> 32-bit with the colour changes; NULL when nothing of this texture changes */
static IDirect3DTexture8 *maria_tex(IDirect3DDevice8 *dev, const Tex *t, int who)
{
    int any = 0;
    for (int p = 0; p < CHR_MPARTS; p++) if (chr_mcolor[who][p]) any = 1;
    if (!any || maria_part(t->id, 0.75f, 0.1f) < 0 && maria_part(t->id, 0.1f, 0.1f) < 0 && maria_part(t->id, 0.1f, 0.9f) < 0) return NULL;
    IDirect3DTexture8 *x = NULL;
    if (dev->lpVtbl->CreateTexture(dev, t->w, t->h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &x) != D3D_OK || !x) return NULL;
    D3DLOCKED_RECT lr;
    if (x->lpVtbl->LockRect(x, 0, &lr, NULL, 0) != D3D_OK) { x->lpVtbl->Release(x); return NULL; }
    int bw = (t->w + 3) / 4, bh = (t->h + 3) / 4;
    for (int by = 0; by < bh; by++) for (int bx = 0; bx < bw; bx++) {
        const u8 *q = t->dxt5 + (by * bw + bx) * 16;
        int a0 = q[0], a1 = q[1], ap[8]; ap[0] = a0; ap[1] = a1;
        if (a0 > a1) for (int i = 1; i < 7; i++) ap[i + 1] = (a0 * (7 - i) + a1 * i) / 7;
        else { for (int i = 1; i < 5; i++) ap[i + 1] = (a0 * (5 - i) + a1 * i) / 5; ap[6] = 0; ap[7] = 255; }
        unsigned long long abits = 0; for (int i = 0; i < 6; i++) abits |= (unsigned long long)q[2 + i] << (8 * i);
        int c0[3], c1[3], cp[4][3]; unsigned u0 = q[8] | q[9] << 8, u1 = q[10] | q[11] << 8;
        c565(u0, c0); c565(u1, c1);
        for (int j = 0; j < 3; j++) { cp[0][j] = c0[j]; cp[1][j] = c1[j]; cp[2][j] = (2 * c0[j] + c1[j]) / 3; cp[3][j] = (c0[j] + 2 * c1[j]) / 3; }
        unsigned cbits = q[12] | q[13] << 8 | q[14] << 16 | (unsigned)q[15] << 24;
        for (int i = 0; i < 16; i++) {
            int px = bx * 4 + (i & 3), py = by * 4 + (i >> 2);
            if (px >= t->w || py >= t->h) continue;
            const int *c = cp[(cbits >> (2 * i)) & 3];
            u8 r = (u8)c[0], g = (u8)c[1], b = (u8)c[2], a = (u8)ap[(abits >> (3 * i)) & 7];
            int part = maria_part(t->id, (px + 0.5f) / t->w, (py + 0.5f) / t->h);
            if (part >= 0 && chr_mcolor[who][part]) ce_apply(chr_mcolor[who][part], &r, &g, &b);
            ((u32 *)((u8 *)lr.pBits + py * lr.Pitch))[px] = (u32)a << 24 | (u32)r << 16 | (u32)g << 8 | b;
        }
    }
    x->lpVtbl->UnlockRect(x, 0);
    return x;
}


static IDirect3DTexture8 *tex_of(IDirect3DDevice8 *dev, Tex *t)
{
    if (t->tex && t->dev == dev) return t->tex;
    t->tex = NULL; t->dev = dev;
    IDirect3DTexture8 *x = NULL;
    if (dev->lpVtbl->CreateTexture(dev, t->w, t->h, 1, 0, D3DFMT_DXT5, D3DPOOL_MANAGED, &x) != D3D_OK || !x) return NULL;
    D3DLOCKED_RECT lr;
    if (x->lpVtbl->LockRect(x, 0, &lr, NULL, 0) != D3D_OK) { x->lpVtbl->Release(x); return NULL; }
    int bw = (t->w + 3) / 4, bh = (t->h + 3) / 4;
    for (int y = 0; y < bh; y++) memcpy((u8 *)lr.pBits + y * lr.Pitch, t->dxt5 + y * bw * 16, bw * 16);
    x->lpVtbl->UnlockRect(x, 0);
    t->tex = x; return x;
}

/* James's state this frame, as the character needs it */
typedef struct {
    M4 jn[41];               /* his bone nodes (character space) */
    int part[2];             /* hand shapes: right 3..13, left 14..20 (0 = none) */
    int torch;               /* flashlight shown in his pocket */
    float ik;                /* left hand onto the held weapon: 0..1 */
} JState;
static int read_james(const u8 *work, JState *js)
{
    const u8 *node = *(const u8 **)(work + 0x9C);
    for (int i = 0; i < 41; i++) { if (!node) return 0; memcpy(&js->jn[i], node + 8, 64); node = *(const u8 **)node; }
    js->part[0] = js->part[1] = 0; js->torch = 0; js->ik = 0;
    const u8 *inst = *(const u8 **)(work + 0x2B0);
    if (inst) {
        const u32 *mask = (const u32 *)(inst + 0x10);
        for (int b = 3; b <= 20; b++) if (mask[b >> 5] & (1u << (b & 31))) { if (b <= 13) { if (!js->part[0]) js->part[0] = b; } else if (!js->part[1]) js->part[1] = b; }
        js->torch = (mask[0] >> 21) & 1;
    }
    return 1;
}
/* grip frame of James's hand s in character space */
/* grip frame of a hand: rotation from the knuckles; origin = the centre of the hand in the shape it has now (James:
   his modelled hand shape; the character: her hand with the fingers bent the same way), so what James holds in the
   middle of his hand ends up in the middle of hers */
static void grip_local(const Hand *H, const float *cent, int ok, M4 *o) { *o = H->G; if (ok) { o->m[12] = cent[0]; o->m[13] = cent[1]; o->m[14] = cent[2]; } }
static void james_grip(const JState *js, int s, M4 *o)
{
    int pt = js->part[s]; M4 g; grip_local(&s_jh[s], pt >= 3 && pt <= 20 ? s_jcent[s][pt] : NULL, pt >= 3 && pt <= 20 && s_jcent_ok[pt], &g);
    m_mul(o, &g, &js->jn[s == 0 ? 35 : 40]);
}
static void chr_grip_local(const Chr *c, const JState *js, int s, M4 *o)
{
    int pt = js->part[s]; grip_local(&c->hand[s], pt >= 3 && pt <= 20 ? c->ccent[s][pt] : NULL, pt >= 3 && pt <= 20 && s_jcent_ok[pt], o);
}

/* pose: W[b] for every bone of character k in CHARACTER space (world = NULL) or world space */
static M4 s_W[128];
static void curl_hand(Chr *c, int s, int part, M4 *Q /* per bone, identity by default */)
{
    Hand *H = &c->hand[s];
    if (!H->ok || part < 3 || part > 20) return;
    const float *cu = kCurl[part];
    for (int f = 0; f < 5; f++) {
        float amt = cu[f]; if (amt < -0.2f) amt = -0.2f; if (amt > 1.1f) amt = 1.1f;
        for (int j = 0; j < 3; j++) {
            int b = H->f[f][j]; if (b < 0) continue;
            float ang = amt * (f == 0 ? kThumbMax[j] : kFingerMax[j]);
            m_axis(&Q[b], H->ax[f][j], ang);
        }
    }
}
static void pose(Chr *c, const JState *js, const M4 *world)
{
    static M4 Q[128];
    for (int b = 0; b < c->nb; b++) m_id(&Q[b]);
    if (js) { curl_hand(c, 0, js->part[0] ? js->part[0] : 3, Q); curl_hand(c, 1, js->part[1] ? js->part[1] : 14, Q); }
    for (int i = 0; i < c->nb; i++) {
        int b = c->order[i], p = c->parent[b];
        M4 *W = &s_W[b];
        if (!js) { *W = c->B[b]; continue; }
        int j = c->map_j[b];
        if (j >= 0 && j < 41) {
            M4 rj; m_rot_only(&rj, &js->jn[j]);
            m_mul(W, &c->P[b], &rj);
            if (p == 0xFF) { W->m[12] = js->jn[j].m[12] * c->sc; W->m[13] = js->jn[j].m[13] * c->sc; W->m[14] = js->jn[j].m[14] * c->sc; }
            else { float o[3]; n_mul(o, c->t_local[b], &s_W[p]); W->m[12] = s_W[p].m[12] + o[0]; W->m[13] = s_W[p].m[13] + o[1]; W->m[14] = s_W[p].m[14] + o[2]; }
        } else if (p != 0xFF) { M4 t; m_mul(&t, &Q[b], &c->L[b]); m_mul(W, &t, &s_W[p]); }
        else *W = c->B[b];
    }
    if (world) for (int b = 0; b < c->nb; b++) m_mul(&s_W[b], &s_W[b], world);
}
/* turn bone r and everything under it about point p (character space) */
static void turn_subtree(Chr *c, int r, const M4 *R, const float *p)
{
    static u8 in[128];
    memset(in, 0, sizeof in); in[r] = 1;
    for (int i = 0; i < c->nb; i++) { int b = c->order[i]; if (c->parent[b] != 0xFF && in[c->parent[b]]) in[b] = 1; }
    for (int b = 0; b < c->nb; b++) if (in[b]) m_turn_about(&s_W[b], R, p);
}
/* left hand onto the weapon: bend her left arm so her left grip frame lands where James's is, relative to the right
   grips (two-bone IK at shoulder and elbow, then the hand turned to the target) */
/* two-bone IK on arm `side` (0 left, 1 right): put the hand bone at Ht (character space), by fraction w, keeping the
   elbow in the plane it bends in now; the hand ends turned to Ht's rotation */
static void arm_ik(Chr *c, int side, const M4 *Ht, float w)
{
    if (w <= 0.001f) return;
    int sh = c->lab[side ? L_ARMR : L_ARML], el = c->lab[side ? L_ELBOWR : L_ELBOWL], hb = c->lab[side ? L_HANDR : L_HANDL];
    float S[3], E[3], Wr[3], T[3];
    bone_pos(S, s_W, sh); bone_pos(E, s_W, el); bone_pos(Wr, s_W, hb);
    T[0] = Wr[0] + (Ht->m[12] - Wr[0]) * w; T[1] = Wr[1] + (Ht->m[13] - Wr[1]) * w; T[2] = Wr[2] + (Ht->m[14] - Wr[2]) * w;
    float se[3], ew[3], st[3]; v_sub(se, E, S); v_sub(ew, Wr, E); v_sub(st, T, S);
    float a = v_len(se), b = v_len(ew), d = v_len(st);
    if (a < 1 || b < 1 || d < 1) return;
    if (d > a + b - 0.5f) d = a + b - 0.5f;
    if (d < fabsf(a - b) + 0.5f) d = fabsf(a - b) + 0.5f;
    float u[3] = { st[0], st[1], st[2] }; v_norm(u);
    float pole[3] = { se[0], se[1], se[2] }; float pu = v_dot(pole, u);
    pole[0] -= pu * u[0]; pole[1] -= pu * u[1]; pole[2] -= pu * u[2];
    if (v_len(pole) < 1e-3f) return;
    v_norm(pole);
    float ca = (a * a + d * d - b * b) / (2 * a * d);
    if (ca > 1) ca = 1;
    if (ca < -1) ca = -1;
    float sa = sqrtf(1 - ca * ca);
    float E2[3] = { S[0] + a * (ca * u[0] + sa * pole[0]), S[1] + a * (ca * u[1] + sa * pole[1]), S[2] + a * (ca * u[2] + sa * pole[2]) };
    float se2[3]; v_sub(se2, E2, S);
    M4 R1; m_from_to(&R1, se, se2, 1.0f); turn_subtree(c, sh, &R1, S);
    bone_pos(Wr, s_W, hb);
    float ew1[3], et[3]; v_sub(ew1, Wr, E2); v_sub(et, T, E2);
    M4 R2; m_from_to(&R2, ew1, et, 1.0f); turn_subtree(c, el, &R2, E2);
    M4 cur, icur, Hr, R3; m_rot_only(&cur, &s_W[hb]); m_rigid_inv(&icur, &cur); m_rot_only(&Hr, Ht);
    M4 delta; m_mul(&delta, &icur, &Hr); m_scale_rot(&R3, &delta, w);
    float P[3]; bone_pos(P, s_W, hb);
    turn_subtree(c, hb, &R3, P);
}
/* left hand onto the weapon: her left grip frame goes where James's is, relative to the right grips */
static void left_hand_ik(Chr *c, const JState *js, float w)
{
    if (w <= 0.001f || !c->hand[0].ok || !c->hand[1].ok || !s_jh[0].ok || !s_jh[1].ok) return;
    int hr = c->lab[L_HANDR];
    M4 gjR, gjL, igjR, rel, gcR, tgt, igcL, Ht;
    james_grip(js, 0, &gjR); james_grip(js, 1, &gjL);
    m_rigid_inv(&igjR, &gjR); m_mul(&rel, &gjL, &igjR);                 /* James's left grip relative to his right */
    M4 gR, gL; chr_grip_local(c, js, 0, &gR); chr_grip_local(c, js, 1, &gL);
    m_mul(&gcR, &gR, &s_W[hr]); m_mul(&tgt, &rel, &gcR);                 /* target left grip, character space */
    m_rigid_inv(&igcL, &gL); m_mul(&Ht, &igcL, &tgt);                    /* her left hand bone there */
    arm_ik(c, 0, &Ht, w);
}
/* a held weapon close to the body: a character with a fuller front (Eddie) holds it that much further out, so it
   doesn't go through her; the right arm reaches forward by the difference, the left hand follows the grip */
static void clear_belly(Chr *c, const JState *js)
{
    if (c->belly <= 2.0f || !js->part[0] || js->part[0] == 3) return;
    int hr = c->lab[L_HANDR];
    M4 gc; m_mul(&gc, &c->hand[0].G, &s_W[hr]);
    float z = gc.m[14], y = gc.m[13];
    if (y < c->B[c->lab[L_SPINE2]].m[13] - 40.0f || y > c->B[c->lab[L_PELVIS]].m[13] + 160.0f) return;   /* not at belly height */
    float w = (c->belly_z + 90.0f - z) / 60.0f; if (w <= 0) return; if (w > 1) w = 1;
    M4 Ht = s_W[hr]; Ht.m[14] += c->belly * w;
    arm_ik(c, 1, &Ht, 1.0f);
}
static void helpers_follow(Chr *c)
{
    for (int i = 0; i < c->nhelp; i++) {
        int b = c->help_b[i];
        M4 wa, rp, ira, rel, half; m_mul(&wa, &c->help_L[i], &s_W[c->help_arm[i]]);
        m_rot_only(&rp, &s_W[b]); m_rigid_inv(&ira, &rp); m_mul(&rel, &ira, &wa); rel.m[12] = rel.m[13] = rel.m[14] = 0;
        m_scale_rot(&half, &rel, 0.5f);
        M4 w; m_mul(&w, &rp, &half);
        w.m[12] = (s_W[b].m[12] + wa.m[12]) * 0.5f; w.m[13] = (s_W[b].m[13] + wa.m[13]) * 0.5f; w.m[14] = (s_W[b].m[14] + wa.m[14]) * 0.5f;
        s_W[b] = w;
    }
}
/* ---- weapons: their shapes, so the character's fingers close ON them instead of through them ----
   The weapon models (data\chr\wp\wp_*.mdl, file id = model id 0x801..0x808) have 2 bones; bone 0 is put on James's
   hand bone by the game, so a point of the weapon in bone 0's frame is a point in James's hand frame. All points
   (vertices + samples over each triangle every ~6 units) are kept in bone 0's frame (bone 1 points through the bind
   pose), only those within 240 units of the hand. */
typedef struct { u32 id; int n; float (*p)[3]; } WGeo;
static WGeo s_wg[10]; static int s_nwg, s_wg_tried;
static const char *const kWpFiles[] = { "data\\chr\\wp\\wp_handgun.mdl", "data\\chr\\wp\\wp_shotgun.mdl", "data\\chr\\wp\\wp_riflgun.mdl",
    "data\\chr\\wp\\wp_sp.mdl", "data\\chr\\wp\\wp_kakuzai.mdl", "data\\chr\\wp\\wp_pipe.mdl", "data\\chr\\wp\\wp_csaw.mdl", "data\\chr\\wp\\wp_nata.mdl" };
#define WG_MAX 16000
static void wg_add(WGeo *g, const float *q)
{
    if (g->n >= WG_MAX || v_len(q) > 700.0f) return;
    for (int i = g->n - 1; i >= 0 && i >= g->n - 64; i--) {          /* cheap de-dup of neighbours */
        float d[3]; v_sub(d, g->p[i], q); if (fabsf(d[0]) < 2 && fabsf(d[1]) < 2 && fabsf(d[2]) < 2) return; }
    memcpy(g->p[g->n++], q, 12);
}
static void load_weapons(void)
{
    if (s_wg_tried) return;
    s_wg_tried = 1;
    for (unsigned f = 0; f < sizeof kWpFiles / sizeof kWpFiles[0] && s_nwg < 10; f++) {
        int n; u8 *d = read_file(kWpFiles[f], &n); Hdr h;
        if (!d) continue;
        if (!header(d, n, &h) || h.nb < 1 || h.nb > 4) { free(d); continue; }
        WGeo *g = &s_wg[s_nwg]; g->id = rd32(d + 4); g->n = 0; g->p = (float (*)[3])malloc(WG_MAX * 12);
        const u8 *m = h.m; M4 B[4]; memcpy(B, m + h.imat, h.nb * 64);
        M4 iB0, toB0[4]; m_rigid_inv(&iB0, &B[0]);
        for (u32 b = 0; b < h.nb; b++) m_mul(&toB0[b], &B[b], &iB0);
        const u16 *ix = (const u16 *)(m + h.ioff); const u8 *p = m + h.prims;
        for (u32 i = 0; i < h.nprim; i++) {
            u32 sz = rd32(p), pnb = rd32(p + 8), boff = rd32(p + 12), st = rd32(p + 0x74), ln = rd32(p + 0x78);
            if (sz < 0x80 || sz > 0x400) break;
            float tri[3][3];
            for (u32 kk = st; kk < st + ln; kk++) {
                unsigned vi = ix[kk]; if (vi >= h.nv) continue;
                const u8 *v = m + h.voff + vi * 48; u32 b = v[44] < pnb ? rd16(p + boff + v[44] * 2) : 0; if (b >= h.nb) b = 0;
                float q[3]; v_mul(q, (const float *)v, &toB0[b]);
                wg_add(g, q);
                memmove(tri[0], tri[1], 24); memcpy(tri[2], q, 12);
                if (kk >= st + 2) {
                    float e1[3], e2[3]; v_sub(e1, tri[1], tri[0]); v_sub(e2, tri[2], tri[0]);
                    float l = v_len(e1) > v_len(e2) ? v_len(e1) : v_len(e2); int ns = (int)(l / 6.0f); if (ns > 40) ns = 40;
                    for (int a = 0; a <= ns; a++) for (int c2 = 0; c2 <= ns - a; c2++) {
                        if (!ns) break;
                        float u = (float)a / ns, w = (float)c2 / ns, r[3] = { tri[0][0] + e1[0] * u + e2[0] * w, tri[0][1] + e1[1] * u + e2[1] * w, tri[0][2] + e1[2] * u + e2[2] * w };
                        wg_add(g, r);
                    }
                }
            }
            p += sz;
        }
        free(d);
        ce_log("chr: weapon 0x%x shape: %d points near the hand", g->id, g->n);
        s_nwg++;
    }
}
static const WGeo *weapon_geo(u32 model)
{
    load_weapons();
    for (int i = 0; i < s_nwg; i++) if (s_wg[i].id == model) return &s_wg[i];
    return NULL;
}
/* James's right grip -> hers, as one matrix (character space) */
static void grip_T(Chr *c, const JState *js, M4 *T)
{
    M4 gj, igj, gc, g;
    james_grip(js, 0, &gj); m_rigid_inv(&igj, &gj);
    chr_grip_local(c, js, 0, &g); m_mul(&gc, &g, &s_W[c->lab[L_HANDR]]);
    m_mul(T, &igj, &gc);
}
static float s_wp[WG_MAX][3]; static int s_nwp;     /* the weapon's points near her hands, character space, this pose */
static int finger_hits(const float *q, float r2)
{
    for (int i = 0; i < s_nwp; i++) { float d0 = s_wp[i][0] - q[0], d1 = s_wp[i][1] - q[1], d2 = s_wp[i][2] - q[2]; if (d0 * d0 + d1 * d1 + d2 * d2 < r2) return 1; }
    return 0;
}
static void finger_set(Chr *c, const Hand *H, int f, float t, const float *cu)
{
    for (int j = 0; j < 3; j++) {
        int b = H->f[f][j], p = c->parent[b]; if (b < 0) continue;
        M4 Q, tm; m_axis(&Q, H->ax[f][j], t * cu[j]); m_mul(&tm, &Q, &c->L[b]); m_mul(&s_W[b], &tm, &s_W[p]);
    }
}
/* samples along segment sgm of a finger (0 knuckle-middle, 1 middle-end, 2 end-tip) */
static int seg_hits(const Hand *H, int f, int sgm, float r2, unsigned *mask, int learn)
{
    float a[3], b[3];
    if (sgm < 2) { bone_pos(a, s_W, H->f[f][sgm]); bone_pos(b, s_W, H->f[f][sgm + 1]); }
    else { bone_pos(a, s_W, H->f[f][2]); v_mul(b, H->tip[f], &s_W[H->f[f][2]]); }
    for (int k = 1; k <= 4; k++) {
        if (!learn && (*mask & (1u << k))) continue;
        float t = sgm == 0 ? 0.3f + 0.7f * k / 4.0f : k / 4.0f;
        float q[3] = { a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t };
        if (finger_hits(q, r2)) { if (learn) *mask |= 1u << k; else return 1; }
    }
    return 0;
}
/* each finger wraps the weapon like a real grip: the knuckle closes toward James's hand shape until the first finger
   bone touches, then the middle joint until the second touches, then the end joint (samples that already touch when
   a joint starts moving - where the palm rests on the grip - do not stop it) */
static void wrap_hand(Chr *c, int s, int part)
{
    Hand *H = &c->hand[s];
    if (!H->ok || part < 3 || part > 20) return;
    float len = 0;
    for (int f = 1; f < 5; f++) { float a[3], b[3]; bone_pos(a, c->B, H->f[f][0]); bone_pos(b, c->B, H->f[f][1]); v_sub(a, b, a); len += v_len(a); }
    float r = len / 4.0f * 0.16f; if (r < 4) r = 4; if (r > 9) r = 9;
    float r2 = r * r;
    for (int f = 0; f < 5; f++) {
        float amt = kCurl[part][f], ang[3] = { 0, 0, 0 }, one[3] = { 1, 1, 1 };
        for (int j = 0; j < 3; j++) {
            float tgt = amt * (f == 0 ? kThumbMax[j] : kFingerMax[j]);
            unsigned pre = 0;
            finger_set(c, H, f, 1, ang);
            for (int sg = j; sg < 3; sg++) { unsigned m = 0; seg_hits(H, f, sg, r2, &m, 1); if (sg == j) pre = m; }
            float a = 0;
            if (j == 0) { ang[0] = tgt; continue; }               /* the knuckle bends like James's: the grip sits in her palm */
            for (float x = 0.06f; x < tgt + 0.0601f; x += 0.06f) {
                float v = x > tgt ? tgt : x; ang[j] = v; finger_set(c, H, f, 1, ang);
                if (seg_hits(H, f, j, r2, &pre, 0)) break;
                a = v;
            }
            ang[j] = a;
        }
        (void)one;
        finger_set(c, H, f, 1, ang);
    }
}
static void wrap_on_weapon(Chr *c, const JState *js, const u8 *weapon)
{
    if (!weapon || !c->hand[0].ok || !s_jh[0].ok) return;
    u32 model = *(const u16 *)(weapon + 0x10);
    const WGeo *g = weapon_geo(model & 0x80F);
    const u8 *node = *(const u8 * const *)(weapon + 0x9C);
    if (!g || !node || !g->n) return;
    M4 T, n0, M; grip_T(c, js, &T); memcpy(&n0, node + 8, 64); m_mul(&M, &n0, &T);
    float hr[3], hl[3]; bone_pos(hr, s_W, c->lab[L_HANDR]); bone_pos(hl, s_W, c->lab[L_HANDL]);
    int left = js->ik > 0.5f;
    s_nwp = 0;
    for (int i = 0; i < g->n; i++) {
        float q[3], d[3]; v_mul(q, g->p[i], &M);
        v_sub(d, q, hr); int close_ = v_len(d) < 170.0f;
        if (!close_ && left) { v_sub(d, q, hl); close_ = v_len(d) < 170.0f; }
        if (close_) memcpy(s_wp[s_nwp++], q, 12);
    }
    if (!s_nwp) return;
    if (js->part[0] && js->part[0] != 3) wrap_hand(c, 0, js->part[0]);
    if (left) wrap_hand(c, 1, js->part[1]);
}
/* full character-space pose for a James this frame: retarget, finger curls, left-hand IK */
typedef struct { const u8 *work; int k; M4 jn[41]; M4 wn; int wid; JState js; M4 W[128]; } PoseCache;
static PoseCache s_pc[2]; static int s_pc_next;
static int pose_for_raw(Chr *c, const u8 *work, JState *js, const u8 *weapon);
static int pose_for(Chr *c, const u8 *work, JState *js, const u8 *weapon)
{
    /* the same James, same bones and same weapon pose as a moment ago (draw, weapon draw, flashlight, the second split
       view): reuse the pose */
    const u8 *node = *(const u8 * const *)(work + 0x9C); M4 jn[41]; M4 wn; int wid = weapon ? *(const u16 *)(weapon + 0x10) : 0;
    for (int i = 0; i < 41; i++) { if (!node) return 0; memcpy(&jn[i], node + 8, 64); node = *(const u8 * const *)node; }
    memset(&wn, 0, sizeof wn);
    if (weapon) { const u8 *wnode = *(const u8 * const *)(weapon + 0x9C); if (wnode) memcpy(&wn, wnode + 8, 64); }
    int k = (int)(c - s_chr);
    for (int i = 0; i < 2; i++) {
        PoseCache *pc = &s_pc[i];
        if (pc->work == work && pc->k == k && pc->wid == wid && !memcmp(pc->jn, jn, sizeof jn) && !memcmp(&pc->wn, &wn, sizeof wn)) {
            *js = pc->js; memcpy(s_W, pc->W, c->nb * 64); return 1;
        }
    }
    if (!pose_for_raw(c, work, js, weapon)) return 0;
    PoseCache *pc = &s_pc[s_pc_next]; s_pc_next ^= 1;
    pc->work = work; pc->k = k; pc->wid = wid; memcpy(pc->jn, jn, sizeof jn); pc->wn = wn; pc->js = *js; memcpy(pc->W, s_W, c->nb * 64);
    return 1;
}
static int pose_for_raw(Chr *c, const u8 *work, JState *js, const u8 *weapon)
{
    if (!read_james(work, js)) return 0;
    /* his left hand is on the weapon when it is in a gripping shape and close to his right grip */
    js->ik = 0;
    if (weapon && js->part[1] && js->part[0] && js->part[0] != 3) {
        /* his left hand is on the weapon when its centre is close to the weapon's shape (in his right hand's frame =
           the weapon's own frame): the pump, the fore-end, the handgun's grip from below - any left hand shape */
        const WGeo *g = weapon_geo(*(const u16 *)(weapon + 0x10) & 0x80F);
        if (g && g->n) {
            M4 b, ir; james_grip(js, 1, &b); m_rigid_inv(&ir, &js->jn[35]);
            float q[3] = { b.m[12], b.m[13], b.m[14] }, lq[3]; v_mul(lq, q, &ir);
            float best = 1e9f;
            for (int i = 0; i < g->n; i++) { float d[3]; v_sub(d, g->p[i], lq); float l = d[0] * d[0] + d[1] * d[1] + d[2] * d[2]; if (l < best) best = l; }
            best = sqrtf(best);
            js->ik = best < 45.0f ? 1.0f : best > 90.0f ? 0.0f : (90.0f - best) / 45.0f;
        }
    }
    pose(c, js, NULL);
    clear_belly(c, js);
    left_hand_ik(c, js, js->ik);
    wrap_on_weapon(c, js, weapon);
    helpers_follow(c);
    return 1;
}

static void light_vertex(PV *o, const float *p, const float *nn, const float *ldir, float amb, float key, float u, float v)
{
    float l = v_len(nn); if (l < 1e-6f) l = 1;
    float d = v_dot(nn, ldir) / l; if (d < 0) d = 0;   /* away from the light: ambient only (CS4 lit backs at 25 %) */
    int cc = (int)((amb + key * d) * 255.0f); if (cc > 255) cc = 255; if (cc < 0) cc = 0;
    o->x = p[0]; o->y = p[1]; o->z = p[2]; o->u = u; o->v = v;
    o->c = 0xFF000000u | (cc << 16) | (cc << 8) | cc;
}
static void skin_and_light(Chr *c, const float *ldir, float amb, float key)
{
    for (int i = 0; i < c->nv; i++) {
        Vtx *v = &c->v[i]; float p[3] = {0, 0, 0}, nn[3] = {0, 0, 0};
        for (int s = 0; s < v->nb; s++) {
            float t[3], tn[3]; const M4 *W = &s_W[v->b[s]];
            v_mul(t, v->q[s], W); n_mul(tn, v->n[s], W);
            p[0] += t[0] * v->w[s]; p[1] += t[1] * v->w[s]; p[2] += t[2] * v->w[s];
            nn[0] += tn[0] * v->w[s]; nn[1] += tn[1] * v->w[s]; nn[2] += tn[2] * v->w[s];
        }
        light_vertex(&c->pv[i], p, nn, ldir, amb, key, v->uv[0], v->uv[1]);
    }
    if (c->lnear) for (int i = c->nvo; i < c->nv; i++) { int j = c->lnear[i - c->nvo]; if (j >= 0) c->pv[i].c = c->pv[j].c; }
}
static void torch_light(Chr *c, const float *ldir, float amb, float key)
{
    Torch *t = &c->torch;
    for (int i = 0; i < t->nv; i++) {
        float p[3], nn[3]; v_mul(p, t->q[i], &s_W[c->sp2]); n_mul(nn, t->n[i], &s_W[c->sp2]);
        light_vertex(&c->tpv[i], p, nn, ldir, amb, key, t->uv[i][0], t->uv[i][1]);
    }
}
static IDirect3DTexture8 *tex_for(IDirect3DDevice8 *dev, Chr *c, int slot, int who)
{
    if (who >= 0 && who < 2 && c == &s_chr[CHR_MARIA]) {
        if (c->rdev[who][slot] != dev || c->rgen[who][slot] != s_mgen[who]) {
            if (c->rtex[who][slot] && c->rdev[who][slot] == dev) c->rtex[who][slot]->lpVtbl->Release(c->rtex[who][slot]);
            c->rtex[who][slot] = maria_tex(dev, &c->tex[slot], who); c->rdev[who][slot] = dev; c->rgen[who][slot] = s_mgen[who];
        }
        if (c->rtex[who][slot]) return c->rtex[who][slot];
    }
    return tex_of(dev, &c->tex[slot]);
}
static void draw_mesh(IDirect3DDevice8 *dev, Chr *c, const D3DMATRIX *view, const D3DMATRIX *proj, int fog, int torch, int who)
{
    D3DMATRIX w_old, v_old, p_old, I; memset(&I, 0, sizeof I); I._11 = I._22 = I._33 = I._44 = 1;
    dev->lpVtbl->GetTransform(dev, D3DTS_WORLD, &w_old); dev->lpVtbl->GetTransform(dev, D3DTS_VIEW, &v_old); dev->lpVtbl->GetTransform(dev, D3DTS_PROJECTION, &p_old);
    static const D3DRENDERSTATETYPE rs[] = { D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_ZFUNC, D3DRS_CULLMODE, D3DRS_LIGHTING, D3DRS_ALPHABLENDENABLE,
        D3DRS_ALPHATESTENABLE, D3DRS_ALPHAREF, D3DRS_ALPHAFUNC, D3DRS_FOGENABLE, D3DRS_STENCILENABLE, D3DRS_SPECULARENABLE, D3DRS_COLORVERTEX,
        D3DRS_FILLMODE, D3DRS_SHADEMODE, D3DRS_CLIPPING, D3DRS_COLORWRITEENABLE, D3DRS_FOGTABLEMODE, D3DRS_FOGVERTEXMODE, D3DRS_SRCBLEND, D3DRS_DESTBLEND };
    DWORD rsv[sizeof rs / sizeof rs[0]];
    for (unsigned i = 0; i < sizeof rs / sizeof rs[0]; i++) dev->lpVtbl->GetRenderState(dev, rs[i], &rsv[i]);
    static const D3DTEXTURESTAGESTATETYPE ts[] = { D3DTSS_COLOROP, D3DTSS_COLORARG1, D3DTSS_COLORARG2, D3DTSS_ALPHAOP, D3DTSS_ALPHAARG1, D3DTSS_MINFILTER, D3DTSS_MAGFILTER, D3DTSS_MIPFILTER, D3DTSS_ADDRESSU, D3DTSS_ADDRESSV, D3DTSS_TEXCOORDINDEX, D3DTSS_TEXTURETRANSFORMFLAGS };
    DWORD tsv[sizeof ts / sizeof ts[0]], ts1op;
    for (unsigned i = 0; i < sizeof ts / sizeof ts[0]; i++) dev->lpVtbl->GetTextureStageState(dev, 0, ts[i], &tsv[i]);
    dev->lpVtbl->GetTextureStageState(dev, 1, D3DTSS_COLOROP, &ts1op);
    DWORD vs_old = 0, ps_old = 0; dev->lpVtbl->GetVertexShader(dev, &vs_old); dev->lpVtbl->GetPixelShader(dev, &ps_old);
    IDirect3DBaseTexture8 *tex_old = NULL; dev->lpVtbl->GetTexture(dev, 0, &tex_old);

    dev->lpVtbl->SetTransform(dev, D3DTS_WORLD, &I); dev->lpVtbl->SetTransform(dev, D3DTS_VIEW, view); dev->lpVtbl->SetTransform(dev, D3DTS_PROJECTION, proj);
    dev->lpVtbl->SetRenderState(dev, D3DRS_ZENABLE, D3DZB_TRUE); dev->lpVtbl->SetRenderState(dev, D3DRS_ZWRITEENABLE, TRUE); dev->lpVtbl->SetRenderState(dev, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    dev->lpVtbl->SetRenderState(dev, D3DRS_CULLMODE, D3DCULL_NONE); dev->lpVtbl->SetRenderState(dev, D3DRS_LIGHTING, FALSE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHABLENDENABLE, FALSE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAREF, 0x60); dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    dev->lpVtbl->SetRenderState(dev, D3DRS_FOGENABLE, fog ? TRUE : FALSE); dev->lpVtbl->SetRenderState(dev, D3DRS_STENCILENABLE, FALSE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_SPECULARENABLE, FALSE); dev->lpVtbl->SetRenderState(dev, D3DRS_COLORVERTEX, TRUE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_FILLMODE, D3DFILL_SOLID); dev->lpVtbl->SetRenderState(dev, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    dev->lpVtbl->SetRenderState(dev, D3DRS_CLIPPING, TRUE); dev->lpVtbl->SetRenderState(dev, D3DRS_COLORWRITEENABLE, 0xF);
    if (fog) { dev->lpVtbl->SetRenderState(dev, D3DRS_FOGTABLEMODE, D3DFOG_NONE); dev->lpVtbl->SetRenderState(dev, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR); }
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLOROP, D3DTOP_MODULATE); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_TEXCOORDINDEX, 0); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->lpVtbl->SetPixelShader(dev, 0); dev->lpVtbl->SetVertexShader(dev, PV_FVF);
    #define DRAW_PRIM(i) do { Prim *pr = &c->prim[i]; \
        dev->lpVtbl->SetTexture(dev, 0, (IDirect3DBaseTexture8 *)(pr->tex >= 0 ? tex_for(dev, c, pr->tex, who) : NULL)); \
        if (pr->length >= 3 && !(chr_test_hide >= 0 && (i) < 32 && (chr_test_hide >> (i)) & 1)) \
            dev->lpVtbl->DrawIndexedPrimitiveUP(dev, D3DPT_TRIANGLESTRIP, 0, c->nv, pr->length - 2, pr->ix + pr->start, D3DFMT_INDEX16, c->pv, sizeof(PV)); } while (0)
    for (int i = 0; i < c->nopaque; i++) {
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHATESTENABLE, c->prim[i].atest ? TRUE : FALSE);
        DRAW_PRIM(i);
    }
    if (c->nprim > c->nopaque) {
        /* the hair strand layers, drawn like solid hair: pass 1 = every texel that is mostly covered (alpha >= ref) solid,
           with depth writes, so nothing behind shows through; pass 2 = only the faint strand ends (alpha < ref) blended
           on top, for soft tips (CS1-CS6 blended the whole layer, so the lower hair was see-through) */
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHATESTENABLE, TRUE);
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL); dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAREF, (DWORD)chr_hair_ref);
        for (int i = c->nopaque; i < c->nprim; i++) DRAW_PRIM(i);
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHABLENDENABLE, TRUE);
        dev->lpVtbl->SetRenderState(dev, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); dev->lpVtbl->SetRenderState(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        dev->lpVtbl->SetRenderState(dev, D3DRS_ZWRITEENABLE, FALSE);
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAFUNC, D3DCMP_LESS);
        for (int i = c->nopaque; i < c->nprim; i++) DRAW_PRIM(i);
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHABLENDENABLE, FALSE); dev->lpVtbl->SetRenderState(dev, D3DRS_ZWRITEENABLE, TRUE);
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL); dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAREF, 0x60);
    }
    #undef DRAW_PRIM
    if (torch && c->torch.nv && s_jtorch_ok) {
        Torch *t = &c->torch;
        dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHATESTENABLE, FALSE);
        dev->lpVtbl->SetTexture(dev, 0, (IDirect3DBaseTexture8 *)tex_of(dev, &s_jtorch_tex));
        for (int i = 0; i < t->nprim; i++)
            if (t->prim_len[i] >= 3) dev->lpVtbl->DrawIndexedPrimitiveUP(dev, D3DPT_TRIANGLESTRIP, 0, t->nv, t->prim_len[i] - 2, t->idx + t->prim_start[i], D3DFMT_INDEX16, c->tpv, sizeof(PV));
    }
    dev->lpVtbl->SetTexture(dev, 0, tex_old); if (tex_old) tex_old->lpVtbl->Release(tex_old);
    dev->lpVtbl->SetVertexShader(dev, vs_old); dev->lpVtbl->SetPixelShader(dev, ps_old);
    for (unsigned i = 0; i < sizeof ts / sizeof ts[0]; i++) dev->lpVtbl->SetTextureStageState(dev, 0, ts[i], tsv[i]);
    dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_COLOROP, ts1op);
    for (unsigned i = 0; i < sizeof rs / sizeof rs[0]; i++) dev->lpVtbl->SetRenderState(dev, rs[i], rsv[i]);
    dev->lpVtbl->SetTransform(dev, D3DTS_WORLD, &w_old); dev->lpVtbl->SetTransform(dev, D3DTS_VIEW, &v_old); dev->lpVtbl->SetTransform(dev, D3DTS_PROJECTION, &p_old);
}
int chr_draw_world(IDirect3DDevice8 *dev, int k, const unsigned char *work, const unsigned char *weapon, int who)
{
    if (!dev || !chr_ready(k)) return 0;
    Chr *c = &s_chr[k];
    JState js;
    if (!pose_for(c, work, &js, weapon)) return 0;
    M4 world; memcpy(&world, work + 0x5C, 64);
    world.m[12] += chr_test_offset[0]; world.m[13] += chr_test_offset[1]; world.m[14] += chr_test_offset[2];
    for (int b = 0; b < c->nb; b++) m_mul(&s_W[b], &s_W[b], &world);
    /* key light from the camera (the flashlight is usually near the viewer): view matrix 3rd column = camera forward */
    const float *V = (const float *)0x01F80E00;
    /* the game lights its characters mostly from the front (their back is dark - see James from behind): key light
       from her front and a little above, with a little of the camera direction as fill */
    float cam[3] = { -V[2], -V[6], -V[10] }; v_norm(cam);
    float fw[3] = { -world.m[8], -world.m[9], -world.m[10] }; v_norm(fw);   /* her front (local -z in world) */
    float l[3] = { fw[0] * 0.8f + cam[0] * 0.35f, fw[1] * 0.8f + cam[1] * 0.35f - 0.45f, fw[2] * 0.8f + cam[2] * 0.35f }; v_norm(l);
    skin_and_light(c, l, 0.22f, 0.78f);
    if (js.torch) torch_light(c, l, 0.22f, 0.78f);
    draw_mesh(dev, c, (const D3DMATRIX *)0x01F80E00, (const D3DMATRIX *)0x01F80E40, 0, js.torch, who);
    return 1;
}
/* the weapon James holds, moved from his right grip frame to hers (character space; the weapon shares James's world
   matrix) - every weapon the same way, since only the grip frames are used */
int chr_weapon_begin(int k, const unsigned char *james, unsigned char *weapon, ChrWeaponSave *sv)
{
    sv->n = 0;
    if (!chr_ready(k)) return 0;
    Chr *c = &s_chr[k];
    if (!c->hand[0].ok || !s_jh[0].ok) return 0;
    JState js;
    if (!pose_for(c, james, &js, weapon)) return 0;
    M4 T; grip_T(c, &js, &T);
    if (chr_test_offset[0] != 0 || chr_test_offset[1] != 0 || chr_test_offset[2] != 0) {   /* cloud test: side by side */
        M4 w, iw, off; memcpy(&w, james + 0x5C, 64); m_rigid_inv(&iw, &w); m_id(&off);
        off.m[12] = chr_test_offset[0]; off.m[13] = chr_test_offset[1]; off.m[14] = chr_test_offset[2];
        M4 t; m_mul(&t, &w, &off); m_mul(&t, &t, &iw); m_mul(&T, &T, &t);
    }
    u8 *node = *(u8 **)(weapon + 0x9C);
    while (node && sv->n < 4) {
        M4 m; memcpy(&m, node + 8, 64);
        sv->node[sv->n] = node; memcpy(sv->m[sv->n], &m, 64); sv->n++;
        M4 r; m_mul(&r, &m, &T); r.m[3] = m.m[3]; r.m[7] = m.m[7]; r.m[11] = m.m[11]; r.m[15] = m.m[15];
        memcpy(node + 8, &r, 64);
        node = *(u8 **)node;
    }
    return sv->n > 0;
}
/* where the flashlight light and its lens glow must move to: from James's pocket to the one on her chest (world) */
int chr_torch_delta(int k, const unsigned char *james, const unsigned char *weapon, float *d)
{
    if (!chr_ready(k) || !s_jtorch_nv) return 0;
    Chr *c = &s_chr[k];
    JState js;
    if (!pose_for(c, james, &js, weapon)) return 0;
    M4 world, ij3, t; memcpy(&world, james + 0x5C, 64);
    float pj[3], pc[3], a[3], b[3];
    m_rigid_inv(&ij3, &s_jB[3]); v_mul(a, s_jtorch_c, &ij3); m_mul(&t, &js.jn[3], &world); v_mul(pj, a, &t);
    M4 isp; m_rigid_inv(&isp, &c->B[c->sp2]); v_mul(b, c->torch_c, &isp); m_mul(&t, &s_W[c->sp2], &world); v_mul(pc, b, &t);
    d[0] = pc[0] - pj[0]; d[1] = pc[1] - pj[1]; d[2] = pc[2] - pj[2];
    return 1;
}
void chr_weapon_end(ChrWeaponSave *sv)
{
    for (int i = 0; i < sv->n; i++) memcpy(sv->node[i] + 8, sv->m[i], 64);
    sv->n = 0;
}
/* preview: bind pose turned by yaw, in a viewport (same camera as the Appearance preview) */
void chr_draw_preview(IDirect3DDevice8 *dev, int k, int who, int px, int py, int pw, int ph, float yaw)
{
    if (!dev || !chr_ready(k) || pw < 8 || ph < 8) return;
    Chr *c = &s_chr[k];
    M4 world; memset(&world, 0, sizeof world);
    float cy = cosf(yaw), sy = sinf(yaw);
    world.m[0] = cy; world.m[2] = -sy; world.m[5] = -1.0f; world.m[8] = sy; world.m[10] = cy; world.m[15] = 1;   /* Y up for the preview */
    pose(c, NULL, &world);
    float l[3] = { -0.45f, 0.55f, -0.70f };
    skin_and_light(c, l, 0.55f, 0.6f);
    D3DVIEWPORT8 vp_old; dev->lpVtbl->GetViewport(dev, &vp_old);
    D3DVIEWPORT8 vp = { (DWORD)px, (DWORD)py, (DWORD)pw, (DWORD)ph, 0.0f, 1.0f };
    dev->lpVtbl->SetViewport(dev, &vp);
    dev->lpVtbl->Clear(dev, 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    D3DMATRIX V, P; memset(&V, 0, sizeof V); memset(&P, 0, sizeof P);
    float ey = 465.0f, ez = -2250.0f;
    V._11 = 1; V._22 = 1; V._33 = 1; V._44 = 1; V._42 = -ey; V._43 = -ez;
    float fov = 24.0f * 3.14159265f / 180.0f, aspect = (float)pw / (float)ph, zn = 100.0f, zf = 8000.0f, ys = 1.0f / tanf(fov * 0.5f);
    P._11 = ys / aspect; P._22 = ys; P._33 = zf / (zf - zn); P._34 = 1.0f; P._43 = -zn * zf / (zf - zn);
    draw_mesh(dev, c, &V, &P, 0, 0, who);
    dev->lpVtbl->SetViewport(dev, &vp_old);
}
