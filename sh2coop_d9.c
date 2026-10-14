/*
 * sh2coop_d9.c  --  the Direct3D 9 side of the co-op mod's split-screen (P33; trimmed in P33c).
 *
 * The game talks Direct3D 8 to the Enhanced Edition's d3d8.dll (EE wrapper -> crosire d3d8to9 -> EE's ReShade d3d9
 * wrapper -> d3d9).  d3d8.h and d3d9.h cannot share one source file, so everything that needs the d3d9 device lives
 * here, behind plain C functions.
 *
 * 1. Viewport keeper.  Each split view is drawn with a D3D viewport for that player's part of the screen.  D3D9 resets
 *    the viewport to the full target on every SetRenderTarget, and the Enhanced Edition switches render targets in the
 *    middle of the world drawing when "EnableSoftShadows" is on (Xbox self-shadow silhouette pass and DrawSoftShadows,
 *    EE 3.0.2218 Wrappers/d3d8/IDirect3DDevice8.cpp: DrawPrimitive :1708-1798, DrawSoftShadows :3472-3699; both get the
 *    current target, draw into their own textures and switch back).  The d3d9 device's SetRenderTarget (vtable slot 37)
 *    is hooked: whenever the view's own target is set again while a view is being drawn, the view's viewport is put
 *    back at once - so the shadow volumes, the stencil clear and the final shadow quad after the switch stay inside
 *    the view, exactly as they stay inside the full screen in a normal frame.
 * 2. Depth bias straight on the d3d9 device (see d9_get_rs).
 * P33c: the diagonal layout is gone, and with it the Reset hook (it freed the diagonal layout's own render target
 * before a device Reset) and the composite; this file no longer creates any Direct3D object.
 */
#include <windows.h>
#include <d3d9.h>
#include <stddef.h>
#include <string.h>

typedef void (*LogFn)(const char *fmt, ...);

static IDirect3DDevice9 *s_d9;
static LogFn s_log;
typedef HRESULT (STDMETHODCALLTYPE *SrtFn)(IDirect3DDevice9 *, DWORD, IDirect3DSurface9 *);
static SrtFn s_srt_orig;
static volatile int s_vp_on;
static D3DVIEWPORT9 s_vp;
static IDirect3DSurface9 *s_vp_rt;          /* the view's target (pointer compare only; no reference kept) */
static volatile unsigned s_reapplied;

static HRESULT STDMETHODCALLTYPE srt_hook(IDirect3DDevice9 *d, DWORD idx, IDirect3DSurface9 *rt)
{
    HRESULT hr = s_srt_orig(d, idx, rt);
    if (s_vp_on && idx == 0 && rt && rt == s_vp_rt && SUCCEEDED(hr)) {
        d->lpVtbl->SetViewport(d, &s_vp);
        s_reapplied++;
    }
    return hr;
}

static int patch_slot(void **vt, int slot, void *hook, void **orig)
{
    DWORD old;
    if (vt[slot] == hook) return 1;                 /* already ours (same class vtable) */
    if (!VirtualProtect(&vt[slot], sizeof(void *), PAGE_EXECUTE_READWRITE, &old)) return 0;
    *orig = vt[slot];
    vt[slot] = hook;
    VirtualProtect(&vt[slot], sizeof(void *), old, &old);
    return 1;
}

static void **s_vt;                                 /* the one d3d9 device class whose vtable carries the hook */

/* dev8 = the game's IDirect3DDevice8 (the Enhanced Edition's wrapper).  Returns 1 when the d3d9 device was found and
   hooked.  QueryInterface(IID_IDirect3DDevice9) is forwarded EE wrapper -> d3d8to9 (keeps the IID) -> the d3d9 device
   d3d8to9 draws with (EE's ReShade wrapper m_IDirect3DDevice9 when its custom shaders are on - e.g. with
   RestoreBrightnessSelector - else d3d9 itself), which returns itself. */
int d9_attach(void *dev8, LogFn log)
{
    IUnknown *u = (IUnknown *)dev8;
    IDirect3DDevice9 *d = NULL;
    s_log = log;
    s_d9 = NULL; s_vp_on = 0;
    if (!u) return 0;
    if (FAILED(u->lpVtbl->QueryInterface(u, &IID_IDirect3DDevice9, (void **)&d)) || !d) {
        if (s_log) s_log("split: no Direct3D 9 device behind the game's device - soft-shadow viewport keeper off");
        return 0;
    }
    d->lpVtbl->Release(d);                          /* no reference kept: the game's device holds it (re-attached if it changes) */
    void **vt = *(void ***)d;
    char mod[MAX_PATH] = "?"; HMODULE hm = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)vt, &hm) && hm)
        GetModuleFileNameA(hm, mod, sizeof mod);
    const char *base = strrchr(mod, '\\'); base = base ? base + 1 : mod;
    if (s_vt && vt != s_vt) {                       /* a different device class: its original would differ - leave it alone */
        if (s_log) s_log("split: Direct3D 9 device %p is of another class (vtable %p in %s) - soft-shadow viewport keeper off for it", (void *)d, (void *)vt, base);
        return 0;
    }
    int a = patch_slot(vt, offsetof(IDirect3DDevice9Vtbl, SetRenderTarget) / sizeof(void *), (void *)srt_hook, (void **)&s_srt_orig);
    s_vt = vt;
    if (s_log) s_log("split: Direct3D 9 device %p (vtable %p in %s): SetRenderTarget hook %s", (void *)d, (void *)vt, base, a ? "in" : "FAILED");
    if (!a) return 0;
    s_d9 = d;
    return 1;
}

/* a render state straight on the d3d9 device (used for D3DRS_DEPTHBIAS: going through Direct3D 8's ZBIAS, the
   Enhanced Edition's FogLayerFix scales every set by 15/16 and d3d8to9 truncates on the way back, so a save /
   restore through the game's interface would shrink a non-zero bias) */
int d9_get_rs(unsigned state, unsigned *v)
{
    DWORD x = 0;
    if (!s_d9 || FAILED(s_d9->lpVtbl->GetRenderState(s_d9, (D3DRENDERSTATETYPE)state, &x))) return 0;
    *v = x; return 1;
}
void d9_set_rs(unsigned state, unsigned v) { if (s_d9) s_d9->lpVtbl->SetRenderState(s_d9, (D3DRENDERSTATETYPE)state, v); }

/* a view starts: the current render target is the view's target; its viewport is kept on it */
void d9_view_begin(int x, int y, int w, int h)
{
    IDirect3DSurface9 *rt = NULL;
    if (!s_d9) return;
    s_vp.X = (DWORD)x; s_vp.Y = (DWORD)y; s_vp.Width = (DWORD)w; s_vp.Height = (DWORD)h; s_vp.MinZ = 0.0f; s_vp.MaxZ = 1.0f;
    s_vp_rt = NULL;
    if (SUCCEEDED(s_d9->lpVtbl->GetRenderTarget(s_d9, 0, &rt)) && rt) { s_vp_rt = rt; rt->lpVtbl->Release(rt); }
    s_vp_on = s_vp_rt != NULL;
}
void d9_view_end(void) { s_vp_on = 0; s_vp_rt = NULL; }
unsigned d9_reapplied(void) { return s_reapplied; }
