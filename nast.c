/* See LICENSE for license details. */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#define __USE_XOPEN
#include <wchar.h>

#include "nast.h"

/* vtiden: identification sequence returned in DA and DECID
   see https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
   search for "Send Device Attributes"

   We use the exact value that xterm gives.

   The initial 64 indicates VT420 features.

   Supported subfeatures:
     1   ->  132-columns.
     2   ->  Printer.
     6   ->  Selective erase.
     9   ->  National Replacement Character sets.
     15  ->  Technical characters.
     16  ->  Locator port.
     17  ->  Terminal state interrogation.
     18  ->  User windows.
     21  ->  Horizontal scrolling.
     22  ->  ANSI color, e.g., VT525.
     28  ->  Rectangular editing.
   Unsupported subfeatures:
     3   ->  ReGIS graphics.
     4   ->  Sixel graphics.
     8   ->  User-defined keys.
     29  ->  ANSI text locator (i.e., DEC Locator mode).
*/
char *vtiden = "\033[?64;1;2;6;9;15;16;17;18;21;22;28c";

#if   defined(__linux)
 #include <pty.h>
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
 #include <util.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
 #include <libutil.h>
#endif

/* Arbitrary sizes */
#define UTF_INVALID   0xFFFD
#define UTF_SIZ       4
#define ESC_BUF_SIZ   (128*UTF_SIZ)
#define ESC_ARG_SIZ   16
#define STR_BUF_SIZ   ESC_BUF_SIZ
#define STR_ARG_SIZ   ESC_ARG_SIZ

/* macros */
#define IS_SET(t, flag)        ((t->mode & (flag)) != 0)
#define ISCONTROLC0(c)        (BETWEEN(c, 0, 0x1f) || (c) == '\177')
#define ISCONTROLC1(c)        (BETWEEN(c, 0x80, 0x9f))
#define ISCONTROL(c)        (ISCONTROLC0(c) || ISCONTROLC1(c))

// actually there is one less line than this...
#define RLINES_LIMIT 10000

enum term_mode {
    MODE_WRAP        = 1 << 0,
    MODE_INSERT      = 1 << 1,
    MODE_ALTSCREEN   = 1 << 2,
    MODE_CRLF        = 1 << 3,
    MODE_ECHO        = 1 << 4,
    MODE_PRINT       = 1 << 5,
    MODE_UTF8        = 1 << 6,
    MODE_SIXEL       = 1 << 7,
    /* This used to be associated with the window rather than with the terminal
       but I don't fully understand why.  I think that copy/paste mechanics
       basically be a detail of the interface between libnast and the backend
       (see architecture diagram in README) and not a window mode. */
    MODE_BRCKTPASTE  = 1 << 8,
};

enum cursor_movement {
    CURSOR_SAVE,
    CURSOR_LOAD
};

enum cursor_state {
    CURSOR_DEFAULT  = 0,
    CURSOR_WRAPNEXT = 1,
    CURSOR_ORIGIN   = 2,
};

enum charset {
    CS_GRAPHIC0,
    CS_GRAPHIC1,
    CS_UK,
    CS_USA,
    CS_MULTI,
    CS_GER,
    CS_FIN
};

enum escape_state {
    ESC_START      = 1,
    ESC_CSI        = 2,
    ESC_STR        = 4,  /* OSC, PM, APC */
    ESC_ALTCHARSET = 8,
    ESC_STR_END    = 16, /* a final string was encountered */
    ESC_TEST       = 32, /* Enter in test mode */
    ESC_UTF8       = 64,
    ESC_DCS        =128,
};

typedef struct {
    Glyph attr; /* current char attributes */
    // position in line
    int x;
    // y cursor position, in terminal coordinates.
    int y;
    char state;
} TCursor;

typedef struct {
    int mode;
    int type;
    int snap;
    /*
     * Selection variables:
     * nb – normalized coordinates of the beginning of the selection
     * ne – normalized coordinates of the end of the selection
     * ob – original coordinates of the beginning of the selection
     * oe – original coordinates of the end of the selection
     */
    struct {
        int x, y;
    } nb, ne, ob, oe;

    int alt;
} Selection;

typedef struct {
    RLine **rlines;
    // ring buffer semantics
    size_t cap;       // cap = item length of the physical buffer
    size_t start;     // the oldest line in memory
    size_t len;       // number of lines in memory
    uint64_t line_id; // current line UID
} Screen;

/* Internal representation of the screen */
struct Term {
    int row;      /* nb row */
    int col;      /* nb col */

    // font stuff
    PangoFontDescription *desc;
    double grid_w;
    double grid_h;

    // rendered dimensions, should be checked each re-render
    double render_w;
    double render_h;

    // main screen
    Screen main;
    // altscreen
    Screen alt;
    // points to either .main or .alt
    Screen *scr;

    // how many unrendered lines are below the window
    size_t scroll;

    // callbacks
    THooks *hooks;

    TCursor c;    /* cursor */
    int ocx;      /* old cursor col */
    int ocy;      /* old cursor row */
    int top;      /* top    scroll limit */
    int bot;      /* bottom scroll limit */
    int mode;     /* terminal mode flags */
    int esc;      /* escape state flags */
    char trantbl[4]; /* charset table translation */
    int charset;  /* current charset */
    int icharset; /* selected charset for sequence */
    int *tabs;
    enum cursor_style cursor_style;

    // buffer for ttyread
    int cmdfd;
    char ttyreadbuf[BUFSIZ];
    size_t ttyreadbuflen;

    // buffer for tcursor
    TCursor saved[2];
};

/* CSI Escape sequence structs */
/* ESC '[' [[ [<priv>] <arg> [;]] [<submode>] <mode> ] */
/* note that <priv> can be '?' or '>' */
// see https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
typedef struct {
    char buf[ESC_BUF_SIZ]; /* raw string */
    size_t len;            /* raw string length */
    char priv;
    int arg[ESC_ARG_SIZ];
    int narg;              /* nb of args */
    char submode;
    char mode;
} CSIEscape;

/* STR Escape sequence structs */
/* ESC type [[ [<priv>] <arg> [;]] <mode>] ESC '\' */
typedef struct {
    char type;             /* ESC type ... */
    char *buf;             /* allocated raw string */
    size_t siz;            /* allocation size */
    size_t len;            /* raw string length */
    char *args[STR_ARG_SIZ];
    int narg;              /* nb of args */
} STREscape;

static void execsh(char *, char **);
static void stty(char **);
// static void ttywriteraw(t, const char *, size_t);

static void csidump(void);
static void csihandle(Term *t);
static void csiparse(void);
static void csireset(void);
static int eschandle(Term *t, uchar);
static void strdump(void);
static void strhandle(Term *t);
static void strparse(void);
static void strreset(void);

static void tprinter(Term *t, char *, size_t);
static void tdumpsel(Term *t);
static void tdumpline(Term *t, int);
static void tdump(Term *t);
static void temit(Term*, Rune, int);
static void tclearregion_abs(Term *t, int, int, int, int);
static void tclearregion_term(Term *t, int, int, int, int);
static void tcursor(Term *t, int);
static int tcursorstyle(Term *t, int);
static void tdeletechar(Term *t, int);
static void tdeleteline(Term *t, int);
static void tinsertblank(Term *t, int);
static void tinsertblankline(Term *t, int);
// static int tlinelen(Term *t, int);
static void tmoveto(Term *t, int, int);
static void tmoveto_origin(Term *t, int x, int y);
static Rune acsc(Rune u, int charset);
static void tnewline(Term *t, int, bool);
static void tputtab(Term *t, int);
static void tputc(Term *t, Rune);
static void treset(Term *t);
static RLine *scr_new_rline(Screen *scr, uint64_t line_id, size_t cols);
static void tscrollup(Term *t, int, int);
static void tscrolldown(Term *t, int, int);
static void tsetattr(Term *t, int *, int);
static void tsetchar(Term *t, Rune, Glyph *, int, int);
static void tswapscreen(Term *t);
static void tsetmode(Term *t, int, int, int *, int);
// static int twrite(t, const char *, int, int);
static void tcontrolcode(Term *t, uchar );
static void tdectest(Term *t, char );
static void tdefutf8(Term *t, char);
static struct rgb24 tdefcolor(Term *t, int *, int *, int, struct rgb24);
static void tdeftran(Term *t, char);
static void tstrsequence(Term *t, uchar);
static void tscrollregion(Term *t, int top, int bot);

static ssize_t xwrite(int, const char *, size_t);

/* Globals */
static Selection sel;
static CSIEscape csiescseq;
static STREscape strescseq;
static int iofd = 1;

// get the physical index from an offset (a logical index)
static inline size_t rlines_idx(Screen *scr, size_t idx){
    return (scr->start + idx) % (scr->cap + 1);
}

static inline RLine *get_rline(Screen *scr, size_t idx){
    return scr->rlines[rlines_idx(scr, idx)];
}

// convert absolute index to a view index, or what is currently in scroll view
static inline size_t abs2view(Term *t, size_t idx){
    return t->scr->len - t->row - t->scroll + idx;
}

static inline size_t view2abs(Term *t, size_t idx){
    return idx + (t->scr->len - t->row - t->scroll);
}

// convert absolute index to a terminal index, or the bottom t->row rows
static inline size_t abs2term_ex(Screen *scr, size_t idx, int t_row){
    return idx - (scr->len - t_row);
}
static inline size_t abs2term(Term *t, size_t idx){
    return abs2term_ex(t->scr, idx, t->row);
}

static inline size_t term2abs_ex(Screen *scr, size_t idx, int t_row){
    return idx + (scr->len - t_row);
}
static inline size_t term2abs(Term *t, size_t idx){
    return term2abs_ex(t->scr, idx, t->row);
}

static inline uint64_t new_line_id(Screen *scr){
    return ++scr->line_id;
}

static inline RLine *get_cursor_rline(Term *t){
    return get_rline(t->scr, term2abs(t, t->c.y));
}

static uchar utfbyte[UTF_SIZ + 1] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static uchar utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static Rune utfmin[UTF_SIZ + 1] = {       0,    0,  0x80,  0x800,  0x10000};
static Rune utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

ssize_t
xwrite(int fd, const char *s, size_t len)
{
    size_t aux = len;
    ssize_t r;

    while (len > 0) {
        r = write(fd, s, len);
        if (r < 0)
            return r;
        len -= r;
        s += r;
    }

    return aux;
}

void *
xmalloc(size_t len)
{
    void *p;

    if (!(p = malloc(len)))
        die("malloc: %s\n", strerror(errno));

    return p;
}

void *
xrealloc(void *p, size_t len)
{
    if ((p = realloc(p, len)) == NULL)
        die("realloc: %s\n", strerror(errno));

    return p;
}

char *
xstrdup(char *s)
{
    if ((s = strdup(s)) == NULL)
        die("strdup: %s\n", strerror(errno));

    return s;
}

size_t
utf8decode(const char *c, Rune *u, size_t clen)
{
    size_t i, j, len, type;
    Rune udecoded;

    *u = UTF_INVALID;
    if (!clen)
        return 0;
    udecoded = utf8decodebyte(c[0], &len);
    if (!BETWEEN(len, 1, UTF_SIZ))
        return 1;
    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    utf8validate(u, len);

    return len;
}

Rune
utf8decodebyte(char c, size_t *i)
{
    for (*i = 0; *i < LEN(utfmask); ++(*i))
        if (((uchar)c & utfmask[*i]) == utfbyte[*i])
            return (uchar)c & ~utfmask[*i];

    return 0;
}

size_t
utf8encode(Rune u, char *c)
{
    size_t len, i;

    len = utf8validate(&u, 0);
    if (len > UTF_SIZ)
        return 0;

    for (i = len - 1; i != 0; --i) {
        c[i] = utf8encodebyte(u, 0);
        u >>= 6;
    }
    c[0] = utf8encodebyte(u, len);

    return len;
}

char
utf8encodebyte(Rune u, size_t i)
{
    return utfbyte[i] | (u & ~utfmask[i]);
}

size_t
utf8validate(Rune *u, size_t i)
{
    if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
        *u = UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i)
        ;

    return i;
}

static const char base64_digits[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0,
    63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, -1, 0, 0, 0, 0, 1,
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char
base64dec_getc(const char **src)
{
    while (**src && !isprint(**src)) (*src)++;
    return **src ? *((*src)++) : '=';  /* emulate padding if string ends */
}

char *
base64dec(const char *src, size_t *len)
{
    size_t in_len = strlen(src);
    char *result, *dst;

    if (in_len % 4)
        in_len += 4 - (in_len % 4);
    result = dst = xmalloc(in_len / 4 * 3 + 1);
    while (*src) {
        int a = base64_digits[(unsigned char) base64dec_getc(&src)];
        int b = base64_digits[(unsigned char) base64dec_getc(&src)];
        int c = base64_digits[(unsigned char) base64dec_getc(&src)];
        int d = base64_digits[(unsigned char) base64dec_getc(&src)];

        /* invalid input. 'a' can be -1, e.g. if src is "\n" (c-str) */
        if (a == -1 || b == -1)
            break;

        *dst++ = (a << 2) | ((b & 0x30) >> 4);
        if (c == -1)
            break;
        *dst++ = ((b & 0x0f) << 4) | ((c & 0x3c) >> 2);
        if (d == -1)
            break;
        *dst++ = ((c & 0x03) << 6) | d;
    }
    *len = (size_t)(dst - result);
    *dst = '\0';
    return result;
}

// int
// tlinelen(Term *t, int y)
// {
//     int i = t->col;
//
//     die("tlinelen!\n");
//     // if (t->line[y][i - 1].mode & ATTR_WRAP)
//     //     return i;
//
//     // while (i > 0 && t->line[y][i - 1].u == ' ')
//     //     --i;
//
//     return i;
// }

void
die(const char *errstr, ...)
{
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(1);
}

void
execsh(char *cmd, char **args)
{
    char *sh, *prog;
    const struct passwd *pw;

    errno = 0;
    if ((pw = getpwuid(getuid())) == NULL) {
        if (errno)
            die("getpwuid: %s\n", strerror(errno));
        else
            die("who are you?\n");
    }

    if ((sh = getenv("SHELL")) == NULL)
        sh = (pw->pw_shell[0]) ? pw->pw_shell : cmd;

    if (args)
        prog = args[0];
    else if (utmp)
        prog = utmp;
    else
        prog = sh;
    DEFAULT(args, ((char *[]) {prog, NULL}));

    unsetenv("COLUMNS");
    unsetenv("LINES");
    unsetenv("TERMCAP");
    setenv("LOGNAME", pw->pw_name, 1);
    setenv("USER", pw->pw_name, 1);
    setenv("SHELL", sh, 1);
    setenv("HOME", pw->pw_dir, 1);
    setenv("TERM", termname, 1);

    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGALRM, SIG_DFL);

    execvp("/bin/sh", (char*[]){"/bin/sh", NULL});
    execvp(prog, args);
    _exit(1);
}

void
stty(char **args)
{
    char cmd[_POSIX_ARG_MAX], **p, *q, *s;
    size_t n, siz;

    if ((n = strlen(stty_args)) > sizeof(cmd)-1)
        die("incorrect stty parameters\n");
    memcpy(cmd, stty_args, n);
    q = cmd + n;
    siz = sizeof(cmd) - n;
    for (p = args; p && (s = *p); ++p) {
        if ((n = strlen(s)) > siz-1)
            die("stty parameter length too long\n");
        *q++ = ' ';
        memcpy(q, s, n);
        q += n;
        siz -= n + 1;
    }
    *q = '\0';
    if (system(cmd) != 0)
        perror("Couldn't call stty");
}

int
ttynew(Term *t, pid_t *pid, char *line, char *cmd, char *out, char **args)
{
    int m, s;

    if (out) {
        t->mode |= MODE_PRINT;
        iofd = (!strcmp(out, "-")) ?
              1 : open(out, O_WRONLY | O_CREAT, 0666);
        if (iofd < 0) {
            fprintf(stderr, "Error opening %s:%s\n",
                out, strerror(errno));
        }
    }

    if (line) {
        if ((t->cmdfd = open(line, O_RDWR)) < 0)
            die("open line '%s' failed: %s\n",
                line, strerror(errno));
        dup2(t->cmdfd, 0);
        stty(args);
        return t->cmdfd;
    }

    /* seems to work fine on linux, openbsd and freebsd */
    if (openpty(&m, &s, NULL, NULL, NULL) < 0)
        die("openpty failed: %s\n", strerror(errno));

    // configure the terminal with 0x08 as the backspace key, like xterm
    struct termios ttyattr;
    if(tcgetattr(m, &ttyattr)){
        die("tcgetattr() failed: %s\n", strerror(errno));
    }
    ttyattr.c_cc[VERASE] = '\b';
    if(tcsetattr(m, TCSADRAIN, &ttyattr)){
        die("tcsetattr() failed: %s\n", strerror(errno));
    }

    switch (*pid = fork()) {
    case -1:
        die("fork failed: %s\n", strerror(errno));
        break;
    case 0:
        close(iofd);
        setsid(); /* create a new process group */
        dup2(s, 0);
        dup2(s, 1);
        dup2(s, 2);
        if (ioctl(s, TIOCSCTTY, NULL) < 0)
            die("ioctl TIOCSCTTY failed: %s\n", strerror(errno));
        close(s);
        close(m);
#ifdef __OpenBSD__
        if (pledge("stdio getpw proc exec", NULL) == -1)
            die("pledge\n");
#endif
        execsh(cmd, args);
        break;
    default:
#ifdef __OpenBSD__
        if (pledge("stdio rpath tty proc", NULL) == -1)
            die("pledge\n");
#endif
        close(s);
        t->cmdfd = m;
        break;
    }
    return t->cmdfd;
}

size_t
ttyread(Term *t)
{
    /* append read bytes to unprocessed bytes */
    ssize_t ret = read(
        t->cmdfd,
        t->ttyreadbuf+t->ttyreadbuflen,
        LEN(t->ttyreadbuf)-t->ttyreadbuflen
    );
    if(ret < 0) die("couldn't read from tty: %s\n", strerror(errno));
    t->ttyreadbuflen += ret;

    size_t written = twrite(t, t->ttyreadbuf, t->ttyreadbuflen, 0);
    t->ttyreadbuflen -= written;
    /* keep any uncomplete utf8 char for the next call */
    if (t->ttyreadbuflen > 0)
        memmove(t->ttyreadbuf, t->ttyreadbuf + written, t->ttyreadbuflen);

    return ret;
}

bool
t_isset_crlf(Term *t){
    return IS_SET(t, MODE_CRLF);
}

bool
t_isset_echo(Term *t){
    return IS_SET(t, MODE_ECHO);
}

int
tattrset(Term *t, int attr)
{
    die("tattrset!\n");
    // int i, j;

    // for (i = 0; i < t->row-1; i++) {
    //     for (j = 0; j < t->col-1; j++) {
    //         if (t->line[i][j].mode & attr)
    //             return 1;
    //     }
    // }

    return 0;
}

void
tcursor(Term *t, int mode)
{
    int alt = IS_SET(t, MODE_ALTSCREEN);

    if (mode == CURSOR_SAVE) {
        t->saved[alt] = t->c;
    } else if (mode == CURSOR_LOAD) {
        t->c = t->saved[alt];
        tmoveto(t, t->saved[alt].x, t->saved[alt].y);
    }
}

int
tcursorstyle(Term *t, int style)
{
    // DECSCUSR: Set Cursor Style:
    // https://vt100.net/docs/vt510-rm/DECSCUSR.html
    switch(style){
    case 0:
    case 1:
        t->cursor_style = CURSOR_BLOCK_BLINK;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        t->cursor_style = style;
        break;
    default:
        return -1;
    }
    return 0;
}

void
treset(Term *t)
{
    uint i;

    t->c = (TCursor){{
        .mode = ATTR_NULL,
        .fg = defaultfg,
        .bg = defaultbg
    }, .x = 0, .y = 0, .state = CURSOR_DEFAULT};

    memset(t->tabs, 0, t->col * sizeof(*t->tabs));
    for (i = tabspaces; i < t->col; i += tabspaces)
        t->tabs[i] = 1;
    t->top = 0;
    t->bot = t->row - 1;
    t->mode = MODE_WRAP|MODE_UTF8;
    memset(t->trantbl, CS_USA, sizeof(t->trantbl));
    t->charset = 0;

    for (i = 0; i < 2; i++) {
        tmoveto(t, 0, 0);
        tcursor(t, CURSOR_SAVE);
        tclearregion_abs(t, 0, 0, t->col-1, t->scr->len - 1);
        tswapscreen(t);
    }
}

int
tfont(
    Term *t,
    char *font_name,
    PangoFontDescription **desc_out,
    double *grid_w_out,
    double *grid_h_out
){
    /* Check that we can get a font description from the font name and make
       sure it is monospaced.  Output the description and the grid size.
       Return 0 on success or -1 on error. */
    PangoFontDescription *desc = pango_font_description_from_string(font_name);
    if(!desc){
        fprintf(stderr, "unable to process font \"%s\"\n", font_name);
        return -1;
    }

    // This code segfaults for no apparent reason:
    // {
    //     const PangoFontFamily *family =
    //         pango_font_description_get_family(desc);
    //     gboolean ans = pango_font_family_is_monospace(family);
    // }

    // hack: check monospace by comparing layout width of 'm' vs 'i'
    int w = 1024;
    int h = 256;
    cairo_surface_t *srfc =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
    if(!srfc){
        die("cairo_image_surface_create(): %s\n", strerror(errno));
    }

    cairo_t *cr = cairo_create(srfc);
    if(!cr){
        die("cairo_create(): %s\n", strerror(errno));
    }

    PangoLayout *layout = pango_cairo_create_layout(cr);
    if(!layout){
        die("pango_cairo_create_layout(): %s\n", strerror(errno));
    }

    PangoRectangle rect;

    pango_layout_set_font_description(layout, desc);
    pango_layout_set_text(layout, "Mm", 2);
    pango_layout_get_extents(layout, NULL, &rect);
    int m_w = rect.width;

    pango_layout_set_font_description(layout, desc);
    pango_layout_set_text(layout, "Ii", 2);
    pango_layout_get_extents(layout, NULL, &rect);
    int i_w = rect.width;

    if(m_w != i_w){
        fprintf(stderr, "non-monospace font detected: \"%s\"\n", font_name);
        goto fail;
    }

    *grid_w_out = ((double)m_w) / 2 / PANGO_SCALE;

    // now measure height
    pango_layout_set_text(layout, "Áy", 2);
    pango_layout_get_extents(layout, NULL, &rect);
    *grid_h_out = ((double)rect.height) / PANGO_SCALE;

    *desc_out = desc;
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(srfc);

    return 0;

fail:
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(srfc);
    pango_font_description_free(desc);
    return -1;
}

void
tnew(Term **tout, int col, int row, char *font_name, THooks *hooks)
{
    Term *t = xmalloc(sizeof(Term));
    *t = (Term){
        .c = {
            .attr = { .fg = defaultfg, .bg = defaultbg }
        },
        .hooks = hooks,
    };

    int ret = tfont(t, font_name, &t->desc, &t->grid_w, &t->grid_h);
    if(ret < 0){
        die("invalid font\n");
    }

    // allocate history (primary screen, lots of scrollback)
    t->main.cap = RLINES_LIMIT - 1;
    size_t nbytes = (t->main.cap + 1) * sizeof(*t->main.rlines);
    t->main.rlines = xmalloc(nbytes);
    // all lines start empty
    memset(t->main.rlines, 0, nbytes);
    // start with the first window of lines allocated
    for(size_t i = 0; i < row; i++){
        t->main.rlines[rlines_idx(&t->main, t->main.len++)] =
            rline_new(col, 0);
    }
    // the first line needs a real line id, since the cursor starts there
    t->main.rlines[rlines_idx(&t->main, 0)]->line_id = new_line_id(&t->main);

    // allocate history (altscreen, zero scrollback)
    t->alt.cap = row;
    nbytes = (t->alt.cap + 1) * sizeof(*t->alt.rlines);
    t->alt.rlines = xmalloc(nbytes);
    memset(t->alt.rlines, 0, nbytes);
    for(size_t i = 0; i < row; i++){
        t->alt.rlines[rlines_idx(&t->alt, t->alt.len++)] =
            rline_new(col, 0);
    }
    t->alt.rlines[rlines_idx(&t->alt, 0)]->line_id = new_line_id(&t->alt);

    // start on main screen
    t->scr = &t->main;

    t->row = row;
    t->col = col;

    t->tabs = xrealloc(t->tabs, col * sizeof(*t->tabs));

    tscrollregion(t, 0, row-1);
    treset(t);

    *tout = t;
}

void
tswapscreen(Term *t)
{
    if(t->mode & MODE_ALTSCREEN){
        // switch to main
        t->scr = &t->main;
    }else{
        // switch to alt
        t->scr = &t->alt;
    }
    t->mode ^= MODE_ALTSCREEN;
}

void
tnewline(Term *t, int first_col, bool continue_line)
{
    uint64_t line_id;
    if(continue_line){
        line_id = get_cursor_rline(t)->line_id;
    }else{
        line_id = new_line_id(t->scr);
    }

    // move the cursor down a line
    t->c.y++;

    // do we need a new line?
    if(t->c.y == t->row){
        t->c.y--;
        scr_new_rline(t->scr, line_id, t->col);

        // hold the scroll window in place, if needed and possible
        if(t->scroll){
            t->scroll++;
            LIMIT(t->scroll, 0, t->scr->len - t->row);
        }
    }

    tmoveto(t, first_col ? 0 : t->c.x, t->c.y);

    // update the line_id
    get_cursor_rline(t)->line_id = line_id;
}

void
csiparse(void)
{
    char *p = csiescseq.buf, *np;
    long int v;

    csiescseq.narg = 0;
    if (*p == '?' || *p == '>') {
        csiescseq.priv = *p;
        p++;
    }

    csiescseq.buf[csiescseq.len] = '\0';
    while (p < csiescseq.buf+csiescseq.len) {
        np = NULL;
        v = strtol(p, &np, 10);
        if (np == p)
            v = 0;
        if (v == LONG_MAX || v == LONG_MIN)
            v = -1;
        csiescseq.arg[csiescseq.narg++] = v;
        p = np;
        if (*p != ';' || csiescseq.narg == ESC_ARG_SIZ)
            break;
        p++;
    }
    // detect when there is a submode
    if(p - csiescseq.buf + 1 < csiescseq.len){
        csiescseq.submode = *p++;
        csiescseq.mode = *p;
    }else{
        csiescseq.submode = '\0';
        csiescseq.mode = *p;
    }
}

// uses terminal coordinates
void
tmoveto(Term *t, int x, int y)
{
    t->c.state &= ~CURSOR_WRAPNEXT;
    t->c.x = LIMIT(x, 0, t->col-1);
    t->c.y = LIMIT(y, 0, t->row-1);
}

// uses terminal coordinates, modded by the scroll region in origin mode
void
tmoveto_origin(Term *t, int x, int y)
{
    tmoveto(t, x, y + ((t->c.state & CURSOR_ORIGIN) ? t->top: 0));
}

// see man 5 terminfo ("Line Graphics")
Rune
acsc(Rune u, int charset)
{
    if(charset != CS_GRAPHIC0){
        return u;
    }

    /* The following table was found by running the following python code
       inside of an xterm window, and copy/pasting the xterm-rendered result:

        import os
        os.write(1, b"    static char *acsc[] = {")
        for i in range(96, 127):
            os.write(1, b"        \"")
            os.system("tput smacs")
            os.write(1, chr(i).encode())
            os.system("tput rmacs")
            os.write(1, b"\",  // 0x%x"%i)
            if i > 32 and i < 127:
                os.write(1, b" (%s)"%chr(i).encode())
            else:
                os.write(1, b"    ")
            os.write(1, b"\n")
        os.write(1, b"    };")

       Note that the ascs string from `infocmp xterm` indicates that b-e are
       not supported, xterm honors them anyway.  We choose to do the same. */
    static char *acsc[] = {
        "◆",  // 0x60 (`)
        "▒",  // 0x61 (a)
        "␉",  // 0x62 (b)
        "␌",  // 0x63 (c)
        "␍",  // 0x64 (d)
        "␊",  // 0x65 (e)
        "°",  // 0x66 (f)
        "±",  // 0x67 (g)
        "␤",  // 0x68 (h)
        "␋",  // 0x69 (i)
        "┘",  // 0x6a (j)
        "┐",  // 0x6b (k)
        "┌",  // 0x6c (l)
        "└",  // 0x6d (m)
        "┼",  // 0x6e (n)
        "⎺",  // 0x6f (o)
        "⎻",  // 0x70 (p)
        "─",  // 0x71 (q)
        "⎼",  // 0x72 (r)
        "⎽",  // 0x73 (s)
        "├",  // 0x74 (t)
        "┤",  // 0x75 (u)
        "┴",  // 0x76 (v)
        "┬",  // 0x77 (w)
        "│",  // 0x78 (x)
        "≤",  // 0x79 (y)
        "≥",  // 0x7a (z)
        "π",  // 0x7b ({)
        "≠",  // 0x7c (|)
        "£",  // 0x7d (})
        "·",  // 0x7e (~)
    };
    static size_t acsc_len = sizeof(acsc)/sizeof(*acsc);

    if(u < '`' || u - '`' > acsc_len){
        return u;
    }

    Rune out;
    utf8decode(acsc[u - '`'], &out, UTF_SIZ);

    return out;
}

// uses terminal coordinates
void
tsetchar(Term *t, Rune u, Glyph *attr, int x, int y)
{
    die("update tsetchar");
}

void
tclearregion_abs(Term *t, int x1, int y1, int x2, int y2)
{
    int x, y, temp;

    if (x1 > x2)
        temp = x1, x1 = x2, x2 = temp;
    if (y1 > y2)
        temp = y1, y1 = y2, y2 = temp;

    for (y = y1; y <= y2; y++) {
        RLine *rline = get_rline(t->scr, y);
        rline_unrender(rline);

        for (x = x1; x <= x2; x++) {
            // TODO:SELECTIONS
            // if (selected(t, x, y))
            //     selclear(t);
            rline->glyphs[x] = (Glyph){
                // default rune is ' ' so that cursor-on-empty-space works
                .u = ' ',
                .mode = ATTR_NORENDER,
                .fg = t->c.attr.fg,
                .bg = t->c.attr.bg,
            };
        }
    }
}

void
tclearregion_term(Term *t, int x1, int y1, int x2, int y2)
{
    LIMIT(x1, 0, t->col-1);
    LIMIT(x2, 0, t->col-1);
    LIMIT(y1, 0, t->row-1);
    LIMIT(y2, 0, t->row-1);

    tclearregion_abs(t, x1, term2abs(t, y1), x2, term2abs(t, y2));
}

void
tdeletechar(Term *t, int n)
{
    int dst, src, size;

    LIMIT(n, 0, t->col - t->c.x);

    dst = t->c.x;
    src = t->c.x + n;
    size = t->col - src;
    RLine *rline = get_rline(t->scr, term2abs(t, t->c.y));

    Glyph *glyphs = rline->glyphs;
    memmove(&glyphs[dst], &glyphs[src], size * sizeof(*glyphs));
    tclearregion_term(t, t->col-n, t->c.y, t->col-1, t->c.y);
}

void
tinsertblank(Term *t, int n)
{
    die("update tinsertblank");
}

void
tinsertblankline(Term *t, int n)
{
    die("update tinsertblankline");
}

void
tdeleteline(Term *t, int n)
{
    die("update tdeleteline");
}

void
tscrollup(Term *t, int orig, int n)
{
    die("update tscrollup");
}

void
tscrolldown(Term *t, int orig, int n)
{
    die("update tscrolldown");
}

struct rgb24
tdefcolor(Term *t, int *attr, int *npar, int l, struct rgb24 fallback)
{
    uint r, g, b;

    switch (attr[*npar + 1]) {
    case 2: /* direct color in RGB space */
        if (*npar + 4 >= l) {
            fprintf(stderr,
                "erresc(38): Incorrect number of parameters (%d)\n",
                *npar);
            break;
        }
        r = attr[*npar + 2];
        g = attr[*npar + 3];
        b = attr[*npar + 4];
        *npar += 4;
        if (!BETWEEN(r, 0, 255) || !BETWEEN(g, 0, 255) || !BETWEEN(b, 0, 255))
            fprintf(stderr, "erresc: bad rgb color (%u,%u,%u)\n",
                r, g, b);
        else
            return (struct rgb24){r, g, b};
        break;
    case 5: /* indexed color */
        if (*npar + 2 >= l) {
            fprintf(stderr,
                "erresc(38): Incorrect number of parameters (%d)\n",
                *npar);
            break;
        }
        *npar += 2;
        if (!BETWEEN(attr[*npar], 0, 255))
            fprintf(stderr, "erresc: bad fgcolor %d\n", attr[*npar]);
        else
            return rgb24_from_index(attr[*npar]);
        break;
    case 0: /* implemented defined (only foreground) */
    case 1: /* transparent */
    case 3: /* direct color in CMY space */
    case 4: /* direct color in CMYK space */
    default:
        fprintf(stderr,
                "erresc(38): gfx attr %d unknown\n", attr[*npar]);
        break;
    }

    return fallback;
}

void
tsetattr(Term *t, int *attr, int l)
{
    int i;

    for (i = 0; i < l; i++) {
        switch (attr[i]) {
        case 0:
            t->c.attr.mode &= ~(
                ATTR_BOLD       |
                ATTR_FAINT      |
                ATTR_ITALIC     |
                ATTR_UNDERLINE  |
                ATTR_BLINK      |
                ATTR_REVERSE    |
                ATTR_INVISIBLE  |
                ATTR_STRUCK     );
            t->c.attr.fg = defaultfg;
            t->c.attr.bg = defaultbg;
            break;
        case 1:
            t->c.attr.mode |= ATTR_BOLD;
            break;
        case 2:
            t->c.attr.mode |= ATTR_FAINT;
            break;
        case 3:
            t->c.attr.mode |= ATTR_ITALIC;
            break;
        case 4:
            t->c.attr.mode |= ATTR_UNDERLINE;
            break;
        case 5: /* slow blink */
            /* FALLTHROUGH */
        case 6: /* rapid blink */
            t->c.attr.mode |= ATTR_BLINK;
            break;
        case 7:
            t->c.attr.mode |= ATTR_REVERSE;
            break;
        case 8:
            t->c.attr.mode |= ATTR_INVISIBLE;
            break;
        case 9:
            t->c.attr.mode |= ATTR_STRUCK;
            break;
        case 22:
            t->c.attr.mode &= ~(ATTR_BOLD | ATTR_FAINT);
            break;
        case 23:
            t->c.attr.mode &= ~ATTR_ITALIC;
            break;
        case 24:
            t->c.attr.mode &= ~ATTR_UNDERLINE;
            break;
        case 25:
            t->c.attr.mode &= ~ATTR_BLINK;
            break;
        case 27:
            t->c.attr.mode &= ~ATTR_REVERSE;
            break;
        case 28:
            t->c.attr.mode &= ~ATTR_INVISIBLE;
            break;
        case 29:
            t->c.attr.mode &= ~ATTR_STRUCK;
            break;
        case 38:
            t->c.attr.fg = tdefcolor(t, attr, &i, l, t->c.attr.fg);
            break;
        case 39:
            t->c.attr.fg = defaultfg;
            break;
        case 48:
            t->c.attr.bg = tdefcolor(t, attr, &i, l, t->c.attr.bg);
            break;
        case 49:
            t->c.attr.bg = defaultbg;
            break;
        default:
            if (BETWEEN(attr[i], 30, 37)) {
                t->c.attr.fg = rgb24_from_index(attr[i] - 30);
            } else if (BETWEEN(attr[i], 40, 47)) {
                t->c.attr.bg = rgb24_from_index(attr[i] - 40);
            } else if (BETWEEN(attr[i], 90, 97)) {
                t->c.attr.fg = rgb24_from_index(attr[i] - 90 + 8);
            } else if (BETWEEN(attr[i], 100, 107)) {
                t->c.attr.bg = rgb24_from_index(attr[i] - 100 + 8);
            } else {
                fprintf(stderr,
                    "erresc(default): gfx attr %d unknown\n",
                    attr[i]);
                csidump();
            }
            break;
        }
    }
}

void
tscrollregion(Term *t, int top, int bot)
{
    int temp;
    LIMIT(top, 0, t->row-1);
    LIMIT(bot, 0, t->row-1);
    if(top > bot){
        temp = top;
        top = bot;
        bot = temp;
    }
    t->top = top;
    t->bot = bot;
}

void
tsetmode(Term *t, int priv, int set, int *args, int narg)
{
    int alt, *lim;

    for (lim = args + narg; args < lim; ++args) {
        if (priv) {
            switch (*args) {
            case 1: /* DECCKM -- Cursor key */
                t->hooks->set_mode(t->hooks, MODE_APPCURSOR, set);
                break;
            case 5: /* DECSCNM -- Reverse video */
                t->hooks->set_mode(t->hooks, MODE_REVERSE, set);
                break;
            case 6: /* DECOM -- Origin */
                MODBIT(t->c.state, set, CURSOR_ORIGIN);
                tmoveto_origin(t, 0, 0);
                break;
            case 7: /* DECAWM -- Auto wrap */
                MODBIT(t->mode, set, MODE_WRAP);
                break;
            case 0:  /* Error (IGNORED) */
            case 2:  /* DECANM -- ANSI/VT52 (IGNORED) */
            case 3:  /* DECCOLM -- Column  (IGNORED) */
            case 4:  /* DECSCLM -- Scroll (IGNORED) */
            case 8:  /* DECARM -- Auto repeat (IGNORED) */
            case 18: /* DECPFF -- Printer feed (IGNORED) */
            case 19: /* DECPEX -- Printer extent (IGNORED) */
            case 42: /* DECNRCM -- National characters (IGNORED) */
            case 12: /* att610 -- Start blinking cursor (IGNORED) */
                break;
            case 25: /* DECTCEM -- Text Cursor Enable Mode */
                t->hooks->set_mode(t->hooks, MODE_HIDE, !set);
                break;
            case 9:    /* X10 mouse compatibility mode */
                // xsetpointermotion(0);
                t->hooks->set_mode(t->hooks, MODE_MOUSE, 0);
                t->hooks->set_mode(t->hooks, MODE_MOUSEX10, set);
                break;
            case 1000: /* 1000: report button press */
                // xsetpointermotion(0);
                t->hooks->set_mode(t->hooks, MODE_MOUSE, 0);
                t->hooks->set_mode(t->hooks, MODE_MOUSEBTN, set);
                break;
            case 1002: /* 1002: report motion on button press */
                // xsetpointermotion(0);
                t->hooks->set_mode(t->hooks, MODE_MOUSE, 0);
                t->hooks->set_mode(t->hooks, MODE_MOUSEMOTION, set);
                break;
            case 1003: /* 1003: enable all mouse motions */
                // xsetpointermotion(set);
                t->hooks->set_mode(t->hooks, MODE_MOUSE, 0);
                t->hooks->set_mode(t->hooks, MODE_MOUSEMANY, set);
                break;
            case 1004: /* 1004: send focus events to tty */
                t->hooks->set_mode(t->hooks, MODE_FOCUS, set);
                break;
            case 1006: /* 1006: extended reporting mode */
                t->hooks->set_mode(t->hooks, MODE_MOUSESGR, set);
                break;
            case 1034:
                t->hooks->set_mode(t->hooks, MODE_8BIT, set);
                break;
            case 1049: /* swap screen & set/restore cursor as xterm */
                tcursor(t, (set) ? CURSOR_SAVE : CURSOR_LOAD);
                /* FALLTHROUGH */
            case 47: /* swap screen */
            case 1047:
                alt = IS_SET(t, MODE_ALTSCREEN);
                if (alt) {
                    tclearregion_abs(t, 0, 0, t->col-1, t->scr->len-1);
                }
                if (set ^ alt) /* set is always 1 or 0 */
                    tswapscreen(t);
                if (*args != 1049)
                    break;
                /* FALLTHROUGH */
            case 1048:
                tcursor(t, (set) ? CURSOR_SAVE : CURSOR_LOAD);
                break;
            case 2004: /* 2004: bracketed paste mode */
                // see https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
                // search for "Ps = 2 0 0 4"
                MODBIT(t->mode, set, MODE_BRCKTPASTE);
                break;
            /* Not implemented mouse modes. See comments there. */
            case 1001: /* mouse highlight mode; can hang the
                      terminal by design when implemented. */
            case 1005: /* UTF-8 mouse mode; will confuse
                      applications not supporting UTF-8
                      and luit. */
            case 1015: /* urxvt mangled mouse mode; incompatible
                      and can be mistaken for other control
                      codes. */
                break;
            default:
                fprintf(stderr,
                    "erresc: unknown private set/reset mode %d\n",
                    *args);
                break;
            }
        } else {
            switch (*args) {
            case 0:  /* Error (IGNORED) */
                break;
            case 2:
                t->hooks->set_mode(t->hooks, MODE_KBDLOCK, set);
                break;
            case 4:  /* IRM -- Insertion-replacement */
                MODBIT(t->mode, set, MODE_INSERT);
                break;
            case 12: /* SRM -- Send/Receive */
                MODBIT(t->mode, !set, MODE_ECHO);
                break;
            case 20: /* LNM -- Linefeed/new line */
                MODBIT(t->mode, set, MODE_CRLF);
                break;
            default:
                fprintf(stderr,
                    "erresc: unknown set/reset mode %d\n",
                    *args);
                break;
            }
        }
    }
}

void
csihandle(Term *t)
{
    char buf[40];
    int len;

    switch (csiescseq.mode) {
    case '@': /* ICH -- Insert <n> blank char */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tinsertblank(t, csiescseq.arg[0]);
        break;
    case 'A': /* CUU -- Cursor <n> Up */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto(t, t->c.x, t->c.y-csiescseq.arg[0]);
        break;
    case 'B': /* CUD -- Cursor <n> Down */
    case 'e': /* VPR --Cursor <n> Down */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto(t, t->c.x, t->c.y+csiescseq.arg[0]);
        break;
    case 'i': /* MC -- Media Copy */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        switch (csiescseq.arg[0]) {
        case 0:
            tdump(t);
            break;
        case 1:
            tdumpline(t, t->c.y);
            break;
        case 2:
            tdumpsel(t);
            break;
        case 4:
            t->mode &= ~MODE_PRINT;
            break;
        case 5:
            t->mode |= MODE_PRINT;
            break;
        }
        break;
    case 'c': /* DA -- Device Attributes */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        if (csiescseq.arg[0] == 0)
            t->hooks->ttywrite(t->hooks, vtiden, strlen(vtiden));
        break;
    case 'C': /* CUF -- Cursor <n> Forward */
    case 'a': /* HPR -- Cursor <n> Forward */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto(t, t->c.x+csiescseq.arg[0], t->c.y);
        break;
    case 'D': /* CUB -- Cursor <n> Backward */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto(t, t->c.x-csiescseq.arg[0], t->c.y);
        break;
    case 'E': /* CNL -- Cursor <n> Down and first col */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto(t, 0, t->c.y+csiescseq.arg[0]);
        break;
    case 'F': /* CPL -- Cursor <n> Up and first col */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto(t, 0, t->c.y-csiescseq.arg[0]);
        break;
    case 'g': /* TBC -- Tabulation clear */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        switch (csiescseq.arg[0]) {
        case 0: /* clear current tab stop */
            t->tabs[t->c.x] = 0;
            break;
        case 3: /* clear all the tabs */
            memset(t->tabs, 0, t->col * sizeof(*t->tabs));
            break;
        default:
            goto unknown;
        }
        break;
    case 'G': /* CHA -- Move to <col> */
    case '`': /* HPA */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto(t, csiescseq.arg[0]-1, t->c.y);
        break;
    case 'H': /* CUP -- Move to <row> <col> */
    case 'f': /* HVP */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        DEFAULT(csiescseq.arg[1], 1);
        tmoveto_origin(t, csiescseq.arg[1]-1, csiescseq.arg[0]-1);
        break;
    case 'I': /* CHT -- Cursor Forward Tabulation <n> tab stops */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tputtab(t, csiescseq.arg[0]);
        break;
    case 'J': /* ED -- Clear screen */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        switch (csiescseq.arg[0]) {
        case 0: /* below */
            tclearregion_term(t, t->c.x, t->c.y, t->col-1, t->c.y);
            if (t->c.y < t->row-1) {
                tclearregion_term(t, 0, t->c.y+1, t->col-1, t->row-1);
            }
            break;
        case 1: /* above */
            if (t->c.y > 1){
                tclearregion_term(t, 0, 0, t->col-1, t->c.y-1);
            }
            tclearregion_term(t, 0, t->c.y, t->c.x, t->c.y);
            break;
        case 2: /* all */
            tclearregion_term(t, 0, 0, t->col-1, t->row-1);
            break;
        case 3: /* xterm extension: clear screen and scrollback buffer */
            tclearregion_abs(t, 0, 0, t->col-1, t->scr->len - 1);
            break;
        default:
            goto unknown;
        }
        break;
    case 'K': /* EL -- Clear line */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        switch (csiescseq.arg[0]) {
        case 0: /* right */
            tclearregion_term(t, t->c.x, t->c.y, t->col-1, t->c.y);
            break;
        case 1: /* left */
            tclearregion_term(t, 0, t->c.y, t->c.x, t->c.y);
            break;
        case 2: /* all */
            tclearregion_term(t, 0, t->c.y, t->col-1, t->c.y);
            break;
        }
        break;
    case 'S': /* SU -- Scroll <n> line up */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tscrollup(t, t->top, csiescseq.arg[0]);
        break;
    case 'T': /* SD -- Scroll <n> line down */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tscrolldown(t, t->top, csiescseq.arg[0]);
        break;
    case 'L': /* IL -- Insert <n> blank lines */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tinsertblankline(t, csiescseq.arg[0]);
        break;
    case 'l': /* RM -- Reset Mode */
        if(csiescseq.priv && csiescseq.priv != '?') goto unknown;
        if(csiescseq.submode) goto unknown;
        tsetmode(t, csiescseq.priv, 0, csiescseq.arg, csiescseq.narg);
        break;
    case 'M': /* DL -- Delete <n> lines */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tdeleteline(t, csiescseq.arg[0]);
        break;
    case 'X': /* ECH -- Erase <n> char */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tclearregion_term(
            t, t->c.x, t->c.y, t->c.x + csiescseq.arg[0] - 1, t->c.y
        );
        break;
    case 'P': /* DCH -- Delete <n> char */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tdeletechar(t, csiescseq.arg[0]);
        break;
    case 'Z': /* CBT -- Cursor Backward Tabulation <n> tab stops */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tputtab(t, -csiescseq.arg[0]);
        break;
    case 'd': /* VPA -- Move to <row> */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        tmoveto_origin(t, t->c.x, csiescseq.arg[0]-1);
        break;
    case 'h': /* SM -- Set terminal mode */
        if(csiescseq.priv && csiescseq.priv != '?') goto unknown;
        if(csiescseq.submode) goto unknown;
        tsetmode(t, csiescseq.priv, 1, csiescseq.arg, csiescseq.narg);
        break;
    case 'm': /* SGR -- Terminal attribute (color) */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        tsetattr(t, csiescseq.arg, csiescseq.narg);
        break;
    case 'n': /* DSR – Device Status Report (cursor position) */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        if (csiescseq.arg[0] == 6) {
            len = snprintf(buf, sizeof(buf),"\033[%i;%iR", t->c.y+1, t->c.x+1);
            t->hooks->ttywrite(t->hooks, buf, len);
        }
        break;
    case 'r': /* DECSTBM -- Set Scrolling Region */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        DEFAULT(csiescseq.arg[0], 1);
        DEFAULT(csiescseq.arg[1], t->row);
        tscrollregion(t, csiescseq.arg[0]-1, csiescseq.arg[1]-1);
        tmoveto_origin(t, 0, 0);
        break;
    case 's': /* DECSC -- Save cursor position (ANSI.SYS) */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        tcursor(t, CURSOR_SAVE);
        break;
    case 'u': /* DECRC -- Restore cursor position (ANSI.SYS) */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        tcursor(t, CURSOR_LOAD);
        break;
    case 't': /* Window manipulation (XTWINOPS) */
        if(csiescseq.priv || csiescseq.submode) goto unknown;
        if(csiescseq.narg < 1) goto unknown;
        switch(csiescseq.arg[0]){
            case 1: // de-iconify window
            case 2: // iconify window
            case 3: // (x, y) ->  Move window to [x, y]
            case 4: /* (h, w) ->  resize to given h,w in pixels.
                       Omitted parameters reuse the current height or width.
                       Zero parameters use the display's height or width. */
            case 5: // Raise the window to the front of the stack.
            case 6: // Lower the window to the bottom of the stack.
            case 7: // Refresh window.
            case 8: /* (h, w) ->  resize to given h,w in characters.
                       Omitted parameters reuse the current height or width.
                       Zero parameters use the display's height or width. */
            case 9: /* 9;0 -> Restore maximized window
                       9;1 -> Maximize window
                       9;2 -> Maximize window vertically
                       9;3 -> Maximize window hoorizontally */
            case 10: /* 10;0 -> undo full-screen mode
                        10;1 -> change to full-screen mode
                        10;2 -> toggle full-screen mode */
            case 11: /* report xterm window state.
                        If non-iconified, it returns CSI 1 t
                        If iconified, it returns CSI 2 t */
            case 13: /* 13   -> report window position (note: X11 specific)
                        13;2 -> report text area position
                        Result is CSI 3 ; x ; y t */
            case 14: /* 14   -> Report xterm text area size in pixels
                        14;2 -> report window size in pixels
                        Result is CSI 4 ; h ; w t */

            case 15: /* Report size of the screen in pixels.
                        Result is CSI 5 ; h ; w t */
            case 16: /* Report xterm character cell size in pixels.
                        Result is CSI 6 ; h ; w t */
            case 18: /* Report size of text area in characters.
                        Result is CSI 8 ; h ; w t */
            case 19: /* Report size of the screen in characters.
                        Result is CSI 9 ; h ; w t */
            case 20: /* Report window icon label.
                        Result is OSC L label ST */
            case 21: /* Report window title.
                        Result is OSC l label ST */
                // these are commands we might respect if there is a need
                goto unknown;

            case 22: /* 22;0 -> Save icon and window title on stack.
                        22;1 -> Save icon title on stack.
                        22;2 -> Save window title on stack. */
            case 23: /* 23;0 ->  Restore icon and window title from stack
                        23;1 ->  Restore icon title from stack.
                        23;2 ->  Restore window title from stack. */
                // these are commands we don't care to support
                break;

            default:
                if(csiescseq.arg[0] >= 24){
                    // DECSLPP: resize to nrows=csiescseq.arg[0]
                    // xterm adapts this by resizing its window
                }else{
                    goto unknown;
                }
        }
        break;
    case 'q':
        if(csiescseq.priv) goto unknown;
        switch (csiescseq.submode) {
        case ' ':
            // DECSCUSR: Set Cursor Style:
            if(tcursorstyle(t, csiescseq.arg[0]))
                goto unknown;
            break;
        default:
            goto unknown;
        }
        break;

    default:
    unknown:
        fprintf(stderr, "erresc: unknown csi ");
        csidump();
        /* die(""); */
        break;
    }
}

void
csidump(void)
{
    size_t i;
    uint c;

    fprintf(stderr, "ESC[");
    for (i = 0; i < csiescseq.len; i++) {
        c = csiescseq.buf[i] & 0xff;
        if (isprint(c)) {
            putc(c, stderr);
        } else if (c == '\n') {
            fprintf(stderr, "(\\n)");
        } else if (c == '\r') {
            fprintf(stderr, "(\\r)");
        } else if (c == 0x1b) {
            fprintf(stderr, "(\\e)");
        } else {
            fprintf(stderr, "(%02x)", c);
        }
    }
    putc('\n', stderr);
}

void
csireset(void)
{
    memset(&csiescseq, 0, sizeof(csiescseq));
}

void
strhandle(Term *t)
{
    char *buf;
    int narg, par;

    t->esc &= ~(ESC_STR_END|ESC_STR);
    strparse();
    par = (narg = strescseq.narg) ? atoi(strescseq.args[0]) : 0;

    switch (strescseq.type) {
    case ']': /* OSC -- Operating System Command */
        switch (par) {
        case 0:
        case 1:
        case 2:
            if (narg > 1)
                t->hooks->set_title(t->hooks, strescseq.args[1]);
            return;
        case 52:
            if (narg > 2) {
                size_t len;
                buf = base64dec(strescseq.args[2], &len);
                if (buf) {
                    t->hooks->set_clipboard(t->hooks, buf, len);
                } else {
                    fprintf(stderr, "erresc: invalid base64\n");
                }
            }
            return;
        }
//// TODO: support custom colors eventually.
//         case 4: /* color set */
//             if (narg < 3)
//                 break;
//             p = strescseq.args[2];
//             /* FALLTHROUGH */
//         case 104: /* color reset, here p = NULL */
//             j = (narg > 1) ? atoi(strescseq.args[1]) : -1;
//             if (xsetcolorname(j, p)) {
//                 if (par == 104 && narg <= 1)
//                     return; /* color reset without parameter */
//                 fprintf(stderr, "erresc: invalid color j=%d, p=%s\n",
//                         j, p ? p : "(null)");
//             } else {
//                 /*
//                  * TODO if defaultbg color is changed, borders
//                  * are dirty
//                  */
//                 redraw();
//             }
//             return;
//         }
//         break;
    case 'k': /* old title set compatibility */
        t->hooks->set_title(t->hooks, strescseq.args[0]);
        return;
    case 'P': /* DCS -- Device Control String */
        t->mode |= ESC_DCS;
    case '_': /* APC -- Application Program Command */
    case '^': /* PM -- Privacy Message */
        return;
    }

    fprintf(stderr, "erresc: unknown str ");
    strdump();
}

void
strparse(void)
{
    int c;
    char *p = strescseq.buf;

    strescseq.narg = 0;
    strescseq.buf[strescseq.len] = '\0';

    if (*p == '\0')
        return;

    while (strescseq.narg < STR_ARG_SIZ) {
        strescseq.args[strescseq.narg++] = p;
        while ((c = *p) != ';' && c != '\0')
            ++p;
        if (c == '\0')
            return;
        *p++ = '\0';
    }
}

void
strdump(void)
{
    size_t i;
    uint c;

    fprintf(stderr, "ESC%c", strescseq.type);
    for (i = 0; i < strescseq.len; i++) {
        c = strescseq.buf[i] & 0xff;
        if (c == '\0') {
            putc('\n', stderr);
            return;
        } else if (isprint(c)) {
            putc(c, stderr);
        } else if (c == '\n') {
            fprintf(stderr, "(\\n)");
        } else if (c == '\r') {
            fprintf(stderr, "(\\r)");
        } else if (c == 0x1b) {
            fprintf(stderr, "(\\e)");
        } else {
            fprintf(stderr, "(%02x)", c);
        }
    }
    fprintf(stderr, "ESC\\\n");
}

void
strreset(void)
{
    strescseq = (STREscape){
        .buf = xrealloc(strescseq.buf, STR_BUF_SIZ),
        .siz = STR_BUF_SIZ,
    };
}

void
tprinter(Term *t, char *s, size_t len)
{
    if (iofd != -1 && xwrite(iofd, s, len) < 0) {
        perror("Error writing to output file");
        close(iofd);
        iofd = -1;
    }
}

void
toggleprinter(Term *t, const Arg *arg)
{
    t->mode ^= MODE_PRINT;
}

void
printscreen(Term *t, const Arg *arg)
{
    tdump(t);
}

void
printsel(Term *t, const Arg *arg)
{
    tdumpsel(t);
}

void
tdumpsel(Term *t)
{
    die("tdumpsel");
}

void
tdumpline(Term *t, int n)
{
    die("tdumpline");
    char buf[UTF_SIZ];
    Glyph *bp, *end;
    (void)bp;
    (void)end;
    (void)buf;

    // bp = &t->line[n][0];
    // end = &bp[MIN(tlinelen(t, n), t->col) - 1];
    // if (bp != end || bp->u != ' ') {
    //     for ( ;bp <= end; ++bp)
    //         tprinter(t, buf, utf8encode(bp->u, buf));
    // }
    // tprinter(t, "\n", 1);
}

void
tdump(Term *t)
{
    int i;

    for (i = 0; i < t->row; ++i)
        tdumpline(t, i);
}

void
tputtab(Term *t, int n)
{
    uint x = t->c.x;

    if (n > 0) {
        while (x < t->col && n--)
            for (++x; x < t->col && !t->tabs[x]; ++x)
                /* nothing */ ;
    } else if (n < 0) {
        while (x > 0 && n++)
            for (--x; x > 0 && !t->tabs[x]; --x)
                /* nothing */ ;
    }
    t->c.x = LIMIT(x, 0, t->col-1);
}

void
tdefutf8(Term *t, char ascii)
{
    if (ascii == 'G')
        t->mode |= MODE_UTF8;
    else if (ascii == '@')
        t->mode &= ~MODE_UTF8;
}

void
tdeftran(Term *t, char ascii)
{
    static char cs[] = "0B";
    static int vcs[] = {CS_GRAPHIC0, CS_USA};
    char *p;

    if ((p = strchr(cs, ascii)) == NULL) {
        fprintf(stderr, "esc unhandled charset: ESC ( %c\n", ascii);
    } else {
        t->trantbl[t->icharset] = vcs[p - cs];
    }
}

void
tdectest(Term *t, char c)
{
    int x, y;

    if (c == '8') { /* DEC screen alignment test. */
        for (x = 0; x < t->col; ++x) {
            for (y = 0; y < t->row; ++y)
                tsetchar(t, 'E', &t->c.attr, x, y);
        }
    }
}

void
tstrsequence(Term *t, uchar c)
{
    strreset();

    switch (c) {
    case 0x90:   /* DCS -- Device Control String */
        c = 'P';
        t->esc |= ESC_DCS;
        break;
    case 0x9f:   /* APC -- Application Program Command */
        c = '_';
        break;
    case 0x9e:   /* PM -- Privacy Message */
        c = '^';
        break;
    case 0x9d:   /* OSC -- Operating System Command */
        c = ']';
        break;
    }
    strescseq.type = c;
    t->esc |= ESC_STR;
}

void
tcontrolcode(Term *t, uchar ascii)
{
    switch (ascii) {
    case '\t':   /* HT */
        tputtab(t, 1);
        return;
    case '\b':   /* BS */
        tmoveto(t, t->c.x-1, t->c.y);
        return;
    case '\r':   /* CR */
        tmoveto(t, 0, t->c.y);
        return;
    case '\f':   /* LF */
    case '\v':   /* VT */
    case '\n':   /* LF */
        /* go to first col if the mode is set */
        tnewline(t, IS_SET(t, MODE_CRLF), false);
        return;
    case '\a':   /* BEL */
        if (t->esc & ESC_STR_END) {
            /* backwards compatibility to xterm */
            strhandle(t);
        } else {
            t->hooks->bell(t->hooks);
        }
        break;
    case '\033': /* ESC */
        csireset();
        t->esc &= ~(ESC_CSI|ESC_ALTCHARSET|ESC_TEST);
        t->esc |= ESC_START;
        return;
    case '\016': /* SO (LS1 -- Locking shift 1) */
    case '\017': /* SI (LS0 -- Locking shift 0) */
        t->charset = 1 - (ascii - '\016');
        return;
    case '\032': /* SUB */
        tsetchar(t, '?', &t->c.attr, t->c.x, t->c.y);
    case '\030': /* CAN */
        csireset();
        break;
    case '\005': /* ENQ (IGNORED) */
    case '\000': /* NUL (IGNORED) */
    case '\021': /* XON (IGNORED) */
    case '\023': /* XOFF (IGNORED) */
    case 0177:   /* DEL (IGNORED) */
        return;
    case 0x80:   /* TODO: PAD */
    case 0x81:   /* TODO: HOP */
    case 0x82:   /* TODO: BPH */
    case 0x83:   /* TODO: NBH */
    case 0x84:   /* TODO: IND */
        break;
    case 0x85:   /* NEL -- Next line */
        tnewline(t, 1, false); /* always go to first col */
        break;
    case 0x86:   /* TODO: SSA */
    case 0x87:   /* TODO: ESA */
        break;
    case 0x88:   /* HTS -- Horizontal tab stop */
        t->tabs[t->c.x] = 1;
        break;
    case 0x89:   /* TODO: HTJ */
    case 0x8a:   /* TODO: VTS */
    case 0x8b:   /* TODO: PLD */
    case 0x8c:   /* TODO: PLU */
    case 0x8d:   /* TODO: RI */
    case 0x8e:   /* TODO: SS2 */
    case 0x8f:   /* TODO: SS3 */
    case 0x91:   /* TODO: PU1 */
    case 0x92:   /* TODO: PU2 */
    case 0x93:   /* TODO: STS */
    case 0x94:   /* TODO: CCH */
    case 0x95:   /* TODO: MW */
    case 0x96:   /* TODO: SPA */
    case 0x97:   /* TODO: EPA */
    case 0x98:   /* TODO: SOS */
    case 0x99:   /* TODO: SGCI */
        break;
    case 0x9a:   /* DECID -- Identify Terminal */
        t->hooks->ttywrite(t->hooks, vtiden, strlen(vtiden));
        break;
    case 0x9b:   /* TODO: CSI */
    case 0x9c:   /* TODO: ST */
        break;
    case 0x90:   /* DCS -- Device Control String */
    case 0x9d:   /* OSC -- Operating System Command */
    case 0x9e:   /* PM -- Privacy Message */
    case 0x9f:   /* APC -- Application Program Command */
        tstrsequence(t, ascii);
        return;
    }
    /* only CAN, SUB, \a and C1 chars interrupt a sequence */
    t->esc &= ~(ESC_STR_END|ESC_STR);
}

/*
 * returns 1 when the sequence is finished and it hasn't to read
 * more characters for this sequence, otherwise 0
 */
int
eschandle(Term *t, uchar ascii)
{
    switch (ascii) {
    case '[':
        t->esc |= ESC_CSI;
        return 0;
    case '#':
        t->esc |= ESC_TEST;
        return 0;
    case '%':
        t->esc |= ESC_UTF8;
        return 0;
    case 'P': /* DCS -- Device Control String */
    case '_': /* APC -- Application Program Command */
    case '^': /* PM -- Privacy Message */
    case ']': /* OSC -- Operating System Command */
    case 'k': /* old title set compatibility */
        tstrsequence(t, ascii);
        return 0;
    case 'n': /* LS2 -- Locking shift 2 */
    case 'o': /* LS3 -- Locking shift 3 */
        t->charset = 2 + (ascii - 'n');
        break;
    case '(': /* GZD4 -- set primary charset G0 */
    case ')': /* G1D4 -- set secondary charset G1 */
    case '*': /* G2D4 -- set tertiary charset G2 */
    case '+': /* G3D4 -- set quaternary charset G3 */
        t->icharset = ascii - '(';
        t->esc |= ESC_ALTCHARSET;
        return 0;
    case 'D': /* IND -- Linefeed */
        if (t->c.y == t->bot) {
            tscrollup(t, t->top, 1);
        } else {
            tmoveto(t, t->c.x, t->c.y+1);
        }
        break;
    case 'E': /* NEL -- Next line */
        tnewline(t, 1, false); /* always go to first col */
        break;
    case 'H': /* HTS -- Horizontal tab stop */
        t->tabs[t->c.x] = 1;
        break;
    case 'M': /* RI -- Reverse index */
        if (t->c.y == t->top) {
            tscrolldown(t, t->top, 1);
        } else {
            tmoveto(t, t->c.x, t->c.y-1);
        }
        break;
    case 'Z': /* DECID -- Identify Terminal */
        t->hooks->ttywrite(t->hooks, vtiden, strlen(vtiden));
        break;
    case 'c': /* RIS -- Reset to initial state */
        treset(t);
        t->hooks->set_title(t->hooks, NULL);
        break;
    case '=': /* DECKPAM -- Application keypad */
        t->hooks->set_mode(t->hooks, MODE_APPKEYPAD, 1);
        break;
    case '>': /* DECKPNM -- Normal keypad */
        t->hooks->set_mode(t->hooks, MODE_APPKEYPAD, 0);
        break;
    case '7': /* DECSC -- Save Cursor */
        tcursor(t, CURSOR_SAVE);
        break;
    case '8': /* DECRC -- Restore Cursor */
        tcursor(t, CURSOR_LOAD);
        break;
    case '\\': /* ST -- String Terminator */
        if (t->esc & ESC_STR_END)
            strhandle(t);
        break;
    default:
        fprintf(stderr, "erresc: unknown sequence ESC 0x%02X '%c'\n",
            (uchar) ascii, isprint(ascii)? ascii:'.');
        break;
    }
    return 1;
}

/* handle one codepoint; either capture it as part of a sequence or emit it to
   the terminal */
void
tputc(Term *t, Rune u)
{
    char c[UTF_SIZ];
    int control;
    int width, len;
    // Glyph *gp;

    control = ISCONTROL(u);
    if (!IS_SET(t, MODE_UTF8) && !IS_SET(t, MODE_SIXEL)) {
        c[0] = u;
        width = len = 1;
    } else {
        len = utf8encode(u, c);
        if (!control && (width = wcwidth(u)) == -1) {
            memcpy(c, "\357\277\275", 4); /* UTF_INVALID */
            width = 1;
        }
    }

    if (IS_SET(t, MODE_PRINT))
        tprinter(t, c, len);

    /* STR sequence must be checked before anything else because it uses all
       following characters until it receives a ESC, a SUB, a ST or any other
       C1 control character. */
    if (t->esc & ESC_STR) {
        if (u == '\a' || u == 030 || u == 032 || u == 033 ||
           ISCONTROLC1(u)) {
            t->esc &= ~(ESC_START|ESC_STR|ESC_DCS);
            if (IS_SET(t, MODE_SIXEL)) {
                /* TODO: render sixel */;
                t->mode &= ~MODE_SIXEL;
                return;
            }
            t->esc |= ESC_STR_END;
            goto check_control_code;
        }

        if (IS_SET(t, MODE_SIXEL)) {
            /* TODO: implement sixel mode */
            return;
        }
        if (t->esc&ESC_DCS && strescseq.len == 0 && u == 'q')
            t->mode |= MODE_SIXEL;

        if (strescseq.len+len >= strescseq.siz) {
            /*
             * Here is a bug in terminals. If the user never sends
             * some code to stop the str or esc command, then st
             * will stop responding. But this is better than
             * silently failing with unknown characters. At least
             * then users will report back.
             *
             * In the case users ever get fixed, here is the code:
             */
            /*
             * t->esc = 0;
             * strhandle();
             */
            if (strescseq.siz > (SIZE_MAX - UTF_SIZ) / 2)
                return;
            strescseq.siz *= 2;
            strescseq.buf = xrealloc(strescseq.buf, strescseq.siz);
        }

        memmove(&strescseq.buf[strescseq.len], c, len);
        strescseq.len += len;
        return;
    }

check_control_code:
    /* Actions of control codes must be performed as soon they arrive
       because they can be embedded inside a control sequence, and
       they must not cause conflicts with sequences. */
    if (control) {
        tcontrolcode(t, u);
        // control codes are not shown ever
        return;
    }
    if (t->esc & ESC_START) {
        if (t->esc & ESC_CSI) {
            csiescseq.buf[csiescseq.len++] = u;
            if (BETWEEN(u, 0x40, 0x7E)
                    || csiescseq.len >= sizeof(csiescseq.buf)-1) {
                t->esc = 0;
                csiparse();
                csihandle(t);
            }
            return;
        } else if (t->esc & ESC_UTF8) {
            tdefutf8(t, u);
        } else if (t->esc & ESC_ALTCHARSET) {
            tdeftran(t, u);
        } else if (t->esc & ESC_TEST) {
            tdectest(t, u);
        } else {
            if (!eschandle(t, u))
                return;
            /* sequence already finished */
        }
        t->esc = 0;
        // All characters which form part of a sequence are not printed
        return;
    }
    if (sel.ob.x != -1 && BETWEEN(t->c.y, sel.ob.y, sel.oe.y)){
        // TODO:SELECTIONS
        // selclear(t);
    }

    u = acsc(u, t->trantbl[t->charset]);
    temit(t, u, width);
}


void
temit(Term *t, Rune u, int width)
{
    Glyph g = t->c.attr;
    g.u = u;

    RLine *rline = get_cursor_rline(t);

    if(t->c.state & CURSOR_WRAPNEXT){
        rline->glyphs[t->c.x].mode |= ATTR_WRAP;
        tnewline(t, 1, true);
        // the rline has changed
        rline = get_cursor_rline(t);
    }

    // TODO: handle double-width characters

    if(IS_SET(t, MODE_INSERT)){
        rline_insert_glyph(rline, t->c.x, g);
    }else{
        rline_set_glyph(rline, t->c.x, g);
    }

    if(t->c.x + width < t->col){
        tmoveto(t, t->c.x + width, t->c.y);
    }else{
        t->c.state |= CURSOR_WRAPNEXT;
    }
}


/*
Read a stream of utf-8, and call tputc on each codepoint.
*/
int
twrite(Term *t, const char *buf, int buflen, int show_ctrl)
{
    int charsize;
    Rune u;
    int n;

    for (n = 0; n < buflen; n += charsize) {
        if (IS_SET(t, MODE_UTF8) && !IS_SET(t, MODE_SIXEL)) {
            /* process a complete utf8 char */
            charsize = utf8decode(buf + n, &u, buflen - n);
            if (charsize == 0)
                break;
        } else {
            u = buf[n] & 0xFF;
            charsize = 1;
        }
        if (show_ctrl && ISCONTROL(u)) {
            if (u & 0x80) {
                u &= 0x7f;
                tputc(t, '^');
                tputc(t, '[');
            } else if (u != '\n' && u != '\r' && u != '\t') {
                u ^= 0x40;
                tputc(t, '^');
            }
        }
        tputc(t, u);
    }
    return n;
}

/* cursor reflow logic: as we reflow lines, we need to reflow the cursor's
   position as well; so keep track of the rline whose position is under the
   cursor, and figure out which y that points us to later */
typedef struct {
    TCursor old;
    const RLine *old_rline;
    int new_abs_y;
    TCursor new;
    bool done;
    bool invalid;
} cursor_reflow_t;

cursor_reflow_t cursor_reflow_new(TCursor old, const RLine *old_rline){
    // new is derived from old with clean x and WRAPNEXT
    TCursor new = old;
    new.x = 0;
    new.state &= ~CURSOR_WRAPNEXT;
    return(cursor_reflow_t){
        .old = old,
        .old_rline = old_rline,
        .new_abs_y = 0,
        .new = new,
    };
}

void cursor_reflow_copyable_glyph(
    cursor_reflow_t *r,
    const RLine *old,
    size_t old_x /*j*/,
    size_t new_x /*glyph_idx*/,
    const RLine *new,
    size_t y_abs
){
    // already reflowed?
    if(r->done) return;
    // wrong line?
    if(old != r->old_rline) return;
    // wrong column?
    if(old_x != r->old.x) return;

    r->new.x = new_x;
    r->new_abs_y = y_abs;
    // if the cursor was set to WRAPNEXT, bump x if it's safe to
    if(r->old.state & CURSOR_WRAPNEXT){
        if(r->new.x + 1 < new->n_glyphs){
            /* in the case that WRAPNEXT was set and the cursor
               position after reflowing is in the middle of the
               line, just bump the cursor forward a little bit */
            r->new.x++;
        }else{
            /* in the corner case where WRAPNEXT was set, but after
               resizing the cursor position is still right on the
               edge of the screen, just set WRAPNEXT again */
            r->new.state |= CURSOR_WRAPNEXT;
        }
    }

    r->done = true;
}

void cursor_reflow_noncopyable_glyph(
    cursor_reflow_t *r,
    const RLine *old,
    size_t new_x /*glyph_idx*/,
    const RLine *new,
    size_t y_abs
){
    // already reflowed?
    if(r->done) return;
    // wrong line?
    if(old != r->old_rline) return;

    // just place the cursor at the end of the line to support 99.99% of cases
    r->new.x = new_x;
    if(r->new.x >= new->n_glyphs){
        /* if placing the cursor after the last glyph would place it
           off of the screen, place it on top of the last glyph and
           set WRAPNEXT */
        r->new.x = new->n_glyphs - 1;
        r->new.state |= CURSOR_WRAPNEXT;
    }
    r->new_abs_y = y_abs;

    r->done = true;
}


size_t cursor_reflow_lines_to_trim(cursor_reflow_t *r, size_t rlines_len, int row){
    if(r->invalid) return 0;

    /* is the distance between our y_abs the bottom of the screen smaller than
       or equal to the number of rows in the screen? */
    if(rlines_len - r->new_abs_y < row) return 0;

    /* So suppose we're on the 0th row of a 40-row terminal, and we resize to
       a 39-row terminal, that means we trim (40 - 0) - 39 = 1 lines. */
    return rlines_len - r->new_abs_y - row;
}

// if new_abs_y-- won't be valid, mark the cursor_reflow as invalid
void cursor_reflow_decrement_y(cursor_reflow_t *r){
    if(!r->done){
        // y_abs not set yet
        return;
    }
    if(r->new_abs_y == 0){
        r->invalid = true;
        return;
    }
    r->new_abs_y--;
    return;
}

void cursor_reflow_invalidate_if_lines_to_trim(
    cursor_reflow_t *r, size_t rlines_len, int row
){
    if(cursor_reflow_lines_to_trim(r, rlines_len, row) > 0){
        r->invalid = true;
    }
}

TCursor cursor_reflow_done(cursor_reflow_t *r, Screen *scr, int row){
    if(r->invalid){
        // invalid cursors are just placed at 0,0
        r->new.x = 0;
        r->new.y = 0;
    }else{
        r->new.y = abs2term_ex(scr, r->new_abs_y, row);
    }
    return r->new;
}

static Screen reflow(
    Screen old,
    int old_col,
    int row,
    int col,
    size_t new_cap,
    cursor_reflow_t **crs,
    size_t ncrs
){
    // any time cap is less than row, just artificially bump it up
    if(new_cap < row){
        fprintf(
            stderr,
            "reflow(new_cap=%zu, row=%d): overriding new_cap\n",
            new_cap,
            row
        );
        new_cap = row;
    }

    // Start with the new_cap specified, and keep the line_id
    // TODO: can we avoid keeping the line_id?
    Screen new = {
        .cap = new_cap,
        .line_id = old.line_id,
        .start = 0,
        .len = 0,
    };
    size_t nbytes = (new.cap + 1) * sizeof(*new.rlines);
    new.rlines = xmalloc(nbytes);
    // all lines start empty
    memset(new.rlines, 0, nbytes);

    // the line_id of the current logical line from old_lines
    size_t old_line_id = 0;
    // the current rline we are writing to ("n"ew)
    RLine *n = NULL;
    size_t glyph_idx = 0;

    // {
    //     printf("PRECOPY:\n");
    //     for(size_t i = 0; i < old.len; i++){
    //         // get the next old rline
    //         size_t idx = (old.start + i) % (old.cap + 1);
    //         RLine *old = old.rlines[idx];
    //         char utf8[4096];
    //         size_t utf8_len = 0;
    //         for(size_t j = 0; j < old->n_glyphs; j++){
    //             utf8_len += utf8encode(old->glyphs[j].u, &utf8[utf8_len]);
    //         }
    //         printf("id=%.3lu: %.*s\n", old->line_id, (int)utf8_len, utf8);
    //     }
    //     printf("ENDPRECOPY\n");
    // }

    // printf("COPY:\n");
    // copy each old rline into a new rline
    for(size_t i = 0; i < old.len; i++){
        // get the next old rline ("o"ld)
        size_t idx = (old.start + i) % (old.cap + 1);
        RLine *o = old.rlines[idx];
        // ignore id=0 lines, which are the initial empty lines
        if(!o->line_id){
            goto cu_rline;
        }
        // does this old_line have a different line_id than what we last saw?
        if(!n || old_line_id != o->line_id){
            if(new.len == new.cap){
                // cursor reflow: decrement the stored y_abs values
                for(size_t i = 0; i < ncrs; i++){
                    cursor_reflow_decrement_y(crs[i]);
                }
            }
            n = scr_new_rline(&new, new_line_id(&new), col);
            // printf("\\n\n");
            glyph_idx = 0;
            old_line_id = o->line_id;
        }
        // copy all of the contents of this rline to the new rline
        for(size_t j = 0; j < o->n_glyphs; j++){
            // TODO: handle wide glpyhs
            Glyph g = o->glyphs[j];
            // ignore glyphs that need no copying
            if(g.mode & ATTR_NORENDER) continue;
            // do we need a new rline?
            if(glyph_idx >= col){
                if(new.len == new.cap){
                    // cursor reflow: decrement the stored y_abs values
                    for(size_t i = 0; i < ncrs; i++){
                        cursor_reflow_decrement_y(crs[i]);
                    }
                }
                // use the same line_id as the last one
                n = scr_new_rline(&new, n->line_id, col);
                // printf("\\n\n");
                glyph_idx = 0;
            }
            // actually copy a glyph into the new line
            n->glyphs[glyph_idx] = g;
            // printf("%c (%lu)\n", (char)g.u, new->line_id);
            // cursor reflow: cursor-over-copyable-glyph case
            for(size_t i = 0; i < ncrs; i++){
                cursor_reflow_copyable_glyph(
                    crs[i], o, j, glyph_idx, n, new.len - 1
                );
            }
            glyph_idx++;
        }
        /* cursor reflow: cursor-not-over-copyable-glyph case: just place the
           cursor at the end of the line to support 99.99% of cases */
        for(size_t i = 0; i < ncrs; i++){
            cursor_reflow_noncopyable_glyph(
                crs[i], o, glyph_idx, n, new.len - 1
            );
        }

    cu_rline:
        rline_free(&o);
    }
    // printf("ENDCOPY\n");

    // {
    //     printf("POSTCOPY:\n");
    //     for(size_t i = 0; i < new.len; i++){
    //         // get the next line
    //         size_t idx = (new.start + i) % (new.cap + 1);
    //         RLine *rline = new.rlines[idx];
    //         char utf8[4096];
    //         size_t utf8_len = 0;
    //         for(size_t j = 0; j < rline->n_glyphs; j++){
    //             printf("%c", (char)rline->glyphs[j].u);
    //             utf8_len += utf8encode(rline->glyphs[j].u, &utf8[utf8_len]);
    //         }
    //         printf("id=%.3lu:%zu: %.*s\n", rline->line_id, rline->n_glyphs, (int)utf8_len, utf8);
    //     }
    //     printf("ENDPOSTCOPY\n");
    // }

    free(old.rlines);

    // make sure we have at least enough rlines to fill the screen
    while(new.len < row){
        scr_new_rline(&new, 0, col);
    }

    return new;
}

void
tresize(Term *t, int col, int row)
{
    if (col < 1 || row < 1) {
        fprintf(stderr, "tresize: error resizing to %dx%d\n", col, row);
        return;
    }

    int *bp;
    int old_col = t->col;

    tunrender(t);

    /* cursors to reflow:
         - the current cursor (might be on main or alt screen)
         - the saved main cursor
         - the saved alt cursor
       Note that when we trim rlines to suit the main cursor, we never trim
       rlines based on either saved cursor.  We just discard the saved cursors
       if that case arises. */
    cursor_reflow_t cr_cur = cursor_reflow_new(t->c, get_cursor_rline(t));
    cursor_reflow_t cr_saved_main = cursor_reflow_new(
        t->c, get_rline(&t->main, term2abs(t, t->saved[0].y))
    );
    cursor_reflow_t cr_saved_alt = cursor_reflow_new(
        t->c, get_rline(&t->alt, term2abs(t, t->saved[1].y))
    );

    // reflow main screen first
    {
        cursor_reflow_t *crs[] = {&cr_saved_main, NULL};
        size_t ncrs = 1;
        if(t->scr == &t->main){
            crs[ncrs++] = &cr_cur;
        }
        t->main = reflow(
            t->main,
            old_col,
            row,
            col,
            t->main.cap, // map cap is unchanged
            crs,
            ncrs
        );
    }

    // then reflow alt screen
    {
        cursor_reflow_t *crs[] = {&cr_saved_alt, NULL};
        size_t ncrs = 1;
        if(t->scr == &t->alt){
            crs[ncrs++] = &cr_cur;
        }
        t->alt = reflow(
            t->alt,
            old_col,
            row,
            col,
            row, // altscreen cap is always the number of rows
            crs,
            ncrs
        );
    }

    /* TODO: Deal with scroll at some point.
        - if scroll = 0, let it stay zero
        - otherwise try to keep the top line as the top line */
    // (until then, just make sure scroll is always valid)
    t->scroll = 0;

    /* current cursor only: Discard lines in the buffer that are so low that
       the cursor would have to move downwards.

       In practice, this should never discard useful information, because
       either the cursor is in an early row and the terminal is either mostly
       empty or some full-window application is responsible for repainting the
       screen after the resize anyway.

       So suppose we're on the 0th row of a 40-row terminal, and we resize to
       a 39-row terminal, that means we trim (40 - 0) - 39 = 1 lines.

       Note that there is no change to x or absolute y position as a result. */
    size_t n_extras = cursor_reflow_lines_to_trim(&cr_cur, t->scr->len, row);
    for(size_t i = 0; i < n_extras; i++){
        RLine *rline = get_rline(t->scr, t->scr->len-- - 1);
        rline_free(&rline);
    }

    /* saved cursors: discard a saved cursor which would require us to drop
       any saved lines */
    cursor_reflow_invalidate_if_lines_to_trim(&cr_saved_main, t->main.len, row);
    cursor_reflow_invalidate_if_lines_to_trim(&cr_saved_alt, t->alt.len, row);

    /* resize to new height */
    t->tabs = xrealloc(t->tabs, col * sizeof(*t->tabs));

    // fix tabs
    if (col > old_col) {
        bp = t->tabs + old_col;

        memset(bp, 0, sizeof(*t->tabs) * (col - old_col));
        while (--bp > t->tabs && !*bp)
            /* nothing */ ;
        for (bp += tabspaces; bp < t->tabs + col; bp += tabspaces)
            *bp = 1;
    }

    /* update terminal size */
    t->col = col;
    t->row = row;

    // set the reflowed cursor positions
    t->c = cursor_reflow_done(&cr_cur, t->scr, row);
    t->saved[0] = cursor_reflow_done(&cr_saved_main, &t->main, row);
    t->saved[1] = cursor_reflow_done(&cr_saved_alt, &t->alt, row);

    tscrollregion(t, 0, row-1);

    // send a signal to the application in the terminal
    t->hooks->ttyresize(t->hooks, row, col);
}

//////

// copy a WxH sub rectangle of the source image to x,y in the destination image
void copy_rectangle(cairo_t *cr, cairo_surface_t *src, double x, double y,
        double w, double h){
    cairo_save(cr);
    cairo_set_source_surface(cr, src, x, y);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
    cairo_restore(cr);
}


int format_eq(Glyph a, Glyph b){
    return a.mode == b.mode
        && a.fg.r == b.fg.r && a.fg.g == b.fg.g && a.fg.b == b.fg.b
        && a.bg.r == b.bg.r && a.bg.g == b.bg.g && a.bg.b == b.bg.b;
}


void rline_free(RLine **rline){
    if(!*rline) return;

    if((*rline)->srfc){
        cairo_surface_destroy((*rline)->srfc);
        (*rline)->srfc = NULL;
    }

    free(*rline);
    *rline = NULL;
}

RLine *rline_new(size_t n_glyphs, uint64_t line_id){
    RLine *rline = xmalloc(sizeof(*rline));
    Glyph *glyphs = xmalloc(sizeof(*glyphs) * n_glyphs);
    *rline = (RLine){.glyphs=glyphs, .n_glyphs=n_glyphs, .line_id=line_id};
    for(size_t i = 0; i < rline->n_glyphs; i++){
        // default rune is ' ' so that cursor-on-empty-space works
        rline->glyphs[i] = (Glyph){ .u = ' ', .mode = ATTR_NORENDER };
    }
    return rline;
}

// create a new rline in the ring buffer, discarding the oldest one as needed.
RLine *scr_new_rline(Screen *scr, uint64_t line_id, size_t cols){
    // is ring buffer full?
    if(scr->len == scr->cap){
        // free oldest rline
        rline_free(&scr->rlines[scr->start]);
        // forget the oldest history element (start of the ring buffer)
        scr->start = rlines_idx(scr, 1);
        scr->len--;
    }
    // extend the buffer
    RLine *out = rline_new(cols, line_id);
    scr->rlines[rlines_idx(scr, scr->len++)] = out;

    return out;

}

static fmt_overrides_t t_get_fmt_override(Term *t, int y_abs){
    int cursor = -1;
    if(term2abs(t, t->c.y) == y_abs){
        cursor = t->c.x;
    }
    return (fmt_overrides_t){
        .cursor = cursor,
        // we don't support selections yet
        .sel_first = -1,
        .sel_last = -1,
    };
}

static bool ovr_eq(fmt_overrides_t a, fmt_overrides_t b){
    return memcmp(&a, &b, sizeof(a)) == 0;
}

static Glyph calc_fmt(fmt_overrides_t ovr, Glyph g, size_t x){
    if(ovr.sel_first > -1 && ovr.sel_last > -1){
        // selection reverses its bg/fg
        if(x >= (size_t)ovr.sel_first && x <= (size_t)ovr.sel_last){
            struct rgb24 old_fg = g.fg;
            g.fg = g.bg;
            g.bg = old_fg;
        }
    }
    if(ovr.cursor > -1 && x == (size_t)ovr.cursor){
        // white fg, bright red bg
        g.fg = rgb24_from_index(7);
        g.bg = rgb24_from_index(9);
        // disable NORENDER
        g.mode &= ~ATTR_NORENDER;
    }
    return g;
}

// "render context"
typedef struct {
    double grid_w;
    double grid_h;
    double render_w;
    PangoFontDescription *desc;
} rctx_t;

// render
double rline_subrender(
    RLine *rline,
    rctx_t rctx,
    cairo_t *cr,
    PangoLayout *layout,
    double x,
    size_t start,
    size_t end,
    // fmt is provided separately, since it may be overridden
    Glyph fmt
){
    // skip NORENDER chunks
    if(fmt.mode & ATTR_NORENDER){
        return x + rctx.grid_w * (end - start);
    }
    // expand glyphs back into utf8 for pango
    // TODO: support arbitrary-length lines
    char utf8[4096];
    size_t utf8_len = 0;
    for(size_t i = start; i < end; i++){
        utf8_len += utf8encode(rline->glyphs[i].u, &utf8[utf8_len]);
    }

    pango_layout_set_text(layout, utf8, utf8_len);

    cairo_move_to(cr, x, 0);
    // draw the background with the background color from the first glyph
    struct rgb24 rgb = fmt.bg;
    cairo_set_source_rgb(cr, rgb.r / 255., rgb.g / 255., rgb.b / 255.);
    cairo_rectangle(cr, x, 0, rctx.grid_w * (end - start), rctx.grid_h);
    cairo_fill(cr);

    cairo_move_to(cr, x, 0);
    // write the text with the foreground color from the first glyph
    rgb = fmt.fg;
    cairo_set_source_rgb(cr, rgb.r / 255., rgb.g / 255., rgb.b / 255.);
    pango_cairo_show_layout(cr, layout);

    // now get the width of the text we printed
    PangoRectangle rect;
    pango_layout_get_extents(layout, NULL, &rect);
    double w = ((double)rect.width) / PANGO_SCALE;
    return x + w;
}


void rline_render(RLine *rline, rctx_t rctx, fmt_overrides_t ovr){
    // handle caching
    if(rline->srfc){
        if(ovr_eq(ovr, rline->last_ovr)){
            // cached surface still valid
            return;
        }
        // otherwise destroy it and rerender
        rline_unrender(rline);
    }
    rline->last_ovr = ovr;

    rline->srfc = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, rctx.render_w, rctx.grid_h);

    // create cairo context and layout
    cairo_t *cr = cairo_create(rline->srfc);
    PangoLayout *layout = pango_cairo_create_layout(cr);

    // set font
    pango_layout_set_font_description(layout, rctx.desc);

    double x = 0;

    // break up the text into multiple chunks of common font settings
    Glyph fmt = calc_fmt(ovr, rline->glyphs[0], 0);
    size_t start = 0;
    size_t i;
    for(i = 1; i < rline->n_glyphs; i++){
        Glyph next_fmt = calc_fmt(ovr, rline->glyphs[i], i);
        if(!format_eq(fmt, next_fmt)){
            // found a different format, i-1 was the end of the render box
            x = rline_subrender(rline, rctx, cr, layout, x, start, i, fmt);
            start = i;
            // i is the beginning of the next format
            fmt = next_fmt;
        }
    }
    // render the final chunk
    rline_subrender(rline, rctx, cr, layout, x, start, rline->n_glyphs, fmt);
    g_object_unref(layout);
    cairo_destroy(cr);
}

void rline_unrender(RLine *rline){
    cairo_surface_destroy(rline->srfc);
    rline->srfc = NULL;
}

void rline_draw(
    RLine *rline,
    rctx_t rctx,
    cairo_t *cr,
    size_t line_offset
){
    copy_rectangle(
        cr,
        rline->srfc,
        0,
        rctx.grid_h * line_offset,
        rctx.render_w,
        rctx.grid_h
    );
}

// insert a glyph before the index
void rline_insert_glyph(RLine *rline, size_t idx, Glyph g){
    // TODO: is it right to just drop the final character when we do this?
    // TODO: xterm doesn't seem to have insert mode, should we even support it?
    // move all the glyphs from idx forwards, dropping the last glyph
    size_t glyphs_to_move = (rline->n_glyphs - 1) - idx;
    size_t bytes_to_move = sizeof(Glyph) * glyphs_to_move;
    memmove(&rline->glyphs[idx+1], &rline->glyphs[idx], bytes_to_move);

    rline->glyphs[idx] = g;

    // mark this line as dirty
    rline_unrender(rline);
}

// set a glyph to be something else
void rline_set_glyph(RLine *rline, size_t idx, Glyph g){
    rline->glyphs[idx] = g;

    // mark this line as dirty
    rline_unrender(rline);
}

// delete any rendered artifacts but leave the text alone
void tunrender(Term *t){
    for(size_t i = 0; i < t->main.len; i++){
        rline_unrender(get_rline(&t->main, i));
    }
    for(size_t i = 0; i < t->alt.len; i++){
        rline_unrender(get_rline(&t->alt, i));
    }
}

void trender(
    Term *t,
    cairo_t *cr,
    double w,
    double h,
    double x1,
    double y1,
    double x2,
    double y2
){
    // TODO: only rerender the dirty parts
    (void)x1; (void)y1; (void)x2; (void)y2;
    if(w != t->render_w || h != t->render_h){
        // delete old rendering
        tunrender(t);
        t->render_w = w;
        t->render_h = h;
        // resize the teriminal?
        int col = t->render_w / t->grid_w;
        int row = t->render_h / t->grid_h;
        if(col != t->col || row != t->row){
            // printf("resize due to render(%f, %f)\n", w, h);
            tresize(t, col, row);
        }
    }

    // make a render context
    rctx_t rctx = {
        .grid_w = t->grid_w,
        .grid_h = t->grid_h,
        .render_w = t->render_w,
        .desc = t->desc,
    };

    for(size_t i = 0; i < t->row; i++){
        size_t y_abs = view2abs(t, i);
        RLine *rline = get_rline(t->scr, y_abs);
        // capture any format overrides
        fmt_overrides_t ovr = t_get_fmt_override(t, y_abs);
        // render this line
        rline_render(rline, rctx, ovr);
        // draw this line onto the cairo surface
        rline_draw(rline, rctx, cr, i);
    }
}

// get the 24-bit color value from an ansi color index
// such as with the CSI 38 ; 5 ; X m notation
struct rgb24 rgb24_from_index(unsigned int index){
    // 16 basic colors
    struct rgb24 basic_colors[] = {
        {0, 0, 0},        // 0
        {205, 0, 0},      // 1
        {0, 205, 0},      // 2
        {205, 205, 0},    // 3
        {0, 0, 238},      // 4
        {205, 0, 205},    // 5
        {0, 205, 205},    // 6
        {229, 229, 229},  // 7
        {127, 127, 127},  // 8
        {255, 0, 0},      // 9
        {0, 255, 0},      // 10
        {255, 255, 0},    // 11
        {92, 92, 255},    // 12
        {255, 0, 255},    // 13
        {0, 255, 255},    // 14
        {255, 255, 255},  // 15
    };
    if(index < sizeof(basic_colors) / sizeof(*basic_colors)){
        return basic_colors[index];
    }

    // 6x6x6 cube
    if(index < 232){
        unsigned int x = index - 16;
        unsigned int b = x % 6;
        unsigned int g = ((x - b) / 6) % 6;
        unsigned int r = (x - b - 6 * g) / 6;
        return (struct rgb24){r * 51, g * 51, b * 51};
    }

    // grayscale in 24 steps (ends at 253, but whatever)
    if(index < 256){
        unsigned int x = index - 232;
        return (struct rgb24){x * 11, x * 11, x * 11};
    }

    fprintf(stderr, "warning: color index exceeds 255: %d\n", index);
    return (struct rgb24){255, 255, 255};
}
