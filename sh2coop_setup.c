/* sh2coop_setup.c - installer + checker of the Silent Hill 2 (PC, Enhanced Edition) two-player co-op mod (sh2coop.asi).
 *
 * One source, two programs:
 *   Install co-op.exe   (default build)      finds the game folder (its own folder, the folder dropped on it, or a folder
 *                                            picker), puts the mod's files in, sets LoadPlugins = 1 in d3d8.ini (backup
 *                                            kept), then runs the checks below and ends with READY.  It only ever adds the
 *                                            mod's own files and changes that one setting; other mods are listed, not touched.
 *                                            -uninstall removes the mod's own files again and puts the setting back.
 *   sh2coop_check.exe   (-DCHECK_ONLY)       the read-only checker: looks at the files the mod depends on and says what, if
 *                                            anything, to change by hand.  The only file it writes is its report.
 *
 * What it checks
 *   1. sh2pc.exe is the Enhanced Edition exe the mod was made for (SHA-1 of the file).
 *   2. d3d8.dll is the Enhanced Edition's "SH2 Enhancements" module (it is what loads .asi plugins).
 *   3. d3d8.ini has LoadPlugins = 1 (the Enhanced Edition ships with plugins turned OFF) and does not restrict
 *      plugins to the scripts\ folder while the mod sits in the game folder.
 *   4. sh2coop.asi (and sh2coop.ini) are in place, and which build they are.
 *   5. Other .asi plugins (game folder, scripts\, plugins\) and DLLs the Enhanced Edition does not ship - these
 *      come from other mods and are the only kind of mod that can really conflict with the co-op.
 *   6. What the mod's own log said the last time the game ran (hooks installed or not).
 *
 * Build (MinGW):  i686-w64-mingw32-gcc -O2 -Wall -s -mconsole -static-libgcc -o "Install co-op.exe" sh2coop_setup.c -ladvapi32 -lole32 -lshell32
 *                 i686-w64-mingw32-gcc -O2 -Wall -s -mconsole -static-libgcc -DCHECK_ONLY -o sh2coop_check.exe sh2coop_setup.c -ladvapi32
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- the files the mod was made and tested with ---- */
#define EXE_SHA1       "3201a5e1029f3ae3b77770255dcbe10360b3cd09"   /* Enhanced Edition exe 1.0 (4GB-aware), 5,685,248 bytes */
#define EXE_SIZE       5685248u
#define EXE_2002_SIZE  6180307u                                     /* the untouched 2002 retail exe (SafeDisc), for a better hint */
#define MODULE_SHA1    "185111f8ada6c76a10ddd52fa3d67e623953f85c"   /* SH2 Enhancements module 3.0.2218.0 */
#define MODULE_VER     "3.0.2218.0"
#define XIDI_SHA1      "e25d5ef214ca77cb77a8dd16ffa079c985127a67"   /* the EE's Xidi dinput8.dll (Xidi SDL3 1.0.0 package) */
/* The mod's build name is read from sh2coop.asi itself (its start-up log line "sh2coop PC build Pxx loaded" is a string
   inside the file), so this checker does not need to be rebuilt when a new mod build is dropped into the package. */

static wchar_t g_dir[MAX_PATH];        /* game folder, with a trailing backslash */
static FILE   *g_rep;                  /* the report file */
static int     g_fail, g_warn;

static void out(const char *fmt, ...)
{
    va_list ap; char line[2048];
    va_start(ap, fmt); _vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    line[sizeof line - 1] = 0;
    fputs(line, stdout); fputs("\n", stdout); fflush(stdout);
    if (g_rep) { fputs(line, g_rep); fputs("\n", g_rep); fflush(g_rep); }
}
static void fail(const char *fmt, ...)
{
    va_list ap; char line[2048];
    va_start(ap, fmt); _vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    line[sizeof line - 1] = 0; g_fail++; out("  [FIX]  %s", line);
}
static void warn(const char *fmt, ...)
{
    va_list ap; char line[2048];
    va_start(ap, fmt); _vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    line[sizeof line - 1] = 0; g_warn++; out("  [NOTE] %s", line);
}
static void ok(const char *fmt, ...)
{
    va_list ap; char line[2048];
    va_start(ap, fmt); _vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    line[sizeof line - 1] = 0; out("  [ok]   %s", line);
}

/* ---- small helpers ---- */
static const char *narrow(const wchar_t *w)            /* for printing only */
{
    static char buf[4][1024]; static int k;
    char *b = buf[k++ & 3];
    if (!WideCharToMultiByte(CP_ACP, 0, w, -1, b, 1024, NULL, NULL)) strcpy(b, "?");
    return b;
}
static void join(wchar_t *dst, const wchar_t *rel)
{
    wcscpy(dst, g_dir); wcsncat(dst, rel, MAX_PATH - wcslen(dst) - 1);
}
static int file_size(const wchar_t *rel, unsigned *size)
{
    wchar_t p[MAX_PATH]; WIN32_FILE_ATTRIBUTE_DATA a;
    join(p, rel);
    if (!GetFileAttributesExW(p, GetFileExInfoStandard, &a) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return 0;
    if (size) *size = a.nFileSizeHigh ? 0xFFFFFFFFu : a.nFileSizeLow;
    return 1;
}
static int dir_exists(const wchar_t *rel)
{
    wchar_t p[MAX_PATH]; DWORD a;
    join(p, rel); a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}
/* SHA-1 of a file through the Windows crypto API (read only, 1 MB at a time) */
static int sha1_file(const wchar_t *rel, char hex[41])
{
    wchar_t p[MAX_PATH]; HANDLE f; HCRYPTPROV prov = 0; HCRYPTHASH h = 0; static unsigned char buf[1 << 20];
    DWORD n, len = 20; unsigned char dig[20]; int res = 0;
    join(p, rel); hex[0] = 0;
    f = CreateFileW(p, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    if (CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) && CryptCreateHash(prov, CALG_SHA1, 0, 0, &h)) {
        res = 1;
        while (ReadFile(f, buf, sizeof buf, &n, NULL) && n) if (!CryptHashData(h, buf, n, 0)) { res = 0; break; }
        if (res && CryptGetHashParam(h, HP_HASHVAL, dig, &len, 0)) { for (int i = 0; i < 20; i++) sprintf(hex + 2 * i, "%02x", dig[i]); }
        else res = 0;
    }
    if (h) CryptDestroyHash(h);
    if (prov) CryptReleaseContext(prov, 0);
    CloseHandle(f);
    return res;
}
/* read a whole file into a malloc'd buffer (max 64 MB) */
static unsigned char *read_file(const wchar_t *rel, unsigned *n_out)
{
    wchar_t p[MAX_PATH]; HANDLE f; DWORD n, got; unsigned char *b;
    join(p, rel);
    f = CreateFileW(p, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return NULL;
    n = GetFileSize(f, NULL); if (n == INVALID_FILE_SIZE || n > (64u << 20)) { CloseHandle(f); return NULL; }
    b = malloc(n + 2);
    if (!b || !ReadFile(f, b, n, &got, NULL)) { CloseHandle(f); free(b); return NULL; }
    b[got] = b[got + 1] = 0; CloseHandle(f);
    *n_out = got;
    return b;
}
static unsigned rd32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }
static unsigned rd16(const unsigned char *p) { return p[0] | p[1] << 8; }
/* a UTF-16 string value that follows the key `key` inside a version block (key, NUL, pad to 4, value) */
static void ver_value(const unsigned char *blk, unsigned n, unsigned base, const char *key, char *dst, int nd)
{
    int kl = (int)strlen(key);
    dst[0] = 0;
    for (unsigned i = 0; i + 2 * kl + 2 <= n; i += 2) {
        int k = 0;
        while (k < kl && blk[i + 2 * k] == (unsigned char)key[k] && blk[i + 2 * k + 1] == 0) k++;
        if (k < kl || blk[i + 2 * kl] || blk[i + 2 * kl + 1]) continue;
        unsigned v = i + 2 * kl + 2; v = (v + base + 3) / 4 * 4 - base;     /* 4-byte alignment counted from the block start */
        int d = 0;
        while (v + 1 < n && (blk[v] || blk[v + 1]) && d < nd - 1) { dst[d++] = blk[v + 1] ? '?' : (char)blk[v]; v += 2; }
        dst[d] = 0;
        return;
    }
}
/* PE resources, read straight from the file (no LoadLibrary / version API: under Wine those answer for Wine's own
   built-in d3d8/dinput8 instead of the file asked about).  pe_resource() returns the k-th resource of the given type
   (RT_VERSION = 16, RT_RCDATA = 10) as a pointer into the file image, or NULL. */
static const unsigned char *pe_resource(const unsigned char *b, unsigned n, unsigned type, unsigned k, unsigned *size)
{
    unsigned pe, nsec, opt, rrva, i, roff = 0, rsize = 0; const unsigned char *sec, *dir, *e;
    if (n < 0x40 || b[0] != 'M' || b[1] != 'Z') return NULL;
    pe = rd32(b + 0x3C); if (pe + 0xF8 > n || rd32(b + pe) != 0x4550) return NULL;
    nsec = rd16(b + pe + 6); opt = rd16(b + pe + 20);
    if (rd16(b + pe + 24) != 0x10B) return NULL;                             /* PE32 only (the game is 32-bit) */
    rrva = rd32(b + pe + 24 + 112);                                          /* data directory 2: resources */
    sec = b + pe + 24 + opt;
    if (!rrva || sec + nsec * 40 > b + n) return NULL;
    for (i = 0; i < nsec; i++) {                                             /* the section that holds the resource RVA */
        unsigned va = rd32(sec + i * 40 + 12), vsz = rd32(sec + i * 40 + 8), raw = rd32(sec + i * 40 + 20), rsz = rd32(sec + i * 40 + 16);
        if (rrva >= va && rrva < va + (vsz > rsz ? vsz : rsz)) { roff = raw + (rrva - va); rsize = rsz - (rrva - va); break; }
    }
    if (!roff || roff + 16 > n) return NULL;
    dir = b + roff;
    {   /* level 1: the type; level 2: the k-th name/id; level 3: first language -> data entry */
        unsigned ne = rd16(dir + 12) + rd16(dir + 14), sub = 0, j, n2;
        for (j = 0; j < ne && (dir + 16 + j * 8 + 8 <= b + n); j++) {
            e = dir + 16 + j * 8;
            if (rd32(e) == type && (rd32(e + 4) & 0x80000000u)) { sub = rd32(e + 4) & 0x7FFFFFFFu; break; }
        }
        if (!sub || roff + sub + 16 > n) return NULL;
        dir = b + roff + sub; n2 = rd16(dir + 12) + rd16(dir + 14);
        if (k >= n2 || roff + sub + 16 + (k + 1) * 8 > n) return NULL;
        e = dir + 16 + k * 8;
        if (!(rd32(e + 4) & 0x80000000u)) return NULL;
        sub = rd32(e + 4) & 0x7FFFFFFFu; if (roff + sub + 24 > n) return NULL;
        e = b + roff + sub + 16;                                             /* first entry of level 3 -> data entry */
        if (rd32(e + 4) & 0x80000000u) return NULL;
        sub = rd32(e + 4); if (roff + sub + 16 > n) return NULL;
        {
            unsigned drva = rd32(b + roff + sub), dsz = rd32(b + roff + sub + 4), doff;
            if (drva < rrva || drva - rrva + dsz > rsize) return NULL;
            doff = roff + (drva - rrva);
            if (doff + dsz > n) return NULL;
            *size = dsz;
            return b + doff;
        }
    }
}
/* the file's own version resource: InternalName + FileVersion (empty when it has none) */
static void ver_info(const wchar_t *rel, char *name, int nn, char *ver, int nv)
{
    unsigned n, dsz; unsigned char *b = read_file(rel, &n); const unsigned char *v;
    name[0] = ver[0] = 0;
    if (!b) return;
    v = pe_resource(b, n, 16, 0, &dsz);
    if (v && dsz <= 65536) { ver_value(v, dsz, 0, "InternalName", name, nn); ver_value(v, dsz, 0, "FileVersion", ver, nv); }
    free(b);
}
/* does the file contain this ASCII text as UTF-16 (the way version resources store it)?  Fallback for ver_info. */
static int contains_utf16(const wchar_t *rel, const char *text)
{
    wchar_t p[MAX_PATH]; HANDLE f; static unsigned char buf[1 << 20]; DWORD n; int tl = (int)strlen(text), found = 0; unsigned carry = 0;
    static unsigned char prev[256];
    join(p, rel);
    f = CreateFileW(p, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    while (!found && ReadFile(f, buf + carry, sizeof buf - carry, &n, NULL) && n) {
        unsigned total = carry + n;
        for (unsigned i = 0; i + 2 * tl <= total && !found; i++) {
            int k = 0;
            while (k < tl && buf[i + 2 * k] == (unsigned char)text[k] && buf[i + 2 * k + 1] == 0) k++;
            if (k == tl) found = 1;
        }
        carry = total < 2u * tl ? total : 2u * tl;          /* keep a tail so a match on a chunk border is not missed */
        memcpy(prev, buf + total - carry, carry); memcpy(buf, prev, carry);
    }
    CloseHandle(f);
    return found;
}
/* read a small text file into a malloc'd, NUL-terminated buffer (max 4 MB) */
static char *read_text(const wchar_t *rel, unsigned *n_out)
{
    wchar_t p[MAX_PATH]; HANDLE f; DWORD n = 0, got; char *b;
    join(p, rel);
    f = CreateFileW(p, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return NULL;
    n = GetFileSize(f, NULL); if (n == INVALID_FILE_SIZE || n > (4u << 20)) { CloseHandle(f); return NULL; }
    b = malloc(n + 1);
    if (!b || !ReadFile(f, b, n, &got, NULL)) { CloseHandle(f); free(b); return NULL; }
    b[got] = 0; CloseHandle(f);
    if (n_out) *n_out = got;
    return b;
}
/* the mod's build name, read from the asi file: the text between "sh2coop PC build " and " loaded" */
static int asi_build_name(const wchar_t *rel, char *name, int n)
{
    unsigned len; char *t = read_text(rel, &len), *p = NULL; static const char key[] = "sh2coop PC build ";
    name[0] = 0;
    if (!t) return 0;
    for (unsigned i = 0; i + sizeof key <= len; i++) if (t[i] == 's' && !memcmp(t + i, key, sizeof key - 1)) { p = t + i + sizeof key - 1; break; }
    if (p) { int k = 0; while (p[k] && p[k] != ' ' && p[k] != '(' && k < n - 1) { name[k] = p[k]; k++; } name[k] = 0; }
    free(t);
    return name[0] != 0;
}
static char *trim(char *s)
{
    char *e;
    while (*s == ' ' || *s == '\t') s++;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}
static int ieq(const char *a, const char *b) { return _stricmp(a, b) == 0; }

/* ---- the checks ---- */
static int check_exe(void)
{
    unsigned sz; char hex[41];
    out("");
    out("1. sh2pc.exe");
    if (!file_size(L"sh2pc.exe", &sz)) { fail("sh2pc.exe is not in this folder - run this checker from the Silent Hill 2 game folder."); return 0; }
    if (!sha1_file(L"sh2pc.exe", hex)) { fail("could not read sh2pc.exe (is the game running, or is the file blocked?)"); return 0; }
    if (!strcmp(hex, EXE_SHA1)) { ok("Enhanced Edition exe 1.0 - exactly the exe the mod was made for (%u bytes, sha1 %.12s...)", sz, hex); return 1; }
    if (sz == EXE_2002_SIZE)
        fail("sh2pc.exe is the untouched 2002 retail exe (%u bytes). The Enhanced Edition setup (SH2EEsetup.exe) has not replaced it yet: "
             "run the setup and let it install its 'Enhanced EXE'. The co-op mod does nothing on this exe (its log will say so).", sz);
    else
        fail("sh2pc.exe is not the Enhanced Edition 1.0 exe the mod was made for (%u bytes, sha1 %s). Another patch or mod has changed it, or it is a "
             "different version. The co-op mod checks the exe at start-up and does nothing if it differs. Re-run SH2EEsetup.exe and let it put "
             "its 'Enhanced EXE' back.", sz, hex);
    return 0;
}
static int check_module(void)
{
    unsigned sz; char hex[41], name[128], ver[64];
    out("");
    out("2. d3d8.dll (the Enhanced Edition module - it is what loads .asi plugins such as the co-op mod)");
    if (!file_size(L"d3d8.dll", &sz)) { fail("d3d8.dll is missing: the Enhanced Edition is not installed in this folder (run SH2EEsetup.exe). Without it no .asi plugin is loaded."); return 0; }
    ver_info(L"d3d8.dll", name, sizeof name, ver, sizeof ver);
    sha1_file(L"d3d8.dll", hex);
    int is_ee = ieq(name, "sh2-enhce") || contains_utf16(L"d3d8.dll", "sh2-enhce");
    if (!strcmp(hex, MODULE_SHA1)) { ok("SH2 Enhancements module %s - the version the mod was tested with", MODULE_VER); return 1; }
    if (is_ee && !strcmp(ver, MODULE_VER)) { warn("SH2 Enhancements module %s, but the file is not byte-for-byte the one the mod was tested with (sha1 %.12s...). Probably fine; mention it if the game misbehaves.", ver, hex); return 1; }
    if (is_ee) { warn("SH2 Enhancements module, version %s (the mod was tested with %s). It should still work; if the game misbehaves, mention this version.", ver[0] ? ver : "unknown", MODULE_VER); return 1; }
    fail("d3d8.dll is not the Enhanced Edition module (it calls itself \"%s\"%s%s, %u bytes). Another wrapper (dgVoodoo, a plain d3d8to9, ...) has replaced it, "
         "so the game would not load any .asi plugin. Re-run SH2EEsetup.exe so it puts the SH2 Enhancements module back.",
         name[0] ? name : "", ver[0] ? " version " : "", ver[0] ? ver : "", sz);
    return 0;
}
static void check_components(void)
{
    char *t = read_text(L"SH2EEsetup.dat", NULL), line[512] = "", *p, *nl; int n = 0;
    if (!t) { warn("SH2EEsetup.dat not found - the Enhanced Edition setup has not been run in this folder (or the file was removed)."); return; }
    for (p = t; *p; p = nl ? nl + 1 : p + strlen(p)) {
        char *id, *inst, *ver, *c;
        nl = strchr(p, '\n'); if (nl) *nl = 0;
        c = trim(p); if (*c == '#' || !*c || !strncmp(c, "id,", 3)) continue;
        id = c; inst = strchr(id, ','); if (!inst) continue; *inst++ = 0;
        ver = strchr(inst, ','); if (!ver) continue; *ver++ = 0;
        if (ieq(trim(inst), "true")) { char one[96]; _snprintf(one, sizeof one, "%s%s %s", n ? ", " : "", trim(id), trim(ver)); one[sizeof one - 1] = 0; strncat(line, one, sizeof line - strlen(line) - 1); n++; }
    }
    free(t);
    if (n) ok("Enhanced Edition components installed: %s", line); else warn("SH2EEsetup.dat lists no installed components.");
    if (!file_size(L"sh2e\\pic\\etc\\start01e.tex", NULL) || !file_size(L"sh2e\\pic\\etc\\start00.tex", NULL))
        warn("the Enhanced Edition's title screen files (sh2e\\pic\\etc\\start00.tex / start01e.tex) are not here: the co-op title screen "
             "(CO-OP under the logo) will stay the original title. Everything else works. They come with the EE 'Essential Files'.");
}
static int check_ini(int *scripts_only)
{
    char *t = read_text(L"d3d8.ini", NULL), *p, *nl; int ln = 0, in_plugins = 0, lp = -1, lp_line = 0, so = -1, so_line = 0, ld = -1, res = 1;
    *scripts_only = 0;
    out("");
    out("3. d3d8.ini (the Enhanced Edition's settings)");
    if (!t) { fail("d3d8.ini is missing. The Enhanced Edition writes it the first time the game starts: start the game once (or run Install co-op.exe, which creates it from d3d8.dll), then come back here."); return 0; }
    for (p = t; *p; p = nl ? nl + 1 : p + strlen(p)) {
        char *c, *eq;
        nl = strchr(p, '\n'); if (nl) *nl = 0; ln++;
        c = trim(p);
        if (*c == ';' || *c == '#' || !*c) continue;
        if (*c == '[') { in_plugins = ieq(c, "[Plugins]"); continue; }
        if (!in_plugins) continue;
        eq = strchr(c, '='); if (!eq) continue;
        *eq = 0; { char *k = trim(c), *v = trim(eq + 1);
          if (ieq(k, "LoadPlugins")) { lp = atoi(v); lp_line = ln; }
          else if (ieq(k, "LoadFromScriptsOnly")) { so = atoi(v); so_line = ln; }
          else if (ieq(k, "LoadD3d8FromScriptsFolder")) ld = atoi(v); }
    }
    free(t);
    if (lp == 1) ok("LoadPlugins = 1 (line %d): .asi plugins are loaded", lp_line);
    else if (lp == 0) { fail("LoadPlugins = 0 (line %d). The Enhanced Edition ships with plugins OFF, so the co-op mod would not be loaded at all. "
                             "Open d3d8.ini in Notepad, find the [Plugins] section and change the line to:  LoadPlugins = 1", lp_line); res = 0; }
    else { fail("no LoadPlugins line found in the [Plugins] section of d3d8.ini. Add  LoadPlugins = 1  under [Plugins] (or re-run SH2EEsetup.exe to get a fresh d3d8.ini, then set it)."); res = 0; }
    if (so == 1) { *scripts_only = 1; warn("LoadFromScriptsOnly = 1 (line %d): plugins are only loaded from the scripts\\ and plugins\\ folders, not from the game folder itself. "
                                           "Either set it to 0, or keep sh2coop.asi + sh2coop.ini in the scripts\\ folder.", so_line); }
    else ok("LoadFromScriptsOnly = %d: plugins in the game folder are loaded", so < 0 ? 0 : so);
    if (ld == 1) warn("LoadD3d8FromScriptsFolder = 1: a third-party d3d8.dll is loaded from scripts\\ or plugins\\. The co-op mod was not tested with that.");
    return res;
}
static int check_mod(int scripts_only)
{
    static const wchar_t *where[] = { L"sh2coop.asi", L"scripts\\sh2coop.asi", L"plugins\\sh2coop.asi" };
    static const wchar_t *ini[]   = { L"sh2coop.ini", L"scripts\\sh2coop.ini", L"plugins\\sh2coop.ini" };
    int found = -1, res = 0; unsigned sz; char hex[41], build[64];
    out("");
    out("4. the co-op mod itself");
    for (int i = 0; i < 3 && found < 0; i++) if (file_size(where[i], &sz)) found = i;
    if (found < 0) {
        if (file_size(L"sh2coop.asi.off", NULL)) fail("sh2coop.asi.off found: the mod is here but switched off. Rename it back to sh2coop.asi to use it.");
        else fail("sh2coop.asi is not here. Copy sh2coop.asi and sh2coop.ini from the mod's zip into this folder (next to sh2pc.exe).");
        return 0;
    }
    sha1_file(where[found], hex);
    if (asi_build_name(where[found], build, sizeof build)) ok("%s: build %s (%u bytes, sha1 %.12s...)", narrow(where[found]), build, sz, hex);
    else { fail("%s does not look like the co-op mod (no build name inside it; %u bytes, sha1 %.12s...). Copy the sh2coop.asi from the mod's zip again.", narrow(where[found]), sz, hex); return 0; }
    res = 1;
    if (found == 0 && scripts_only) { fail("sh2coop.asi is in the game folder but d3d8.ini only loads plugins from scripts\\ (see above): it would not be loaded."); res = 0; }
    if (found > 0 && !scripts_only) ok("%s is in a plugin subfolder; the Enhanced Edition loads plugins from there too", narrow(where[found]));
    if (file_size(ini[found], NULL)) ok("%s is next to it", narrow(ini[found]));
    else warn("%s is missing - not a problem: the mod writes one with default settings (both players 'auto', split screen side by side) the first time the game starts.", narrow(ini[found]));
    if (file_size(L"sh2coop.asi.off", NULL) && found == 0) warn("there is also a sh2coop.asi.off here (an older switched-off copy?) - harmless, it is not loaded.");
    return res;
}
/* list files matching rel\pattern that are not in the keep list; returns how many were listed */
/* ---- other mods.  Policy (the user's, 13 Oct): the co-op is made for, and only supported on, a CLEAN Enhanced Edition
   install.  Anything that runs code inside the game - other .asi plugins, DLLs the Enhanced Edition does not ship, a
   third-party d3d8 from scripts\ - is a [FIX]: the installer refuses to install over it and the checker says NOT READY.
   Data mods (textures, models, sounds) cannot be told apart from the Enhanced Edition's own files reliably, so only
   their usual leftovers (ReShade files, a mods folder) are pointed out as [NOTE]. ---- */
static const char *dll_hint(const wchar_t *name)
{
    if (!_wcsicmp(name, L"d3d9.dll") || !_wcsicmp(name, L"dxgi.dll") || !_wcsicmp(name, L"d3d11.dll") || !_wcsicmp(name, L"opengl32.dll"))
        return "  <- a ReShade / overlay wrapper (the Enhanced Edition already has ReShade built in)";
    if (!_wcsicmp(name, L"d3d8.dll")) return "  <- a second Direct3D 8 wrapper (dgVoodoo, d3d8to9, ...)";
    if (!_wcsicmp(name, L"dsound.dll")) return "  <- a sound wrapper (DSOAL?)";
    if (!_wcsicmp(name, L"xinput1_3.dll") || !_wcsicmp(name, L"xinput1_4.dll") || !_wcsicmp(name, L"xinput9_1_0.dll")) return "  <- an XInput wrapper; the mod reads controllers through XInput";
    if (!_wcsicmp(name, L"version.dll") || !_wcsicmp(name, L"winmm.dll") || !_wcsicmp(name, L"dinput.dll") || !_wcsicmp(name, L"ddraw.dll") || !_wcsicmp(name, L"wininet.dll"))
        return "  <- a proxy DLL (often a second ASI loader)";
    return "";
}
static int list_files(const wchar_t *sub, const wchar_t *pattern, const wchar_t **keep, int nkeep, const char *what, int hint)
{
    wchar_t p[MAX_PATH]; WIN32_FIND_DATAW fd; HANDLE h; int n = 0;
    join(p, sub); wcsncat(p, pattern, MAX_PATH - wcslen(p) - 1);
    h = FindFirstFileW(p, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        int skip = 0;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (!_wcsnicmp(fd.cFileName, L"sh2coop", 7)) continue;                         /* this mod and its own files */
        for (int i = 0; i < nkeep; i++) if (_wcsicmp(fd.cFileName, keep[i]) == 0) skip = 1;
        if (skip) continue;
        if (!n) fail("%s", what);
        out("           %s%s  (%lu bytes)%s", narrow(sub), narrow(fd.cFileName), (unsigned long)fd.nFileSizeLow, hint ? dll_hint(fd.cFileName) : "");
        n++;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return n;
}
/* everything that runs code inside the game and is not the Enhanced Edition's: returns how many were found (and lists them) */
static int scan_code_mods(void)
{
    static const wchar_t *keep_none[] = { L"" };
    static const wchar_t *keep_dll[] = { L"d3d8.dll", L"dinput8.dll", L"SDL3.dll", L"SDL.XidiPlugin.32.dll", L"Xidi.32.dll", L"binkw32.dll", L"msvcp70.dll", L"msvcr70.dll" };
    int n = 0; char name[128], ver[64], hex[41];
    n += list_files(L"", L"*.asi", keep_none, 0, "other .asi plugins in the game folder:", 0);
    n += list_files(L"scripts\\", L"*.asi", keep_none, 0, "other .asi plugins in scripts\\:", 0);
    n += list_files(L"plugins\\", L"*.asi", keep_none, 0, "other .asi plugins in plugins\\:", 0);
    n += list_files(L"", L"*.dll", keep_dll, 8, "DLLs in the game folder that the Enhanced Edition does not ship:", 1);
    n += list_files(L"scripts\\", L"*.dll", keep_none, 0, "DLLs in scripts\\ (a third-party d3d8.dll or other wrapper):", 1);
    n += list_files(L"plugins\\", L"*.dll", keep_none, 0, "DLLs in plugins\\ (a third-party d3d8.dll or other wrapper):", 1);
    /* dinput8.dll: the mod opens the second controller through it, so it must be the EE's Xidi (or absent) */
    if (file_size(L"dinput8.dll", NULL)) {
        ver_info(L"dinput8.dll", name, sizeof name, ver, sizeof ver); sha1_file(L"dinput8.dll", hex);
        if (!strcmp(hex, XIDI_SHA1)) ok("dinput8.dll is the Enhanced Edition's Xidi (controller wrapper) - the mod opens the second controller through it");
        else if (ieq(name, "Xidi")) warn("dinput8.dll is Xidi %s, a different build than the Enhanced Edition package's. Should be fine.", ver[0] ? ver : "");
        else { fail("dinput8.dll is not the Enhanced Edition's Xidi%s%s%s%s: another input wrapper. The mod opens the second controller through this DLL.", name[0] || ver[0] ? " (" : "", name[0] ? name : "", ver[0] ? " " : "", name[0] || ver[0] ? (ver[0] ? ver : ")") : ""); n++; }
    } else warn("dinput8.dll (the Enhanced Edition's Xidi controller wrapper) is not here: the second controller is opened through Windows' own dinput8 instead. Usually fine.");
    return n;
}
/* the usual leftovers of data / overlay mods - pointed out, not blocking */
static void scan_data_mods(void)
{
    static const wchar_t *dirs[] = { L"reshade-shaders", L"ReShade", L"mods", L"Mods", L"dxvk.cache" };
    static const wchar_t *files[] = { L"ReShade.ini", L"ReShadePreset.ini", L"dxvk.conf", L"dgVoodoo.conf", L"dgVoodooCpl.exe" };
    int n = 0;
    for (int i = 0; i < (int)(sizeof dirs / sizeof *dirs); i++) if (dir_exists(dirs[i])) { if (!n) warn("leftovers of other mods / tools:"); out("           %s\\", narrow(dirs[i])); n++; }
    for (int i = 0; i < (int)(sizeof files / sizeof *files); i++) if (file_size(files[i], NULL)) { if (!n) warn("leftovers of other mods / tools:"); out("           %s", narrow(files[i])); n++; }
    if (n) out("         These do not run code by themselves, so they are not blocking - but the co-op is only supported on a clean Enhanced Edition install.");
}
static void check_others(void)
{
    int n;
    out("");
    out("5. other mods (the co-op is made for a CLEAN Enhanced Edition install and is only supported on one)");
    n = scan_code_mods();
    if (n) out("         Other plugins and wrapper DLLs run code inside the game and can break the co-op (or be broken by it). Move them out of\n"
               "         the game folder (or rename each .asi to .asi.off), start the game once to make sure it still runs, then come back here.");
    else ok("no other plugins or wrapper DLLs - a clean Enhanced Edition install");
    scan_data_mods();
    if (dir_exists(L"data\\save")) ok("saves: data\\save is where the game keeps them; the co-op shares them with the normal game and nothing here touches them");
}
static void check_log(void)
{
    char *t = read_text(L"sh2coop.log", NULL), *p, *nl; int ln = 0, hooks = 0, none = 0; char first[300] = "", hl[300] = "";
    out("");
    out("6. what the mod's log said the last time the game ran (sh2coop.log)");
    if (!t) { out("         no sh2coop.log yet - it appears after the first start with the mod. Look for 'hooks installed' in it."); return; }
    for (p = t; *p && ln < 400; p = nl ? nl + 1 : p + strlen(p)) {
        nl = strchr(p, '\n'); if (nl) *nl = 0; ln++;
        if (ln == 1) { strncpy(first, trim(p), sizeof first - 1); }
        if (strstr(p, "hooks installed")) { hooks = 1; strncpy(hl, trim(p), sizeof hl - 1); }
        if (strstr(p, "not the Enhanced Edition")) none = 1;
    }
    free(t);
    if (first[0]) { char *b = strstr(first, "sh2coop PC build"); out("         first line: %s", b ? b : first); }
    if (none) fail("the last time the game ran, the mod found a different sh2pc.exe and did nothing. If check 1 is [ok] now, the exe has been fixed since: start the game once and run this checker again.");
    else if (hooks) {
        char *of = strstr(hl, " of "), *s = of; int a, b = of ? atoi(of + 4) : 0;
        while (s && s > hl && isdigit((unsigned char)s[-1])) s--;            /* the number right before " of " (the line starts with a time stamp) */
        a = s ? atoi(s) : 0;
        if (b && a == b) ok("%s - every hook went in", s ? s : hl);
        else warn("%s - some hooks did not go in; the lines above it in the log say which part is off (the rest of the mod still works)", s ? s : hl);
    } else warn("no 'hooks installed' line in the log - was the game started with the mod enabled? (LoadPlugins = 1, sh2coop.asi next to sh2pc.exe)");
}


/* ======================= the checks as one run (both programs) ======================= */
static int run_checks(const char *who)
{
    wchar_t rep[MAX_PATH]; int exe_ok, mod_ok, ini_ok, m_ok, scripts_only, ready;
    g_fail = g_warn = 0;
    wcscpy(rep, g_dir); wcsncat(rep, L"sh2coop_check.txt", MAX_PATH - wcslen(rep) - 1);
    g_rep = _wfopen(rep, L"w");
    out("%s", who);
    out("Game folder: %s", narrow(g_dir));
    exe_ok = check_exe();
    m_ok   = check_module();
    check_components();
    ini_ok = check_ini(&scripts_only);
    mod_ok = check_mod(scripts_only);
    check_others();
    check_log();
    out("");
    out("==========================================================================");
    ready = exe_ok && m_ok && ini_ok && mod_ok && !g_fail;
    if (ready)
        out("READY: everything the co-op mod needs is in place.%s Start the game; the title screen says CO-OP under the logo, and sh2coop.log ends its start-up with '... of ... hooks installed'.",
            g_warn ? " (Read the [NOTE] lines above - none of them stops the mod.)" : "");
    else
        out("NOT READY: %d thing%s to fix, marked [FIX] above. Fix %s by hand, then run the check again.", g_fail, g_fail == 1 ? "" : "s", g_fail == 1 ? "it" : "them");
    out("==========================================================================");
    if (g_rep) { fclose(g_rep); g_rep = NULL; out("(this report was also saved as sh2coop_check.txt in the game folder)"); }
    return ready;
}
static void pause_exit(int quiet, int code)
{
    if (!quiet) { printf("\nPress Enter to close.\n"); fflush(stdout); getchar(); }
    exit(code);
}
static void set_game_dir(const wchar_t *dir)       /* g_dir = dir with one trailing backslash */
{
    wcsncpy(g_dir, dir, MAX_PATH - 2); g_dir[MAX_PATH - 2] = 0;
    { size_t n = wcslen(g_dir); if (n && g_dir[n - 1] != L'\\') { g_dir[n] = L'\\'; g_dir[n + 1] = 0; } }
}

#ifdef CHECK_ONLY
int main(int argc, char **argv)
{
    wchar_t self[MAX_PATH], *sl; int quiet = 0;
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "/q")) quiet = 1;
    if (argc == 3 && !strcmp(argv[1], "-ver")) {                 /* test aid: print the version resource of one file */
        wchar_t w[MAX_PATH]; char name[128], ver[64];
        MultiByteToWideChar(CP_ACP, 0, argv[2], -1, w, MAX_PATH); g_dir[0] = 0;
        ver_info(w, name, sizeof name, ver, sizeof ver);
        printf("%s: InternalName=\"%s\" FileVersion=\"%s\"\n", argv[2], name, ver);
        return 0;
    }
    /* the game folder = where this checker is; if sh2pc.exe is not there, try the current folder */
    GetModuleFileNameW(NULL, self, MAX_PATH);
    wcscpy(g_dir, self); sl = wcsrchr(g_dir, L'\\'); if (sl) sl[1] = 0;
    if (!file_size(L"sh2pc.exe", NULL)) { wchar_t cd[MAX_PATH]; GetCurrentDirectoryW(MAX_PATH, cd); set_game_dir(cd); }
    if (!file_size(L"sh2pc.exe", NULL)) {
        out("Silent Hill 2 co-op mod - install checker.");
        out("Game folder: %s", narrow(g_dir));
        out("");
        fail("sh2pc.exe is not in this folder. Put sh2coop_check.exe into the Silent Hill 2 folder (the one with sh2pc.exe) and run it from there.");
        pause_exit(quiet, 1);
    }
    run_checks("Silent Hill 2 co-op mod - install checker. Reads only; the only file it writes is this report (sh2coop_check.txt).");
    pause_exit(quiet, g_fail ? 1 : 0);
    return 0;
}
#else
/* ======================= the installer ======================= */
#include <shlobj.h>

static wchar_t g_pkg[MAX_PATH];        /* the installer's own folder: where sh2coop.asi etc. are expected, trailing backslash */
static int g_yes;                      /* -y: take the default answer everywhere (unattended / tests) */

static void say(const char *fmt, ...)
{
    va_list ap; char line[2048];
    va_start(ap, fmt); _vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    line[sizeof line - 1] = 0; fputs(line, stdout); fputs("\n", stdout); fflush(stdout);
}
/* a one-key question; keys = the allowed answers in lower case, def = the index of the default (Enter) */
static int ask(const char *q, const char *keys, int def)
{
    char line[64];
    printf("%s ", q); fflush(stdout);
    if (g_yes) { printf("%c (automatic)\n", keys[def]); return def; }
    for (;;) {
        if (!fgets(line, sizeof line, stdin)) return def;
        { char *c = line; while (*c == ' ' || *c == '\t') c++;
          if (*c == '\n' || *c == '\r' || !*c) return def;
          for (int i = 0; keys[i]; i++) if (tolower((unsigned char)*c) == keys[i]) return i; }
        printf("please answer with one of: %s  ", keys); fflush(stdout);
    }
}
static int abs_exists(const wchar_t *p, int want_dir)
{
    DWORD a = GetFileAttributesW(p);
    if (a == INVALID_FILE_ATTRIBUTES) return 0;
    return want_dir ? (a & FILE_ATTRIBUTE_DIRECTORY) != 0 : (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
static void pjoin(wchar_t *dst, const wchar_t *dir, const wchar_t *name)
{
    size_t n;
    wcsncpy(dst, dir, MAX_PATH - 1); dst[MAX_PATH - 1] = 0; n = wcslen(dst);
    if (n && dst[n - 1] != L'\\' && n < MAX_PATH - 1) { dst[n++] = L'\\'; dst[n] = 0; }
    wcsncat(dst, name, MAX_PATH - n - 1);
}
static int is_game_dir(const wchar_t *dir) { wchar_t p[MAX_PATH]; pjoin(p, dir, L"sh2pc.exe"); return abs_exists(p, 0); }
static void strip_slash(wchar_t *dir) { size_t n = wcslen(dir); while (n > 3 && dir[n - 1] == L'\\') dir[--n] = 0; }

/* ---- d3d8.ini: one setting in the [Plugins] section, everything else left byte for byte ---- */
static int ini_get_int(const wchar_t *ini_abs, const char *section, const char *key, int def)
{
    wchar_t keep[MAX_PATH]; char *t, *p, *nl; int in = 0, v = def;
    wcscpy(keep, g_dir); g_dir[0] = 0; t = read_text(ini_abs, NULL); wcscpy(g_dir, keep);
    if (!t) return def;
    for (p = t; *p; p = nl ? nl + 1 : p + strlen(p)) {
        char *c, *eq;
        nl = strchr(p, '\n'); if (nl) *nl = 0;
        c = trim(p);
        if (*c == ';' || *c == '#' || !*c) continue;
        if (*c == '[') { char sec[128]; _snprintf(sec, sizeof sec, "[%s]", section); sec[sizeof sec - 1] = 0; in = ieq(c, sec); continue; }
        if (!in || !(eq = strchr(c, '='))) continue;
        *eq = 0;
        if (ieq(trim(c), key)) { v = atoi(trim(eq + 1)); break; }
    }
    free(t);
    return v;
}
/* sets "key = value" in [section] of d3d8.ini; a backup d3d8.ini.bak-before-coop is made the first time.  Returns 1 if ok. */
static int ini_set_int(const wchar_t *game, const char *section, const char *key, int value)
{
    wchar_t ini[MAX_PATH], bak[MAX_PATH], keep[MAX_PATH]; char *t, *outb, *p, *nl, sec[128], line[160]; unsigned n; int in = 0, done = 0, hdr = 0, crlf;
    size_t cap;
    pjoin(ini, game, L"d3d8.ini"); pjoin(bak, game, L"d3d8.ini.bak-before-coop");
    wcscpy(keep, g_dir); g_dir[0] = 0; t = read_text(ini, &n); wcscpy(g_dir, keep);
    if (!t) { say("  d3d8.ini could not be read"); return 0; }
    crlf = strstr(t, "\r\n") != NULL;
    _snprintf(sec, sizeof sec, "[%s]", section); sec[sizeof sec - 1] = 0;
    _snprintf(line, sizeof line, "%s = %d%s", key, value, crlf ? "\r\n" : "\n"); line[sizeof line - 1] = 0;
    cap = n + 2 * strlen(line) + 64; outb = malloc(cap); outb[0] = 0;
    for (p = t; *p; p = nl ? nl + 1 : p + strlen(p)) {
        char raw[4096], *c, *eq, tmp[4096]; size_t len;
        nl = strchr(p, '\n');
        len = nl ? (size_t)(nl - p + 1) : strlen(p);
        if (len >= sizeof raw) len = sizeof raw - 1;
        memcpy(raw, p, len); raw[len] = 0;
        strcpy(tmp, raw); c = trim(tmp);
        if (*c == '[') { if (in && !done) { strcat(outb, line); done = 1; }     /* leaving [Plugins] without the key: add it at its end */
                         in = ieq(c, sec); if (in) hdr = 1; strcat(outb, raw); continue; }
        if (in && !done && *c != ';' && *c != '#' && (eq = strchr(c, '='))) {
            *eq = 0;
            if (ieq(trim(c), key)) { strcat(outb, line); done = 1; continue; }     /* the line itself, replaced */
        }
        strcat(outb, raw);
    }
    if (!done) {
        size_t l = strlen(outb);
        if (l && outb[l - 1] != '\n') strcat(outb, crlf ? "\r\n" : "\n");
        if (!hdr) { strcat(outb, sec); strcat(outb, crlf ? "\r\n" : "\n"); }
        strcat(outb, line);
    }
    free(t);
    if (!abs_exists(bak, 0)) {
        if (CopyFileW(ini, bak, TRUE)) say("  backup of your d3d8.ini kept as d3d8.ini.bak-before-coop");
        else say("  (could not make the d3d8.ini backup, error %lu - going on)", GetLastError());
    }
    {
        HANDLE f = CreateFileW(ini, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); DWORD w;
        if (f == INVALID_HANDLE_VALUE) { say("  d3d8.ini could not be written (error %lu) - is it read-only, or the game running?", GetLastError()); free(outb); return 0; }
        WriteFile(f, outb, (DWORD)strlen(outb), &w, NULL); CloseHandle(f);
    }
    free(outb);
    return 1;
}

/* ---- a base Enhanced Edition install that has never been started has no d3d8.ini yet: the module writes its default
   one on the first start (dllmain.cpp: if (!PathFileExists(configpath)) ExtractFileFromResource(IDR_SETTINGS_INI, ...)).
   Do the same here from the player's own d3d8.dll: the RCDATA resource that holds the settings text. ---- */
static int ini_create_default(const wchar_t *game)
{
    wchar_t keep[MAX_PATH], ini[MAX_PATH]; unsigned n, dsz, k; unsigned char *b; const unsigned char *r; int done = 0;
    pjoin(ini, game, L"d3d8.ini");
    if (abs_exists(ini, 0)) return 1;
    wcscpy(keep, g_dir); set_game_dir(game); b = read_file(L"d3d8.dll", &n); wcscpy(g_dir, keep);
    if (!b) return 0;
    for (k = 0; k < 64 && !done; k++) {
        r = pe_resource(b, n, 10, k, &dsz);                       /* RT_RCDATA */
        if (!r) break;
        if (dsz < 1000 || dsz > (1u << 20)) continue;
        {   /* the settings text: has "[Plugins]" and "LoadPlugins" and is plain text */
            unsigned i, bad = 0; int has_sec = 0, has_key = 0;
            for (i = 0; i + 11 < dsz; i++) {
                if (r[i] == '[' && !memcmp(r + i, "[Plugins]", 9)) has_sec = 1;
                if (r[i] == 'L' && !memcmp(r + i, "LoadPlugins", 11)) has_key = 1;
                if (r[i] == 0) bad++;
            }
            if (!has_sec || !has_key || bad) continue;
        }
        {
            HANDLE f = CreateFileW(ini, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL); DWORD w;
            if (f == INVALID_HANDLE_VALUE) break;
            WriteFile(f, r, dsz, &w, NULL); CloseHandle(f);
            say("  d3d8.ini did not exist yet (the game has not been started since the Enhanced Edition was installed):");
            say("  created it from the Enhanced Edition's own default settings inside d3d8.dll, exactly as the game does on its first start (%u bytes)", dsz);
            done = 1;
        }
    }
    free(b);
    return done;
}

/* ---- putting the mod's files in ---- */
static int put_file(const wchar_t *name, const wchar_t *dstdir, const wchar_t *dstname, int keep_existing)
{
    wchar_t s[MAX_PATH], d[MAX_PATH];
    pjoin(s, g_pkg, name); pjoin(d, dstdir, dstname);
    if (!abs_exists(s, 0)) { say("  %s is missing next to the installer - unzip the whole package first", narrow(name)); return 0; }
    if (!_wcsicmp(s, d)) { say("  %s: already in place", narrow(dstname)); return 1; }
    if (keep_existing && abs_exists(d, 0)) { say("  %s: kept (your existing settings)", narrow(dstname)); return 1; }
    if (!CopyFileW(s, d, FALSE)) { say("  %s: could not be copied (error %lu)", narrow(dstname), GetLastError()); return 0; }
    say("  %s: copied", narrow(dstname));
    return 1;
}
/* an optional file of the package: under the package's name or the repository's name (the GitHub "Download ZIP" of the
   repository has the installer next to "sh2coop README.txt" instead of README.txt); skipped quietly if neither is there */
static int put_file_opt(const wchar_t *a, const wchar_t *b, const wchar_t *dstdir, const wchar_t *dstname)
{
    wchar_t s[MAX_PATH];
    pjoin(s, g_pkg, a); if (abs_exists(s, 0)) return put_file(a, dstdir, dstname, 0);
    pjoin(s, g_pkg, b); if (abs_exists(s, 0)) return put_file(b, dstdir, dstname, 0);
    say("  %s: not in this package - skipped", narrow(dstname));
    return 1;
}
/* can this program write into the game folder?  (Under C:\Program Files Windows refuses unless run as administrator.)
   Tries to create a temporary file there and removes it again. */
static int can_write_here(const wchar_t *game)
{
    wchar_t t[MAX_PATH]; HANDLE f; DWORD e;
    pjoin(t, game, L"sh2coop_write_test.tmp");
    f = CreateFileW(t, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (f != INVALID_HANDLE_VALUE) { CloseHandle(f); return 1; }
    e = GetLastError();
    if (e == ERROR_ALREADY_EXISTS || e == ERROR_FILE_EXISTS) return 1;
    say("  Windows did not allow this program to write into the game folder (error %lu%s).", e, e == ERROR_ACCESS_DENIED ? ", access denied" : "");
    say("  That happens when the game is under C:\\Program Files. Right-click Install co-op.exe, choose 'Run as administrator', and run it again.");
    return 0;
}
static int verify_game(const wchar_t *game)
{
    char hex[41], name[128], ver[64]; unsigned sz;
    set_game_dir(game);
    if (!file_size(L"sh2pc.exe", &sz)) { say("  no sh2pc.exe in %s", narrow(game)); return 0; }
    if (!sha1_file(L"sh2pc.exe", hex) || strcmp(hex, EXE_SHA1)) {
        say("  sh2pc.exe here is not the Enhanced Edition 1.0 exe the mod was made for (%u bytes%s).", sz, sz == EXE_2002_SIZE ? ", the untouched 2002 exe" : "");
        say("  Install the Enhanced Edition first (SH2EEsetup.exe, let it put its 'Enhanced EXE' in), then run this installer again.");
        return 0;
    }
    if (!file_size(L"d3d8.dll", NULL)) { say("  d3d8.dll is missing: the Enhanced Edition is not installed in this folder. Run SH2EEsetup.exe first."); return 0; }
    ver_info(L"d3d8.dll", name, sizeof name, ver, sizeof ver); sha1_file(L"d3d8.dll", hex);
    if (strcmp(hex, MODULE_SHA1) && !ieq(name, "sh2-enhce") && !contains_utf16(L"d3d8.dll", "sh2-enhce")) {
        say("  d3d8.dll here is not the Enhanced Edition module (it is what loads the mod). Re-run SH2EEsetup.exe so it puts the SH2 Enhancements module back.");
        return 0;
    }
    if (!file_size(L"d3d8.ini", NULL) && !ini_create_default(game)) {
        say("  d3d8.ini is missing and could not be created from d3d8.dll. Start the game once (the Enhanced Edition writes its d3d8.ini on the first start), then run this installer again.");
        return 0;
    }
    return 1;
}
static int install_into(const wchar_t *game)
{
    wchar_t ini[MAX_PATH], target[MAX_PATH]; int scripts_only, ok = 1;
    pjoin(ini, game, L"d3d8.ini");
    scripts_only = ini_get_int(ini, "Plugins", "LoadFromScriptsOnly", 0) == 1;
    if (scripts_only) { pjoin(target, game, L"scripts"); CreateDirectoryW(target, NULL); say("  (this d3d8.ini loads plugins only from scripts\\ - the mod goes there)"); }
    else wcscpy(target, game);
    say("Putting the mod's files into %s", narrow(target));
    ok &= put_file(L"sh2coop.asi", target, L"sh2coop.asi", 0);
    ok &= put_file(L"sh2coop.ini", target, L"sh2coop.ini", 1);
    ok &= put_file(L"sh2coop_check.exe", target, L"sh2coop_check.exe", 0);
    put_file_opt(L"README.txt", L"sh2coop README.txt", target, L"sh2coop README.txt");
    put_file_opt(L"VERSION.txt", L"sh2coop VERSION.txt", target, L"sh2coop VERSION.txt");
    say("Setting LoadPlugins = 1 in d3d8.ini (the Enhanced Edition ships with plugins off)");
    ok &= ini_set_int(game, "Plugins", "LoadPlugins", 1);
    return ok;
}
static int uninstall_from(const wchar_t *game)
{
    static const wchar_t *ours[] = { L"sh2coop.asi", L"sh2coop_check.exe", L"sh2coop README.txt", L"sh2coop VERSION.txt" };
    wchar_t p[MAX_PATH], bak[MAX_PATH]; int n = 0;
    say("Removing the co-op mod's own files from %s (sh2coop.ini and the logs are left for you to delete if you like)", narrow(game));
    for (int sub = 0; sub < 2; sub++)
        for (int i = 0; i < 4; i++) {
            if (sub) { wchar_t s[MAX_PATH]; pjoin(s, game, L"scripts"); pjoin(p, s, ours[i]); } else pjoin(p, game, ours[i]);
            if (abs_exists(p, 0)) { if (DeleteFileW(p)) { say("  removed %s%s", sub ? "scripts\\" : "", narrow(ours[i])); n++; } else say("  could not remove %s (error %lu)", narrow(p), GetLastError()); }
        }
    pjoin(bak, game, L"d3d8.ini.bak-before-coop");
    if (abs_exists(bak, 0) && ini_get_int(bak, "Plugins", "LoadPlugins", 0) == 0) {
        say("Setting LoadPlugins back to 0 (it was 0 before the mod was installed)");
        ini_set_int(game, "Plugins", "LoadPlugins", 0);
    } else say("LoadPlugins left as it is (it was already 1 before the mod, or there is no backup to tell)");
    say("%d file%s removed. The game is the plain Enhanced Edition again.", n, n == 1 ? "" : "s");
    return 1;
}
static int pick_folder(wchar_t *out)
{
    BROWSEINFOW bi; LPITEMIDLIST il; int ok = 0;
    CoInitialize(NULL);
    memset(&bi, 0, sizeof bi);
    bi.lpszTitle = L"Choose your Silent Hill 2 folder (the one that contains sh2pc.exe)";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    il = SHBrowseForFolderW(&bi);
    if (il && SHGetPathFromIDListW(il, out)) ok = 1;
    if (il) CoTaskMemFree(il);
    CoUninitialize();
    return ok;
}
int main(int argc, char **argv)
{
    wchar_t self[MAX_PATH], game[MAX_PATH] = L""; int quiet = 0, do_check = 0, do_uninst = 0, i;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-q")) quiet = 1;
        else if (!strcmp(argv[i], "-y")) g_yes = 1;
        else if (!strcmp(argv[i], "-check")) do_check = 1;
        else if (!strcmp(argv[i], "-uninstall")) do_uninst = 1;
        else if (argv[i][0] != '-') MultiByteToWideChar(CP_ACP, 0, argv[i], -1, game, MAX_PATH);   /* a folder given, or dropped on the exe */
    }
    GetModuleFileNameW(NULL, self, MAX_PATH);
    wcscpy(g_pkg, self); { wchar_t *sl = wcsrchr(g_pkg, L'\\'); if (sl) sl[1] = 0; }

    say("Silent Hill 2 co-op mod - installer");
    say("===================================");
    say("Adds the two-player co-op mod to your Silent Hill 2 Enhanced Edition: copies the mod's own files into the game");
    say("folder and switches plugin loading on in d3d8.ini (a backup of the ini is kept). Nothing else is changed.");
    say("To take it out again later, run this again and choose R, or rename sh2coop.asi to sh2coop.asi.off.");
    say("");
    say("  PLEASE USE A CLEAN INSTALL OF THE ENHANCED EDITION. The mod was made for, and tested on, the Enhanced Edition");
    say("  exactly as its setup tool installs it. With other mods in the game folder it may not work correctly, and the");
    say("  installer will not install over other plugins or wrapper DLLs.");
    say("");
    /* 1. which game folder */
    if (game[0]) { strip_slash(game); if (!is_game_dir(game)) { say("There is no sh2pc.exe in %s.", narrow(game)); game[0] = 0; } }
    if (!game[0] && is_game_dir(g_pkg)) { wcscpy(game, g_pkg); strip_slash(game); }
    if (!game[0]) { wchar_t cd[MAX_PATH]; GetCurrentDirectoryW(MAX_PATH, cd); if (is_game_dir(cd)) { wcscpy(game, cd); strip_slash(game); } }
    if (!game[0]) {
        say("Please choose your Silent Hill 2 folder in the window that opens (the folder with sh2pc.exe in it).");
        say("(Next time you can also just drag that folder onto this installer.)");
        if (g_yes || !pick_folder(game) || !is_game_dir(game)) {
            say("No Silent Hill 2 folder chosen%s. Nothing was changed.", game[0] ? " (that folder has no sh2pc.exe)" : "");
            pause_exit(quiet, 1);
        }
        strip_slash(game);
    }
    say("Silent Hill 2 folder: %s", narrow(game));
    say("");
    if (do_check) { set_game_dir(game); run_checks("Silent Hill 2 co-op mod - check only (nothing is changed)."); pause_exit(quiet, g_fail ? 1 : 0); }
    if (do_uninst) {
        if (ask("Remove the co-op mod's files from this folder? [y/N]", "ny", 0) != 1) { say("Nothing was changed."); pause_exit(quiet, 0); }
        if (!can_write_here(game)) { say("Nothing was changed."); pause_exit(quiet, 1); }
        uninstall_from(game); pause_exit(quiet, 0);
    }
    /* 2. is it an Enhanced Edition install the mod can use? */
    say("Checking the install...");
    if (!verify_game(game)) { say(""); say("Nothing was changed."); pause_exit(quiet, 1); }
    if (!can_write_here(game)) { say(""); say("Nothing was changed."); pause_exit(quiet, 1); }
    say("  Enhanced Edition exe 1.0 and the SH2 Enhancements module: ok");
    {   /* other code in the game folder?  then stop here, before anything is changed */
        int n; g_fail = g_warn = 0;
        n = scan_code_mods();
        if (n) {
            say("");
            say("STOP: this is not a clean Enhanced Edition install - the %d file%s listed above run%s code inside the game.", n, n == 1 ? "" : "s", n == 1 ? "s" : "");
            say("The co-op mod was made for a clean Enhanced Edition install and may not work correctly next to other mods, so it");
            say("is not installed over them. Move those files out of the game folder (or rename each .asi to .asi.off), start the");
            say("game once to make sure it still runs, then run this installer again.");
            say("");
            say("Nothing was changed.");
            pause_exit(quiet, 1);
        }
        say("  no other plugins or wrapper DLLs: a clean Enhanced Edition install");
        scan_data_mods(); g_fail = g_warn = 0;
    }
    say("");
    {   /* already installed here?  then offer update / uninstall / quit instead of a plain install */
        wchar_t a[MAX_PATH], b[MAX_PATH], sd[MAX_PATH]; char build[64] = "";
        pjoin(a, game, L"sh2coop.asi"); pjoin(sd, game, L"scripts"); pjoin(b, sd, L"sh2coop.asi");
        if (abs_exists(a, 0) || abs_exists(b, 0)) {
            set_game_dir(game); asi_build_name(abs_exists(a, 0) ? L"sh2coop.asi" : L"scripts\\sh2coop.asi", build, sizeof build);
            if (build[0]) say("The co-op mod is already installed here (build %s).", build); else say("The co-op mod is already installed here.");
            say("  [U] Update it with the files next to this installer");
            say("  [R] Remove it (the mod's own files are deleted, plugin loading is set back as it was)");
            say("  [Q] Quit, change nothing");
            switch (ask("Update, Remove or Quit? [U/r/q]", "urq", 0)) {
                case 1: if (!can_write_here(game)) { say("Nothing was changed."); pause_exit(quiet, 1); } uninstall_from(game); pause_exit(quiet, 0); break;
                case 2: say("Nothing was changed."); pause_exit(quiet, 0); break;
                default: break;
            }
        } else if (ask("Install the co-op mod into this folder? [Y/n]", "yn", 0) != 0) { say("Nothing was changed."); pause_exit(quiet, 0); }
    }
    say("");
    /* 3. the mod's files + the ini setting */
    if (!install_into(game)) { say(""); say("Something above did not work - see the line. The check below says what is still missing."); }
    say("");
    /* 4. the full check, same as sh2coop_check.exe */
    set_game_dir(game);
    if (run_checks("Final check (the same one sh2coop_check.exe makes):")) {
        say("");
        say("Done. Start the game as usual. The title screen says CO-OP under the logo.");
        say("In the game: G on the keyboard, or BACK+START on a controller, brings James 2 in. The full controls are in 'sh2coop README.txt' in the game folder.");
    } else {
        say("");
        say("The mod's files are in, but the check above found something to fix by hand (the [FIX] lines). Fix it, then run sh2coop_check.exe in the game folder.");
    }
    pause_exit(quiet, g_fail ? 1 : 0);
    return 0;
}
#endif
