/* See LICENSE for license details. */

#include <stdint.h>
#include <sys/types.h>

#include <cairo.h>
#include <pango/pangocairo.h>

/* macros */
#ifndef MIN
#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)        ((a) < (b) ? (b) : (a))
#endif
#define LEN(a)            (sizeof(a) / sizeof(a)[0])
#define BETWEEN(x, a, b)    ((a) <= (x) && (x) <= (b))
#define DIVCEIL(n, d)        (((n) + ((d) - 1)) / (d))
#define DEFAULT(a, b)        (a) = (a) ? (a) : (b)
#define LIMIT(x, a, b)        (x) = (x) < (a) ? (a) : (x) > (b) ? (b) : (x)
#define ATTRCMP(a, b)        ((a).mode != (b).mode || (a).fg != (b).fg || \
                (a).bg != (b).bg)
#define TIMEDIFF(t1, t2)    ((t1.tv_sec-t2.tv_sec)*1000 + \
                (t1.tv_nsec-t2.tv_nsec)/1E6)
#define MODBIT(x, set, bit)    ((set) ? ((x) |= (bit)) : ((x) &= ~(bit)))

enum glyph_attribute {
    ATTR_NULL       = 0,
    ATTR_BOLD       = 1 << 0,
    ATTR_FAINT      = 1 << 1,
    ATTR_ITALIC     = 1 << 2,
    ATTR_UNDERLINE  = 1 << 3,
    ATTR_BLINK      = 1 << 4,
    ATTR_REVERSE    = 1 << 5,
    ATTR_INVISIBLE  = 1 << 6,
    ATTR_STRUCK     = 1 << 7,
    ATTR_WRAP       = 1 << 8,
    ATTR_WIDE       = 1 << 9,
    ATTR_WDUMMY     = 1 << 10,
    ATTR_BOLD_FAINT = ATTR_BOLD | ATTR_FAINT,
};

enum selection_mode {
    SEL_IDLE = 0,
    SEL_EMPTY = 1,
    SEL_READY = 2
};

enum selection_type {
    SEL_REGULAR = 1,
    SEL_RECTANGULAR = 2
};

enum selection_snap {
    SNAP_WORD = 1,
    SNAP_LINE = 2
};

struct rgb24 {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;

typedef uint_least32_t Rune;

#define Glyph Glyph_
typedef struct {
    Rune u;           /* character code */
    ushort mode;      /* attribute flags */
    struct rgb24 fg; /* foreground  */
    struct rgb24 bg; /* background  */
} Glyph;

typedef Glyph* Line;

// render line, one line of rendered text
typedef struct {
    cairo_surface_t *srfc;
    // which characters in the TLine belong in this rendered line
    const Glyph *glyphs;
    size_t n_glyphs;
} RLine;

// terminal line, might get rendered onto many lines
typedef struct {
    // one glyph per character
    Glyph *glyphs;
    size_t n_glyphs;
    size_t glyphs_cap;
    // rendering results (n_rlines == 0 after tline_unrender())
    RLine **rlines;
    size_t n_rlines;
    size_t rlines_cap;
} TLine;

typedef union {
    int i;
    uint ui;
    float f;
    const void *v;
    const char *s;
} Arg;

void die(const char *, ...);
void redraw(void);
void draw(void);

void printscreen(const Arg *);
void printsel(const Arg *);
void sendbreak(const Arg *);
void toggleprinter(const Arg *);

int tattrset(int);
void tnew(int col, int row, char *font_name, void (*ttywrite)(const char *, size_t));
void tresize(int, int);
void tsetdirtattr(int);
void ttyhangup(void);
int ttynew(char *, char *, char *, char **);
size_t ttyread(void);
void ttyresize(int, int);
void die_on_ttywrite(const char *buf, size_t len);

void resettitle(void);

void selclear(void);
void selinit(void);
void selstart(int, int, int);
void selextend(int, int, int, int);
int selected(int, int);
char *getsel(void);

size_t utf8encode(Rune, char *);

void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(char *);

/* config.h globals */
extern char *utmp;
extern char *stty_args;
extern char *vtiden;
extern wchar_t *worddelimiters;
extern int allowaltscreen;
extern char *termname;
extern unsigned int tabspaces;

extern struct rgb24 defaultfg;
extern struct rgb24 defaultbg;

struct rgb24 rgb24_from_index(unsigned int index);

//////

int twrite(const char *, int, int);
TLine *tline_new(void);
void tline_free(TLine **tline);
// insert a glpyh before the index
void tline_insert_glyph(TLine *tline, size_t idx, Glyph g);
// set a glyph to be something else
void tline_set_glyph(TLine *tline, size_t idx, Glyph g);

void trender(cairo_t *cr, double w, double h);