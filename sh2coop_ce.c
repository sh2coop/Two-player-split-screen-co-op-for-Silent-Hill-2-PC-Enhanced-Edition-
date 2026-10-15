/* sh2coop_ce.c - character editor (experimental build, separate from the released co-op mod)
 *
 *   Colours: each James has a palette entry per part (jacket, shirt, jeans, boots, hair). A texel of James's
 *   textures is classified into a part by ce_part_of (regions of the texture + colour tests, the same rules the
 *   released mod uses for James 2's red jacket and brown hair) and recoloured as  out = L * m + b  with L the texel's
 *   luminance, so the cloth's shading and folds stay.  Stored in sh2coop.ini [look] as  james1_jacket = Red  etc.
 *
 *   Preview: the title screen has no character loaded, so the editor reads James's own model file
 *   (data\chr\jms\hhh_jms.mdl - the PC format: 64-byte file header, model section at +0x40 with bone initial
 *   matrices / primitive headers / 48-byte vertices / triangle-strip indices, textures as DXT5 containers at the
 *   end) once, puts the vertices into bind pose on the CPU (vertex * initial_matrix[first bone], row vectors, Y down)
 *   and draws him with the game's Direct3D 8 device: fixed-function, no lighting state (the light is baked into the
 *   vertex colours every frame), alpha test for the hair strands, inside a viewport over the menu page. */
#include "sh2coop_ce.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

const char *const ce_part_name[CE_PARTS] = { "Jacket", "Shirt", "Jeans", "Boots", "Hair" };
/* out = L * m + b.  The original jacket is a dark green (L ~ 40..90), the shirt a grey-beige (L ~ 60..150), the
   jeans a dark blue, the hair blond (L ~ 60..180): the multipliers are chosen so mid-tones land on the named colour. */
const CeColor ce_palette[] = {
    { "Original",    { 1.00f, 1.00f, 1.00f }, { 0, 0, 0 } },
    { "Black",       { 0.22f, 0.22f, 0.22f }, { 0, 0, 0 } },
    { "Dark grey",   { 0.55f, 0.55f, 0.55f }, { 0, 0, 0 } },
    { "Grey",        { 1.00f, 1.00f, 1.00f }, { 10, 10, 10 } },
    { "White",       { 1.80f, 1.80f, 1.80f }, { 40, 40, 40 } },
    { "Red",         { 1.70f, 0.26f, 0.24f }, { 12, 2, 2 } },
    { "Dark red",    { 1.05f, 0.16f, 0.14f }, { 6, 0, 0 } },
    { "Orange",      { 1.90f, 0.95f, 0.20f }, { 14, 4, 0 } },
    { "Yellow",      { 1.90f, 1.75f, 0.30f }, { 20, 18, 0 } },
    { "Green",       { 0.45f, 1.50f, 0.40f }, { 0, 8, 0 } },
    { "Dark green",  { 0.28f, 0.85f, 0.30f }, { 0, 4, 0 } },
    { "Olive",       { 0.95f, 0.90f, 0.40f }, { 4, 4, 0 } },
    { "Blue",        { 0.30f, 0.62f, 1.90f }, { 0, 4, 14 } },
    { "Navy",        { 0.18f, 0.28f, 0.95f }, { 0, 0, 6 } },
    { "Light blue",  { 1.00f, 1.40f, 1.90f }, { 20, 30, 40 } },
    { "Purple",      { 1.20f, 0.40f, 1.60f }, { 8, 0, 12 } },
    { "Pink",        { 1.90f, 1.00f, 1.40f }, { 30, 10, 20 } },
    { "Brown",       { 1.05f, 0.65f, 0.40f }, { 6, 3, 0 } },
    { "Dark brown",  { 0.55f, 0.34f, 0.22f }, { 10, 6, 4 } },
    { "Blond",       { 1.45f, 1.25f, 0.75f }, { 20, 16, 6 } },
    { "Ginger",      { 1.60f, 0.75f, 0.30f }, { 14, 4, 0 } },
    { "Silver",      { 1.35f, 1.35f, 1.45f }, { 30, 30, 34 } },
};
const int ce_palette_n = sizeof ce_palette / sizeof ce_palette[0];
int ce_color[2][CE_PARTS];
int ce_gen[2];
static char s_ini[MAX_PATH];

static inline ce_u8 clamp8f(float v) { return v < 0 ? 0 : v > 255 ? 255 : (ce_u8)v; }
int ce_part_of(ce_u32 id, float fx, float fy, int R, int G, int B)
{
    float L = 0.30f * R + 0.59f * G + 0.11f * B;
    if (id == 1240) {                                                      /* jacket (+ hands at the left of the lower half) */
        if (B > G + 10 || L > 110.0f || (R > G + 25 && R > 70)) return -1;  /* badges, flag, metal studs */
        if (R > G && G > B && R - B > 20 && L > 40.0f) return -1;           /* skin */
        if (fx < 0.5625f && fy >= 0.5f && G < R) return -1;                /* hands picture: only the green-ish texels there are jacket */
        return CE_JACKET;
    }
    if (id == 1242) return CE_HAIR;                                        /* all hair */
    if (id == 1241) {
        if (fx >= 0.5f) return CE_HAIR;                                    /* right half = hair strands + their backing */
        if (fx >= 0.25f) return CE_JEANS;                                  /* second quarter = jeans */
        if (fy < 0.5f) return CE_SHIRT;                                    /* top-left quarter = shirt */
        if (fx < 0.19f && fy < 0.75f) return CE_BOOTS;                     /* boots (the belt buckle / sole pictures are left) */
        return -1;
    }
    if (id == 1239) {                                                      /* hair painted on the head texture: told from skin by colour */
        int inreg = (fy < 0.30f && fx >= 0.27f && fx <= 0.97f) || (fx >= 0.60f && fy < 0.60f) ||
                    (fy >= 0.84f && (fx >= 0.70f || fx <= 0.08f)) || (fx <= 0.07f && fy >= 0.53f && fy <= 0.60f);
        if (inreg && L > 8.0f && R - G < 20 && G >= B - 4) return CE_HAIR;
        return -1;
    }
    return -1;
}
void ce_apply(int ci, ce_u8 *r, ce_u8 *g, ce_u8 *b)
{
    if (ci <= 0 || ci >= ce_palette_n) return;
    const CeColor *c = &ce_palette[ci];
    float L = 0.30f * *r + 0.59f * *g + 0.11f * *b;
    *r = clamp8f(L * c->m[0] + c->b[0]); *g = clamp8f(L * c->m[1] + c->b[1]); *b = clamp8f(L * c->m[2] + c->b[2]);
}
int ce_any(int who) { for (int p = 0; p < CE_PARTS; p++) if (ce_color[who][p]) return 1; return 0; }

static const char *kKey[2][CE_PARTS] = { { "james1_jacket", "james1_shirt", "james1_jeans", "james1_boots", "james1_hair" },
                                          { "james2_jacket", "james2_shirt", "james2_jeans", "james2_boots", "james2_hair" } };
static int find_color(const char *s)
{
    for (int i = 0; i < ce_palette_n; i++) if (!_stricmp(s, ce_palette[i].name)) return i;
    return -1;
}
void ce_load(const char *inipath, int james2_red_jacket)
{
    strncpy(s_ini, inipath, MAX_PATH - 1);
    memset(ce_color, 0, sizeof ce_color);
    if (james2_red_jacket) { ce_color[1][CE_JACKET] = find_color("Red"); ce_color[1][CE_HAIR] = find_color("Dark brown"); }
    int missing = 0;
    for (int w = 0; w < 2; w++) for (int p = 0; p < CE_PARTS; p++) {
        char v[64];
        if (GetPrivateProfileStringA("look", kKey[w][p], "", v, sizeof v, s_ini) == 0) { missing = 1; continue; }
        int ci = find_color(v);
        if (ci < 0) { ce_log("look: sh2coop.ini [look] %s = \"%s\" is not a colour name - using the default", kKey[w][p], v); continue; }
        ce_color[w][p] = ci;
    }
    if (missing) ce_save();                                             /* make the section visible in older ini files */
    ce_log("look: James 1 = %s / %s / %s / %s / %s | James 2 = %s / %s / %s / %s / %s (jacket / shirt / jeans / boots / hair)",
           ce_palette[ce_color[0][0]].name, ce_palette[ce_color[0][1]].name, ce_palette[ce_color[0][2]].name, ce_palette[ce_color[0][3]].name, ce_palette[ce_color[0][4]].name,
           ce_palette[ce_color[1][0]].name, ce_palette[ce_color[1][1]].name, ce_palette[ce_color[1][2]].name, ce_palette[ce_color[1][3]].name, ce_palette[ce_color[1][4]].name);
}
void ce_save(void)
{
    if (!s_ini[0]) return;
    for (int w = 0; w < 2; w++) for (int p = 0; p < CE_PARTS; p++)
        WritePrivateProfileStringA("look", kKey[w][p], ce_palette[ce_color[w][p]].name, s_ini);
}
void ce_cycle(int who, int part, int dir)
{
    ce_color[who][part] = (ce_color[who][part] + dir + ce_palette_n) % ce_palette_n;
    ce_gen[who]++;
    ce_save();
    ce_log("look: James %d %s = %s", who + 1, ce_part_name[part], ce_palette[ce_color[who][part]].name);
}

/* ===================== the preview model ===================== */
typedef struct { float x, y, z; DWORD color; float u, v; } PV;        /* D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 */
#define PV_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
typedef struct { int tex; int start, length; } Prim;
typedef struct { ce_u32 id; int w, h; const ce_u8 *dxt5; IDirect3DTexture8 *tex[2]; int built[2]; } Tex;
static ce_u8 *s_file; static int s_file_n;
static PV *s_pv; static float *s_nrm; static int s_nv;                 /* bind-pose positions + normals (model space) */
static const unsigned short *s_idx;
static Prim s_prim[32]; static int s_nprim;
static Tex s_tex[8]; static int s_ntex;
static HRESULT s_clear_hr; static int s_loaded = -1; static char s_status[160] = "not loaded";
const char *ce_preview_status(void) { return s_status; }

static ce_u32 rd32(const ce_u8 *p) { return *(const ce_u32 *)p; }
static unsigned rd16(const ce_u8 *p) { return *(const unsigned short *)p; }
static int tex_slot(ce_u32 id) { for (int i = 0; i < s_ntex; i++) if (s_tex[i].id == id) return i; return -1; }

int ce_preview_load(const char *gamedir)
{
    if (s_loaded >= 0) return s_loaded;
    s_loaded = 0;
    char path[MAX_PATH]; snprintf(path, sizeof path, "%sdata\\chr\\jms\\hhh_jms.mdl", gamedir);
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(path, sizeof path, "%s..\\data\\chr\\jms\\hhh_jms.mdl", gamedir); f = fopen(path, "rb"); }   /* the mod in scripts\ */
    if (!f) { snprintf(s_status, sizeof s_status, "model file not found: %s", path); ce_log("look preview: %s", s_status); return 0; }
    fseek(f, 0, SEEK_END); s_file_n = (int)ftell(f); fseek(f, 0, SEEK_SET);
    s_file = (ce_u8 *)malloc(s_file_n);
    if (!s_file || fread(s_file, 1, s_file_n, f) != (size_t)s_file_n) { fclose(f); snprintf(s_status, sizeof s_status, "model file could not be read"); return 0; }
    fclose(f);
    const ce_u8 *d = s_file;
    ce_u32 T = rd32(d + 12), M = rd32(d + 20), ntexf = rd32(d + 8);
    if (M + 0xA0 > (ce_u32)s_file_n || rd32(d + M) != 0xFFFF0003u) { snprintf(s_status, sizeof s_status, "not a PC model file (magic %08x)", M < (ce_u32)s_file_n ? rd32(d + M) : 0); ce_log("look preview: %s", s_status); return 0; }
    const ce_u8 *m = d + M;
    ce_u32 imat = rd32(m + 8), nb = rd32(m + 12), nprim = rd32(m + 32), prims = rd32(m + 36);
    ce_u32 ntexblk = rd32(m + 48), texblk = rd32(m + 52), ntexid = rd32(m + 56), texid = rd32(m + 60);
    ce_u32 nv = rd32(m + 128), voff = rd32(m + 132), ioff = rd32(m + 144), tioff = rd32(m + 148);
    if (nb > 256 || nprim > 32 || nv > 20000 || ntexblk > 8 || ntexid > 64) { snprintf(s_status, sizeof s_status, "model header out of range"); return 0; }
    /* textures: containers at T + 16; each: id, w, h, w2, h2, sprite count, 18 bytes; sprite headers of 32 bytes; the
       pixel data follows the last header, sized by that header's data_size.  Only DXT5 (format 4) is expected. */
    const ce_u8 *t = d + T + 16;
    for (ce_u32 i = 0; i < ntexf && s_ntex < 8; i++) {
        if (t + 32 > d + s_file_n) break;
        ce_u32 id = rd32(t); int w = rd16(t + 4), hgt = rd16(t + 6), ns = rd16(t + 12);
        const ce_u8 *sh = t + 32; ce_u32 dsz = 0; int fmt = 0;
        for (int s = 0; s < ns; s++) { fmt = sh[12]; dsz = rd32(sh + 16); sh += 32; }
        if (sh + dsz > d + s_file_n) break;
        if (fmt == 4 && dsz == (ce_u32)(w * hgt)) { s_tex[s_ntex].id = id; s_tex[s_ntex].w = w; s_tex[s_ntex].h = hgt; s_tex[s_ntex].dxt5 = sh; s_ntex++; }
        else ce_log("look preview: texture %u is format %d (%u bytes) - not used", id, fmt, dsz);
        t = sh + dsz;
    }
    /* vertices into bind pose: 48-byte vertices, bone index 0 picks a bone of the primitive's bone list; the bone's
       initial matrix is its model-space pose (row vectors). */
    s_nv = (int)nv;
    s_pv = (PV *)calloc(nv, sizeof(PV)); s_nrm = (float *)calloc(nv, 3 * sizeof(float));
    s_idx = (const unsigned short *)(m + ioff);
    int nidx = (int)((tioff > ioff ? tioff - ioff : T - M - ioff) / 2);
    const ce_u8 *p = m + prims; s_nprim = 0;
    unsigned char *done = (unsigned char *)calloc(nv, 1);
    for (ce_u32 i = 0; i < nprim; i++) {
        ce_u32 sz = rd32(p), pnb = rd32(p + 8), boff = rd32(p + 12), ntx = rd32(p + 24), txoff = rd32(p + 28);
        ce_u32 start = rd32(p + 0x74), length = rd32(p + 0x78);
        if (sz < 0x80 || sz > 0x400 || (int)(start + length) > nidx) break;
        int slot = -1;
        if (ntx) { ce_u32 ti = rd16(p + txoff); if (ti < ntexid) { ce_u32 blk = rd32(m + texid + ti * 8); if (blk < ntexblk) slot = tex_slot(rd32(m + texblk + blk * 4)); } }
        for (ce_u32 k = start; k < start + length; k++) {
            unsigned vi = s_idx[k]; if (vi >= nv || done[vi]) continue;
            done[vi] = 1;
            const ce_u8 *v = m + voff + vi * 48;
            const float *pos = (const float *)v; const short *n16 = (const short *)(v + 28); const float *uv = (const float *)(v + 36);
            unsigned bi = v[44]; ce_u32 bone = bi < pnb ? rd16(p + boff + bi * 2) : 0; if (bone >= nb) bone = 0;
            const float *M4 = (const float *)(m + imat + bone * 64);
            float x = pos[0] * M4[0] + pos[1] * M4[4] + pos[2] * M4[8] + M4[12];
            float y = pos[0] * M4[1] + pos[1] * M4[5] + pos[2] * M4[9] + M4[13];
            float z = pos[0] * M4[2] + pos[1] * M4[6] + pos[2] * M4[10] + M4[14];
            float nx = n16[0] / 32767.0f, ny = n16[1] / 32767.0f, nz = n16[2] / 32767.0f;
            float wx = nx * M4[0] + ny * M4[4] + nz * M4[8], wy = nx * M4[1] + ny * M4[5] + nz * M4[9], wz = nx * M4[2] + ny * M4[6] + nz * M4[10];
            float ln = sqrtf(wx * wx + wy * wy + wz * wz); if (ln < 1e-6f) ln = 1.0f;
            s_pv[vi].x = x; s_pv[vi].y = -y; s_pv[vi].z = z;                /* the model is Y-down: flipped to Y-up here */
            s_pv[vi].u = uv[0]; s_pv[vi].v = uv[1]; s_pv[vi].color = 0xFFFFFFFF;
            s_nrm[vi * 3] = wx / ln; s_nrm[vi * 3 + 1] = -wy / ln; s_nrm[vi * 3 + 2] = wz / ln;
        }
        s_prim[s_nprim].tex = slot; s_prim[s_nprim].start = (int)start; s_prim[s_nprim].length = (int)length; s_nprim++;
        p += sz;
    }
    free(done);
    s_loaded = s_nprim > 0 && s_ntex > 0;
    snprintf(s_status, sizeof s_status, "%s: %d parts, %d vertices, %d textures (%u bones)", s_loaded ? "ready" : "unusable", s_nprim, s_nv, s_ntex, nb);
    ce_log("look preview: %s - %s", path, s_status);
    return s_loaded;
}
void ce_preview_release(void)
{
    for (int i = 0; i < s_ntex; i++) for (int w = 0; w < 2; w++) { if (s_tex[i].tex[w]) s_tex[i].tex[w]->lpVtbl->Release(s_tex[i].tex[w]); s_tex[i].tex[w] = NULL; s_tex[i].built[w] = 0; }
}
static IDirect3DTexture8 *preview_tex(IDirect3DDevice8 *dev, int slot, int who)
{
    Tex *t = &s_tex[slot];
    if (t->built[who] && t->tex[who]) return t->tex[who];
    if (t->tex[who]) { t->tex[who]->lpVtbl->Release(t->tex[who]); t->tex[who] = NULL; }
    IDirect3DTexture8 *x = NULL;
    if (dev->lpVtbl->CreateTexture(dev, t->w, t->h, 1, 0, D3DFMT_DXT5, D3DPOOL_MANAGED, &x) != D3D_OK || !x) { ce_log("look preview: CreateTexture %u failed", t->id); t->built[who] = 1; return NULL; }
    D3DLOCKED_RECT lr;
    if (x->lpVtbl->LockRect(x, 0, &lr, NULL, 0) != D3D_OK) { x->lpVtbl->Release(x); t->built[who] = 1; return NULL; }
    look_recolor_dxt5(who, t->id, t->w, t->h, t->dxt5, (ce_u8 *)lr.pBits, lr.Pitch);
    x->lpVtbl->UnlockRect(x, 0);
    t->tex[who] = x; t->built[who] = 1;
    return x;
}
static void mat_identity(D3DMATRIX *m) { memset(m, 0, sizeof *m); m->_11 = m->_22 = m->_33 = m->_44 = 1.0f; }
void ce_preview_draw(IDirect3DDevice8 *dev, int who, int px, int py, int pw, int ph, float yaw)
{
    if (!dev || s_loaded != 1 || pw < 8 || ph < 8) return;
    { static IDirect3DDevice8 *last; if (last != dev) { if (last) ce_log("look preview: new Direct3D device - textures remade"); last = dev; for (int i = 0; i < s_ntex; i++) for (int w = 0; w < 2; w++) { s_tex[i].tex[w] = NULL; s_tex[i].built[w] = 0; } } }
    { static int seen[2]; if (seen[who] != ce_gen[who]) { seen[who] = ce_gen[who]; for (int i = 0; i < s_ntex; i++) s_tex[i].built[who] = 0; } }
    /* light baked into the vertex colours: a key light from the upper front-left, soft fill from behind */
    float cy = cosf(yaw), sy = sinf(yaw);
    float lx = -0.45f, ly = 0.55f, lz = -0.70f;                           /* world space (camera looks down +Z) */
    float lmx = lx * cy - lz * sy, lmz = lx * sy + lz * cy;                 /* into model space (inverse of the Y rotation) */
    for (int i = 0; i < s_nv; i++) {
        float d = s_nrm[i * 3] * lmx + s_nrm[i * 3 + 1] * ly + s_nrm[i * 3 + 2] * lmz;
        float b = 0.58f + 0.55f * (d > 0 ? d : 0) + 0.16f * (d < 0 ? -d : 0);
        int c = (int)(b * 255.0f); if (c > 255) c = 255;
        s_pv[i].color = 0xFF000000u | (c << 16) | (c << 8) | c;
    }
    /* save what is changed */
    D3DVIEWPORT8 vp_old; dev->lpVtbl->GetViewport(dev, &vp_old);
    D3DMATRIX w_old, v_old, p_old; dev->lpVtbl->GetTransform(dev, D3DTS_WORLD, &w_old); dev->lpVtbl->GetTransform(dev, D3DTS_VIEW, &v_old); dev->lpVtbl->GetTransform(dev, D3DTS_PROJECTION, &p_old);
    static const D3DRENDERSTATETYPE rs[] = { D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_ZFUNC, D3DRS_CULLMODE, D3DRS_LIGHTING, D3DRS_ALPHABLENDENABLE,
        D3DRS_ALPHATESTENABLE, D3DRS_ALPHAREF, D3DRS_ALPHAFUNC, D3DRS_FOGENABLE, D3DRS_STENCILENABLE, D3DRS_SPECULARENABLE, D3DRS_COLORVERTEX, D3DRS_FILLMODE, D3DRS_SHADEMODE, D3DRS_CLIPPING };
    DWORD rsv[sizeof rs / sizeof rs[0]];
    for (unsigned i = 0; i < sizeof rs / sizeof rs[0]; i++) dev->lpVtbl->GetRenderState(dev, rs[i], &rsv[i]);
    static const D3DTEXTURESTAGESTATETYPE ts[] = { D3DTSS_COLOROP, D3DTSS_COLORARG1, D3DTSS_COLORARG2, D3DTSS_ALPHAOP, D3DTSS_ALPHAARG1, D3DTSS_MINFILTER, D3DTSS_MAGFILTER, D3DTSS_MIPFILTER, D3DTSS_ADDRESSU, D3DTSS_ADDRESSV, D3DTSS_TEXCOORDINDEX, D3DTSS_TEXTURETRANSFORMFLAGS };
    DWORD tsv[sizeof ts / sizeof ts[0]], ts1op;
    for (unsigned i = 0; i < sizeof ts / sizeof ts[0]; i++) dev->lpVtbl->GetTextureStageState(dev, 0, ts[i], &tsv[i]);
    dev->lpVtbl->GetTextureStageState(dev, 1, D3DTSS_COLOROP, &ts1op);
    DWORD vs_old = 0, ps_old = 0; dev->lpVtbl->GetVertexShader(dev, &vs_old); dev->lpVtbl->GetPixelShader(dev, &ps_old);
    IDirect3DBaseTexture8 *tex_old = NULL; dev->lpVtbl->GetTexture(dev, 0, &tex_old);

    D3DVIEWPORT8 vp = { (DWORD)px, (DWORD)py, (DWORD)pw, (DWORD)ph, 0.0f, 1.0f };
    dev->lpVtbl->SetViewport(dev, &vp);
    s_clear_hr = dev->lpVtbl->Clear(dev, 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    D3DMATRIX W, V, P; mat_identity(&W);
    W._11 = cy; W._13 = -sy; W._31 = sy; W._33 = cy;                        /* rotation about Y */
    float eye[3] = { 0.0f, 465.0f, -2250.0f }, at[3] = { 0.0f, 465.0f, 0.0f };
    float zx = at[0] - eye[0], zy = at[1] - eye[1], zz = at[2] - eye[2]; float zl = sqrtf(zx * zx + zy * zy + zz * zz); zx /= zl; zy /= zl; zz /= zl;
    float xx = zz, xy = 0.0f, xz = -zx;                                     /* cross(up=(0,1,0), z) */
    float xl = sqrtf(xx * xx + xz * xz); xx /= xl; xz /= xl;
    float yx = zy * xz - zz * xy, yy = zz * xx - zx * xz, yz = zx * xy - zy * xx;  /* cross(z, x) */
    mat_identity(&V);
    V._11 = xx; V._12 = yx; V._13 = zx; V._21 = xy; V._22 = yy; V._23 = zy; V._31 = xz; V._32 = yz; V._33 = zz;
    V._41 = -(xx * eye[0] + xy * eye[1] + xz * eye[2]); V._42 = -(yx * eye[0] + yy * eye[1] + yz * eye[2]); V._43 = -(zx * eye[0] + zy * eye[1] + zz * eye[2]);
    float fov = 24.0f * 3.14159265f / 180.0f, aspect = (float)pw / (float)ph, zn = 100.0f, zf = 8000.0f;
    float ys = 1.0f / tanf(fov * 0.5f), xs = ys / aspect;
    memset(&P, 0, sizeof P); P._11 = xs; P._22 = ys; P._33 = zf / (zf - zn); P._34 = 1.0f; P._43 = -zn * zf / (zf - zn);
    dev->lpVtbl->SetTransform(dev, D3DTS_WORLD, &W); dev->lpVtbl->SetTransform(dev, D3DTS_VIEW, &V); dev->lpVtbl->SetTransform(dev, D3DTS_PROJECTION, &P);
    dev->lpVtbl->SetRenderState(dev, D3DRS_ZENABLE, D3DZB_TRUE); dev->lpVtbl->SetRenderState(dev, D3DRS_ZWRITEENABLE, TRUE); dev->lpVtbl->SetRenderState(dev, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    dev->lpVtbl->SetRenderState(dev, D3DRS_CULLMODE, D3DCULL_NONE); dev->lpVtbl->SetRenderState(dev, D3DRS_LIGHTING, FALSE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHABLENDENABLE, FALSE); dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHATESTENABLE, TRUE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAREF, 0x60); dev->lpVtbl->SetRenderState(dev, D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    dev->lpVtbl->SetRenderState(dev, D3DRS_FOGENABLE, FALSE); dev->lpVtbl->SetRenderState(dev, D3DRS_STENCILENABLE, FALSE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_SPECULARENABLE, FALSE); dev->lpVtbl->SetRenderState(dev, D3DRS_COLORVERTEX, TRUE);
    dev->lpVtbl->SetRenderState(dev, D3DRS_FILLMODE, D3DFILL_SOLID); dev->lpVtbl->SetRenderState(dev, D3DRS_SHADEMODE, D3DSHADE_GOURAUD); dev->lpVtbl->SetRenderState(dev, D3DRS_CLIPPING, TRUE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLOROP, D3DTOP_MODULATE); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_TEXCOORDINDEX, 0); dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->lpVtbl->SetPixelShader(dev, 0); dev->lpVtbl->SetVertexShader(dev, PV_FVF);
    for (int i = 0; i < s_nprim; i++) {
        Prim *pr = &s_prim[i];
        IDirect3DTexture8 *tx = pr->tex >= 0 ? preview_tex(dev, pr->tex, who) : NULL;
        dev->lpVtbl->SetTexture(dev, 0, (IDirect3DBaseTexture8 *)tx);
        if (pr->length >= 3) {
            HRESULT hr = dev->lpVtbl->DrawIndexedPrimitiveUP(dev, D3DPT_TRIANGLESTRIP, 0, s_nv, pr->length - 2, s_idx + pr->start, D3DFMT_INDEX16, s_pv, sizeof(PV));
            static int said; if (!said && i == s_nprim - 1) { said = 1; ce_log("look preview: first frame drawn: DrawIndexedPrimitiveUP %08lx, viewport %d,%d %dx%d, texture %p, z-clear %08lx", (unsigned long)hr, px, py, pw, ph, (void *)tx, (unsigned long)s_clear_hr); }
        }
    }
    /* put everything back */
    dev->lpVtbl->SetTexture(dev, 0, tex_old); if (tex_old) tex_old->lpVtbl->Release(tex_old);
    dev->lpVtbl->SetVertexShader(dev, vs_old); dev->lpVtbl->SetPixelShader(dev, ps_old);
    for (unsigned i = 0; i < sizeof ts / sizeof ts[0]; i++) dev->lpVtbl->SetTextureStageState(dev, 0, ts[i], tsv[i]);
    dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_COLOROP, ts1op);
    for (unsigned i = 0; i < sizeof rs / sizeof rs[0]; i++) dev->lpVtbl->SetRenderState(dev, rs[i], rsv[i]);
    dev->lpVtbl->SetTransform(dev, D3DTS_WORLD, &w_old); dev->lpVtbl->SetTransform(dev, D3DTS_VIEW, &v_old); dev->lpVtbl->SetTransform(dev, D3DTS_PROJECTION, &p_old);
    dev->lpVtbl->SetViewport(dev, &vp_old);
}
