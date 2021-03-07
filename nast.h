/* See LICENSE for license details. */

#include <stdint.h>
#include <sys/types.h>
#include <stdbool.h>

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
    ATTR_NORENDER   = 1 << 11,
};

enum win_mode {
    MODE_VISIBLE     = 1 << 0,
    MODE_FOCUSED     = 1 << 1,
    MODE_APPKEYPAD   = 1 << 2,
    MODE_MOUSEBTN    = 1 << 3,
    MODE_MOUSEMOTION = 1 << 4,
    MODE_REVERSE     = 1 << 5,
    MODE_KBDLOCK     = 1 << 6,
    MODE_HIDE        = 1 << 7,
    MODE_APPCURSOR   = 1 << 8,
    MODE_MOUSESGR    = 1 << 9,
    MODE_8BIT        = 1 << 10,
    MODE_BLINK       = 1 << 11,
    MODE_FBLINK      = 1 << 12,
    MODE_FOCUS       = 1 << 13,
    MODE_MOUSEX10    = 1 << 14,
    MODE_MOUSEMANY   = 1 << 15,
    MODE_NUMLOCK     = 1 << 16,
    MODE_MOUSE       = MODE_MOUSEBTN
                     | MODE_MOUSEMOTION
                     | MODE_MOUSEX10
                     | MODE_MOUSEMANY,
};

enum cursor_style {
    CURSOR_BLOCK_BLINK = 1, // 0 or 1
    CURSOR_BLOCK_SOLID,
    CURSOR_UNDRLN_BLINK,
    CURSOR_UNDRLN_SOLID,
    CURSOR_BAR_BLINK,
    CURSOR_BAR_SOLID,
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
    Glyph *glyphs;
    // can this line be rewrapped with the prev/next lines?
    bool ends_line;
    /* in order to detect rewrappable lines, we must be able to detect behavior
       which is not acceptable for rewrapping.  Unacceptable means writing any
       character which was neither an overwrite of an existing character nor an
       append to the end of a block of characters starting at the beginning of
       a line */
    bool rewrappable;
    size_t n_glyphs;
    // consecutive physical lines of matching line_id form a logical line.
    uint64_t line_id;
} RLine;

struct THooks;
typedef struct THooks THooks;

struct THooks {
    // for writing to TTY for certain control codes
    void (*ttywrite)(THooks*, const char *, size_t);
    void (*bell)(THooks*);
    void (*set_mode)(THooks*, enum win_mode, int);
    void (*set_title)(THooks*, const char *);
    void (*set_clipboard)(THooks*, char *buf, size_t len);
};

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
void tnew(int col, int row, char *font_name, THooks *hooks);
void tresize(int, int);
void tsetdirtattr(int);
void ttyhangup(void);
int ttynew(char *, char *, char *, char **);
size_t ttyread(void);
void ttyresize(int, int);

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
RLine *rline_new(size_t n_glyphs, uint64_t line_id);
void rline_free(RLine **rline);
// insert a glpyh before the index
void rline_insert_glyph(RLine *rline, size_t idx, Glyph g);
// set a glyph to be something else
void rline_set_glyph(RLine *rline, size_t idx, Glyph g);

void trender(
    cairo_t *cr, double w, double h, double x1, double y1, double x2, double y2
);
void tunrender(void);
void rline_unrender(RLine *rline);

/*

Rendering details:

    ring buffer, with logical indices from y=0 to y=rlines_len()-1
    "window" is a section of lines of term.rows length that is to be rendered
    "scroll" is how many unrendered lines are after the window
    scroll is a render-only concept, and ANSI codes are unaffected

                  rlines
                   ___
              y=0 |___|
                  |___|
                  |___|
             ___  |___| __
            /     |___|   |   y=window_start()
           /      |___|   |
  render -+       |___|   |term.rows
  window   \      |___|   |
            \___  |___| __| __
                  |___|       |
                  |___|       |
                  |___|       |term.scroll
                  |___|       |
 y=rlines_len()-1 |___| ______|

*/
