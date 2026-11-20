/* sh2coop_upd.c - in-game updater (P33w).
 *
 * Where updates come from: the GitHub Releases of the mod's repository. Every release carries
 *   sh2coop-manifest.txt   "sh2coop-update 1", "build P33x", "tag P33x", then one line per file:
 *                          file <TAB> asset name <TAB> local name <TAB> size <TAB> sha256 (hex)
 *   sh2coop-manifest.sig   the ECDSA P-256 / SHA-256 signature of the manifest's exact bytes (r||s, 128 hex digits),
 *                          made with the release key, which is never on GitHub; the public half is below.
 * GitHub's "releases/latest/download/<asset>" link always points at the newest published (non-draft, non-pre-release)
 * release, so the mod needs no API call and no JSON.
 *
 * What it does, only when the player picks the row: reads the manifest and the signature, checks the signature with
 * Windows' own crypto (bcrypt), compares the build with its own (upd_build_seq), and on the second confirmation
 * downloads the files, checks every size and SHA-256, writes them beside the old ones as *.new, then swaps them in:
 * each old file is renamed to *.old (Windows lets a loaded DLL be renamed, not deleted) and the new one takes its
 * name. Any failure before the swap changes nothing; a failure during it puts back what was swapped. The next start
 * deletes the *.old files. Only the mod's own files can be written (kLocal), only into the mod's folder; sh2coop.ini
 * is never touched (a new build reads missing keys with their defaults).
 */
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sh2coop_upd.h"

#define REPO_URL   "https://github.com/sh2coop/Two-player-split-screen-co-op-for-Silent-Hill-2-PC-Enhanced-Edition-/releases/"
#define MANIFEST   "sh2coop-manifest.txt"
#define SIGNATURE  "sh2coop-manifest.sig"
#define MAX_FILES  8
#define MAX_FILE   (16u << 20)
#define MAX_TEXT   (64u << 10)

/* the release key's public half (P-256: X, Y) */
static const char kPubX[] = "3ef2e6c96be08df001b1cd0c25533e85a927daeccb61bf0b7cf54379e7954afc";
static const char kPubY[] = "7404282b3d82a2b5996df553cd231b0e582620b3d123f074fc14d37c94a1f171";

/* the only files an update may write (the installer's set, minus the ini) */
static const char *const kLocal[] = { "sh2coop.asi", "sh2coop_check.exe", "sh2coop README.txt", "sh2coop VERSION.txt" };
#define N_LOCAL (int)(sizeof kLocal / sizeof kLocal[0])

typedef struct { char asset[64], local[64]; DWORD size; unsigned char sha[32]; } UpdFile;

static char s_dir[MAX_PATH], s_self[MAX_PATH];
static UpdLogFn s_log;
static volatile LONG s_state = UPD_IDLE;
static char s_remote[32], s_tag[64], s_msg[64];
static UpdFile s_files[MAX_FILES];
static int s_nfiles;

static void ulog(const char *fmt, ...)
{
    if (!s_log) return;
    char b[512]; va_list a; va_start(a, fmt); _vsnprintf(b, sizeof b, fmt, a); va_end(a); b[sizeof b - 1] = 0;
    s_log("update: %s", b);
}
static void set_state(int st, const char *msg)
{
    if (msg) { lstrcpynA(s_msg, msg, sizeof s_msg); }
    MemoryBarrier();
    InterlockedExchange(&s_state, st);
}
static void fail(const char *msg, const char *why) { ulog("%s (%s) - nothing was changed", msg, why); set_state(UPD_FAILED, msg); }

long upd_build_seq(const char *name)
{
    if (!name || (name[0] != 'P' && name[0] != 'p')) return 0;
    const char *p = name + 1; long num = 0, let = 0; int digits = 0;
    while (*p >= '0' && *p <= '9' && digits < 5) { num = num * 10 + (*p - '0'); p++; digits++; }
    if (!digits) return 0;
    while (*p >= 'a' && *p <= 'z') { let = let * 26 + (*p - 'a' + 1); p++; if (let > 999) return 0; }
    if (*p) return 0;
    return num * 1000 + let;
}

/* ---------------- HTTP (WinHTTP) ---------------- */
static int http_get(const char *url, unsigned char **out, DWORD *len, DWORD max, char *why, int whylen)
{
    *out = NULL; *len = 0;
#ifdef SH2COOP_UPD_TEST
    if (!strncmp(url, "file:", 5)) {               /* test build only: a folder laid out like the release links */
        char path[MAX_PATH]; lstrcpynA(path, url + 5, sizeof path);
        for (char *c = path; *c; c++) if (*c == '/') *c = '\\';
        HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (h == INVALID_HANDLE_VALUE) { lstrcpynA(why, "HTTP 404", whylen); return 0; }
        DWORD sz = GetFileSize(h, NULL), got = 0;
        if (sz > max) { CloseHandle(h); lstrcpynA(why, "file too big", whylen); return 0; }
        unsigned char *buf = malloc(sz + 1);
        int ok = buf && ReadFile(h, buf, sz, &got, NULL) && got == sz;
        CloseHandle(h);
        if (!ok) { free(buf); lstrcpynA(why, "read error", whylen); return 0; }
        buf[sz] = 0; *out = buf; *len = sz; return 1;
    }
#endif
    WCHAR wurl[1024]; if (!MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 1024)) { lstrcpynA(why, "bad url", whylen); return 0; }
    URL_COMPONENTS uc; WCHAR host[256], path[768];
    ZeroMemory(&uc, sizeof uc); uc.dwStructSize = sizeof uc;
    uc.lpszHostName = host; uc.dwHostNameLength = 256; uc.lpszUrlPath = path; uc.dwUrlPathLength = 768;
    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) { lstrcpynA(why, "bad url", whylen); return 0; }
    int secure = uc.nScheme == INTERNET_SCHEME_HTTPS;
#ifndef SH2COOP_UPD_TEST
    if (!secure) { lstrcpynA(why, "not https", whylen); return 0; }
#endif
    HINTERNET ses = WinHttpOpen(L"sh2coop-updater/" SH2COOP_BUILD, 4 /* WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, 8.1+ */, NULL, NULL, 0);
    if (!ses) ses = WinHttpOpen(L"sh2coop-updater/" SH2COOP_BUILD, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) { _snprintf(why, whylen, "WinHttpOpen %lu", GetLastError()); return 0; }
    WinHttpSetTimeouts(ses, 15000, 15000, 20000, 30000);
    DWORD pol = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    WinHttpSetOption(ses, WINHTTP_OPTION_REDIRECT_POLICY, &pol, sizeof pol);
    HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0), req = NULL;
    int ok = 0;
    if (con) req = WinHttpOpenRequest(con, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0);
    if (!req) { _snprintf(why, whylen, "connect %lu", GetLastError()); goto done; }
    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(req, NULL)) {
        _snprintf(why, whylen, "no connection (%lu)", GetLastError()); goto done;
    }
    DWORD code = 0, sz = sizeof code;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &sz, WINHTTP_NO_HEADER_INDEX);
    if (code != 200) { _snprintf(why, whylen, "HTTP %lu", code); goto done; }
    unsigned char *buf = malloc(max + 1); DWORD n = 0;
    if (!buf) { lstrcpynA(why, "out of memory", whylen); goto done; }
    for (;;) {
        DWORD avail = 0, got = 0;
        if (!WinHttpQueryDataAvailable(req, &avail)) { _snprintf(why, whylen, "download cut (%lu)", GetLastError()); free(buf); goto done; }
        if (!avail) break;
        if (n + avail > max) { lstrcpynA(why, "file too big", whylen); free(buf); goto done; }
        if (!WinHttpReadData(req, buf + n, avail, &got)) { _snprintf(why, whylen, "download cut (%lu)", GetLastError()); free(buf); goto done; }
        n += got;
    }
    buf[n] = 0; *out = buf; *len = n; ok = 1;
done:
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
    return ok;
}
static const char *base_url(void)
{
#ifdef SH2COOP_UPD_TEST
    static char b[512];
    if (GetEnvironmentVariableA("SH2COOP_UPD_BASE", b, sizeof b)) return b;   /* test build only: a local server */
#endif
    return REPO_URL;
}

/* ---------------- crypto (bcrypt.dll, loaded on demand) ---------------- */
typedef LONG (WINAPI *OpenAlgFn)(void **, LPCWSTR, LPCWSTR, ULONG);
typedef LONG (WINAPI *CloseAlgFn)(void *, ULONG);
typedef LONG (WINAPI *CreateHashFn)(void *, void **, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
typedef LONG (WINAPI *HashDataFn)(void *, PUCHAR, ULONG, ULONG);
typedef LONG (WINAPI *FinishHashFn)(void *, PUCHAR, ULONG, ULONG);
typedef LONG (WINAPI *DestroyFn)(void *);
typedef LONG (WINAPI *ImportFn)(void *, void *, LPCWSTR, void **, PUCHAR, ULONG, ULONG);
typedef LONG (WINAPI *VerifyFn)(void *, void *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
static struct { OpenAlgFn open; CloseAlgFn close; CreateHashFn create; HashDataFn data; FinishHashFn finish; DestroyFn dhash, dkey; ImportFn import; VerifyFn verify; } B;
static int bcrypt_ok(void)
{
    static int tried, ok;
    if (tried) return ok;
    tried = 1;
    HMODULE h = LoadLibraryA("bcrypt.dll");
    if (!h) return 0;
    B.open = (OpenAlgFn)GetProcAddress(h, "BCryptOpenAlgorithmProvider"); B.close = (CloseAlgFn)GetProcAddress(h, "BCryptCloseAlgorithmProvider");
    B.create = (CreateHashFn)GetProcAddress(h, "BCryptCreateHash"); B.data = (HashDataFn)GetProcAddress(h, "BCryptHashData");
    B.finish = (FinishHashFn)GetProcAddress(h, "BCryptFinishHash"); B.dhash = (DestroyFn)GetProcAddress(h, "BCryptDestroyHash");
    B.dkey = (DestroyFn)GetProcAddress(h, "BCryptDestroyKey"); B.import = (ImportFn)GetProcAddress(h, "BCryptImportKeyPair");
    B.verify = (VerifyFn)GetProcAddress(h, "BCryptVerifySignature");
    ok = B.open && B.close && B.create && B.data && B.finish && B.dhash && B.dkey && B.import && B.verify;
    return ok;
}
static int sha256(const unsigned char *p, DWORD n, unsigned char out[32])
{
    void *alg = NULL, *h = NULL; int ok = 0;
    if (B.open(&alg, L"SHA256", NULL, 0) != 0) return 0;
    if (B.create(alg, &h, NULL, 0, NULL, 0, 0) == 0) {
        ok = B.data(h, (PUCHAR)p, n, 0) == 0 && B.finish(h, out, 32, 0) == 0;
        B.dhash(h);
    }
    B.close(alg, 0);
    return ok;
}
static int unhex(const char *s, unsigned char *out, int n)
{
    for (int i = 0; i < n; i++) {
        int v = 0;
        for (int k = 0; k < 2; k++) {
            char c = s[2 * i + k]; v <<= 4;
            if (c >= '0' && c <= '9') v |= c - '0'; else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10; else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10; else return 0;
        }
        out[i] = (unsigned char)v;
    }
    return 1;
}
static int signature_ok(const unsigned char *msg, DWORD n, const unsigned char sig[64])
{
    struct { ULONG magic, cb; unsigned char xy[64]; } blob;
    blob.magic = 0x31534345; /* BCRYPT_ECDSA_PUBLIC_P256_MAGIC */ blob.cb = 32;
    if (!unhex(kPubX, blob.xy, 32) || !unhex(kPubY, blob.xy + 32, 32)) return 0;
    unsigned char hash[32];
    if (!sha256(msg, n, hash)) return 0;
    void *alg = NULL, *key = NULL; int ok = 0;
    if (B.open(&alg, L"ECDSA_P256", NULL, 0) != 0) return 0;
    if (B.import(alg, NULL, L"ECCPUBLICBLOB", &key, (PUCHAR)&blob, sizeof blob, 0) == 0) {
        ok = B.verify(key, NULL, hash, 32, (PUCHAR)sig, 64, 0) == 0;
        B.dkey(key);
    }
    B.close(alg, 0);
    return ok;
}

/* ---------------- manifest ---------------- */
static int name_chars_ok(const char *s, int spaces)
{
    if (!*s || *s == '.') return 0;
    for (; *s; s++)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '.' || *s == '_' || *s == '-' || (spaces && *s == ' ')))
            return 0;
    return 1;
}
static int local_allowed(const char *s) { for (int i = 0; i < N_LOCAL; i++) if (!strcmp(s, kLocal[i])) return 1; return 0; }
/* the manifest's text (already signature-checked): 1 = parsed */
static int parse_manifest(char *t, char *why, int whylen)
{
    s_nfiles = 0; s_remote[0] = s_tag[0] = 0;
    int line = 0, has_asi = 0;
    for (char *p = t; p && *p;) {
        char *e = strchr(p, '\n'); if (e) *e = 0;
        size_t l = strlen(p); if (l && p[l - 1] == '\r') p[l - 1] = 0;
        if (line == 0 && strcmp(p, "sh2coop-update 1")) { lstrcpynA(why, "unknown manifest format", whylen); return 0; }
        if (!strncmp(p, "build ", 6)) lstrcpynA(s_remote, p + 6, sizeof s_remote);
        else if (!strncmp(p, "tag ", 4)) lstrcpynA(s_tag, p + 4, sizeof s_tag);
        else if (!strncmp(p, "file\t", 5)) {
            if (s_nfiles >= MAX_FILES) { lstrcpynA(why, "too many files", whylen); return 0; }
            UpdFile *f = &s_files[s_nfiles];
            char *fld[4]; int nf = 0; char *q = p + 5;        /* asset, local name, size, sha256 */
            while (nf < 4 && q) { fld[nf++] = q; q = strchr(q, '\t'); if (q) *q++ = 0; }
            if (nf != 4 || q) { lstrcpynA(why, "bad file line", whylen); return 0; }
            if (strlen(fld[0]) >= sizeof f->asset || strlen(fld[1]) >= sizeof f->local || !name_chars_ok(fld[0], 0) || !local_allowed(fld[1])
                || strlen(fld[3]) != 64 || !unhex(fld[3], f->sha, 32)) { lstrcpynA(why, "file not allowed", whylen); return 0; }
            char *end; unsigned long sz = strtoul(fld[2], &end, 10);
            if (*end || !sz || sz > MAX_FILE) { lstrcpynA(why, "bad file size", whylen); return 0; }
            for (int i = 0; i < s_nfiles; i++) if (!strcmp(s_files[i].local, fld[1])) { lstrcpynA(why, "file listed twice", whylen); return 0; }
            strcpy(f->asset, fld[0]); strcpy(f->local, fld[1]); f->size = sz;
            if (!strcmp(f->local, "sh2coop.asi")) has_asi = 1;
            s_nfiles++;
        }
        line++;
        p = e ? e + 1 : NULL;
    }
    if (!upd_build_seq(s_remote) || !name_chars_ok(s_tag, 0) || !has_asi) { lstrcpynA(why, "manifest incomplete", whylen); return 0; }
    return 1;
}

/* ---------------- check ---------------- */
static DWORD WINAPI check_thread(LPVOID unused)
{
    (void)unused;
    char url[600], why[96] = "";
    unsigned char *man = NULL, *sig = NULL; DWORD nman = 0, nsig = 0;
    if (!bcrypt_ok()) { fail("This Windows can't check updates", "bcrypt.dll missing"); return 0; }
    _snprintf(url, sizeof url, "%slatest/download/" MANIFEST, base_url()); url[sizeof url - 1] = 0;
    ulog("checking %s", url);
    if (!http_get(url, &man, &nman, MAX_TEXT, why, sizeof why)) {
        if (!strcmp(why, "HTTP 404")) fail("No release has been published yet", why);
        else fail("Couldn't reach GitHub", why);
        return 0;
    }
    /* the signature of the same release (by its tag, so a release published in between can't mix the two) */
    char tag[64] = "";
    { const char *t = strstr((char *)man, "\ntag "); if (t) { int i = 0; t += 5; while (t[i] && t[i] != '\r' && t[i] != '\n' && i < 63) { tag[i] = t[i]; i++; } tag[i] = 0; } }
    if (!name_chars_ok(tag, 0)) { fail("The release's files are damaged", "no tag"); free(man); return 0; }
    _snprintf(url, sizeof url, "%sdownload/%s/" SIGNATURE, base_url(), tag); url[sizeof url - 1] = 0;
    if (!http_get(url, &sig, &nsig, 1024, why, sizeof why)) { fail("Couldn't reach GitHub", why); free(man); return 0; }
    unsigned char raw[64];
    while (nsig && (sig[nsig - 1] == '\n' || sig[nsig - 1] == '\r' || sig[nsig - 1] == ' ')) sig[--nsig] = 0;
    if (nsig != 128 || !unhex((char *)sig, raw, 64) || !signature_ok(man, nman, raw)) {
        fail("Update NOT trusted: bad signature", "the manifest is not signed with the release key");
        free(man); free(sig); return 0;
    }
    free(sig);
    if (!parse_manifest((char *)man, why, sizeof why)) { fail("The release's files are damaged", why); free(man); return 0; }
    free(man);
    long mine = upd_build_seq(SH2COOP_BUILD), theirs = upd_build_seq(s_remote);
    ulog("latest release %s (tag %s, %d files), this build %s", s_remote, s_tag, s_nfiles, SH2COOP_BUILD);
    if (theirs > mine) set_state(UPD_AVAILABLE, "Press Enter again to install it");
    else set_state(UPD_CURRENT, "You have the newest version");
    return 0;
}

/* ---------------- install ---------------- */
static void local_path(const UpdFile *f, char *out, const char *suffix)
{
    if (!strcmp(f->local, "sh2coop.asi") && s_self[0]) _snprintf(out, MAX_PATH, "%s%s", s_self, suffix);   /* the mod's own file, whatever it is called */
    else _snprintf(out, MAX_PATH, "%s%s%s", s_dir, f->local, suffix);
    out[MAX_PATH - 1] = 0;
}
static int write_file(const char *path, const unsigned char *p, DWORD n)
{
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    DWORD w = 0; int ok = WriteFile(h, p, n, &w, NULL) && w == n && FlushFileBuffers(h);
    CloseHandle(h);
    if (!ok) DeleteFileA(path);
    return ok;
}
static DWORD WINAPI install_thread(LPVOID unused)
{
    (void)unused;
    unsigned char *data[MAX_FILES] = { 0 }; DWORD len[MAX_FILES] = { 0 };
    char url[700], why[96] = "", p[MAX_PATH], q[MAX_PATH];
    int i, swapped = 0, existed[MAX_FILES] = { 0 };
    /* 1. can we write here at all (Program Files without admin cannot) */
    _snprintf(p, sizeof p, "%ssh2coop_update.tmp", s_dir); p[sizeof p - 1] = 0;
    if (!write_file(p, (const unsigned char *)"x", 1)) { fail("Can't write here: use the installer as admin", "the game folder is read-only"); return 0; }
    DeleteFileA(p);
    /* 2. every file, checked */
    for (i = 0; i < s_nfiles; i++) {
        _snprintf(url, sizeof url, "%sdownload/%s/%s", base_url(), s_tag, s_files[i].asset); url[sizeof url - 1] = 0;
        ulog("downloading %s", url);
        unsigned char h[32];
        if (!http_get(url, &data[i], &len[i], MAX_FILE, why, sizeof why)) { fail("Download failed - try again", why); goto out; }
        if (len[i] != s_files[i].size || !sha256(data[i], len[i], h) || memcmp(h, s_files[i].sha, 32)) {
            fail("Download damaged - try again", s_files[i].local); goto out;
        }
    }
    /* 3. beside the old ones */
    for (i = 0; i < s_nfiles; i++) {
        local_path(&s_files[i], p, ".new");
        if (!write_file(p, data[i], len[i])) {
            for (int k = 0; k < i; k++) { local_path(&s_files[k], q, ".new"); DeleteFileA(q); }
            fail("Can't write here: use the installer as admin", p); goto out;
        }
    }
    /* 4. the swap */
    for (i = 0; i < s_nfiles; i++) {
        char cur[MAX_PATH], old[MAX_PATH], nw[MAX_PATH];
        local_path(&s_files[i], cur, ""); local_path(&s_files[i], old, ".old"); local_path(&s_files[i], nw, ".new");
        existed[i] = GetFileAttributesA(cur) != INVALID_FILE_ATTRIBUTES;
        DeleteFileA(old);
        if (existed[i] && !MoveFileExA(cur, old, MOVEFILE_REPLACE_EXISTING)) { _snprintf(why, sizeof why, "rename %s: %lu", s_files[i].local, GetLastError()); break; }
        if (!MoveFileExA(nw, cur, MOVEFILE_REPLACE_EXISTING)) {
            _snprintf(why, sizeof why, "place %s: %lu", s_files[i].local, GetLastError());
            if (existed[i]) MoveFileExA(old, cur, MOVEFILE_REPLACE_EXISTING);
            break;
        }
        swapped++;
    }
    if (swapped < s_nfiles) {   /* put back what was swapped, remove what was not placed */
        for (int k = 0; k < swapped; k++) {
            char cur[MAX_PATH], old[MAX_PATH]; local_path(&s_files[k], cur, ""); local_path(&s_files[k], old, ".old");
            if (existed[k]) MoveFileExA(old, cur, MOVEFILE_REPLACE_EXISTING); else DeleteFileA(cur);
        }
        for (int k = 0; k < s_nfiles; k++) { local_path(&s_files[k], q, ".new"); DeleteFileA(q); }
        fail("Couldn't replace the files", why); goto out;
    }
    ulog("installed %s (%d files); the old files are kept as *.old until the next start", s_remote, s_nfiles);
    set_state(UPD_DONE, "Quit and restart the game to finish");
out:
    for (i = 0; i < s_nfiles; i++) free(data[i]);
    return 0;
}

/* ---------------- interface ---------------- */
void upd_init(const char *dir, const char *self_path, UpdLogFn log)
{
    lstrcpynA(s_dir, dir, sizeof s_dir); lstrcpynA(s_self, self_path ? self_path : "", sizeof s_self); s_log = log;
    for (int i = 0; i < N_LOCAL; i++) {           /* what an update left behind */
        UpdFile f; strcpy(f.local, kLocal[i]);
        char p[MAX_PATH];
        local_path(&f, p, ".old"); if (DeleteFileA(p)) ulog("removed %s (left by the last update)", p);
        local_path(&f, p, ".new"); if (DeleteFileA(p)) ulog("removed %s (an update that did not finish)", p);
    }
}
static void start(LPTHREAD_START_ROUTINE fn, int st, const char *msg)
{
    set_state(st, msg);
    HANDLE t = CreateThread(NULL, 0, fn, NULL, 0, NULL);
    if (t) CloseHandle(t); else fail("Couldn't start the update", "CreateThread");
}
void upd_check(void)
{
    LONG st = s_state;
    if (st == UPD_IDLE || st == UPD_CURRENT || st == UPD_FAILED) start(check_thread, UPD_CHECKING, "Asking GitHub for the newest version");
}
void upd_install(void) { if (s_state == UPD_AVAILABLE) start(install_thread, UPD_INSTALLING, "Downloading - you can keep playing"); }
int upd_state(void) { return (int)s_state; }
const char *upd_remote(void) { return s_remote; }
const char *upd_message(void) { return s_msg; }

#ifdef SH2COOP_UPD_MAIN
/* test tool (never shipped): the updater without the game.
     upd_test seq NAME                        the build-order number of a build name
     upd_test get URL                         one download: status, size, SHA-256
     upd_test run FOLDER\ check|install [LOCKED_FILE]
                                              FOLDER\sh2coop.asi is loaded first (as in the game), the folder is then
                                              updated from SH2COOP_UPD_BASE; LOCKED_FILE is held open without sharing
                                              during the install (a file another program has open)
     upd_test ver FOLDER\                     loads FOLDER\sh2coop.asi and prints its dummy_version() */
static void plog(const char *fmt, ...) { va_list a; va_start(a, fmt); vprintf(fmt, a); va_end(a); printf("\n"); fflush(stdout); }
int main(int argc, char **argv)
{
    if (argc >= 3 && !strcmp(argv[1], "seq")) { printf("%s=%ld\n", argv[2], upd_build_seq(argv[2])); return 0; }
    if (argc >= 3 && !strcmp(argv[1], "get")) {
        unsigned char *b; DWORD n; char why[96] = "";
        if (!bcrypt_ok()) { printf("GET no bcrypt\n"); return 1; }
        if (!http_get(argv[2], &b, &n, MAX_FILE, why, sizeof why)) { printf("GET %s -> failed: %s\n", argv[2], why); return 1; }
        unsigned char h[32]; sha256(b, n, h);
        printf("GET %s -> %lu bytes, sha256 ", argv[2], n); for (int i = 0; i < 32; i++) printf("%02x", h[i]); printf("\n");
        return 0;
    }
    if (argc >= 3 && !strcmp(argv[1], "ver")) {
        char self[MAX_PATH]; _snprintf(self, sizeof self, "%ssh2coop.asi", argv[2]);
        HMODULE m = LoadLibraryA(self); int (*v)(void) = m ? (int (*)(void))GetProcAddress(m, "dummy_version") : NULL;
        printf("VER %s -> %d\n", self, v ? v() : -1); return 0;
    }
    if (argc < 4 || strcmp(argv[1], "run")) { printf("usage: see the source\n"); return 2; }
    char self[MAX_PATH]; _snprintf(self, sizeof self, "%ssh2coop.asi", argv[2]);
    HMODULE m = LoadLibraryA(self);
    plog("loaded %s: %s", self, m ? "yes" : "no");
    upd_init(argv[2], self, plog);
    upd_check();
    while (upd_state() == UPD_CHECKING) Sleep(20);
    plog("CHECK state %d remote '%s' msg '%s'", upd_state(), upd_remote(), upd_message());
    if (!strcmp(argv[3], "install") && upd_state() == UPD_AVAILABLE) {
        HANDLE lk = INVALID_HANDLE_VALUE;
        if (argc >= 5) { char lp[MAX_PATH]; _snprintf(lp, sizeof lp, "%s%s", argv[2], argv[4]); lk = CreateFileA(lp, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL); plog("holding %s open: %s", lp, lk != INVALID_HANDLE_VALUE ? "yes" : "no"); }
        upd_install();
        while (upd_state() == UPD_INSTALLING) Sleep(20);
        plog("INSTALL state %d msg '%s'", upd_state(), upd_message());
        if (lk != INVALID_HANDLE_VALUE) CloseHandle(lk);
    }
    return 0;
}
#endif
