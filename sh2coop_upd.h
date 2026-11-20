/* sh2coop_upd.h - in-game updater: the newest GitHub Release of the mod, checked and installed from
   Options > Game Options > Co-op Options > Updates. Nothing happens unless the player picks the row. */
#ifndef SH2COOP_UPD_H
#define SH2COOP_UPD_H

/* the build this file belongs to (also in the log's first line and in the release manifest) */
#ifndef SH2COOP_BUILD               /* (a test build may be compiled as an older one, -DSH2COOP_BUILD=\"P1\") */
#define SH2COOP_BUILD "P33x"
#endif

enum {
    UPD_IDLE,        /* nothing asked yet */
    UPD_CHECKING,    /* reading the latest release's manifest */
    UPD_CURRENT,     /* this build is the newest */
    UPD_AVAILABLE,   /* a newer build exists: upd_remote() names it */
    UPD_INSTALLING,  /* downloading and checking the files */
    UPD_DONE,        /* installed: the game must be restarted */
    UPD_FAILED       /* upd_message() says why; nothing was changed */
};

typedef void (*UpdLogFn)(const char *fmt, ...);

/* at start-up: the folder the mod's files are in (with the trailing backslash), the mod's own file, the log;
   removes what an earlier update left behind (*.old of our files) */
void upd_init(const char *dir, const char *self_path, UpdLogFn log);
void upd_check(void);          /* starts a check in the background (from UPD_IDLE / CURRENT / FAILED) */
void upd_install(void);        /* starts the install (from UPD_AVAILABLE) */
int  upd_state(void);
const char *upd_remote(void);  /* the newer build's name (UPD_AVAILABLE and later) */
const char *upd_message(void); /* a short line for the menu (why it failed, what to do) */

/* "P33w" -> a number that grows with every build: 33 * 1000 + letters (a = 1 .. z = 26, aa = 27 ...); 0 = not a build name */
long upd_build_seq(const char *name);

#endif
