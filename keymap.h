typedef struct {
    char *text;
    size_t len;
} simple_key_t;

typedef enum {
    KEY_ACTION_SIMPLE,
    KEY_ACTION_FUNC,
    KEY_ACTION_APPCURSOR,
    KEY_ACTION_APPKEY,
    KEY_ACTION_MODS,
} key_action_type_e;

struct key_action_t;
typedef struct key_action_t key_action_t;

typedef union {
    simple_key_t simple;
    void (*func)(void *globals, GdkEventKey *event_key);
    key_action_t *appcursor[2];
    key_action_t *appkey[2];
    key_action_t *mods[2];
} key_action_u;

struct key_action_t {
    key_action_type_e type;
    key_action_u val;
};

typedef struct {
    unsigned int mask;
    key_action_t *action;
} key_map_t;

// "key"
#define K(str) &(key_action_t){ \
    .type=KEY_ACTION_SIMPLE, \
    .val={ \
        .simple={ \
            .text=str, \
            .len=sizeof(str)-1 \
        } \
    } \
}

// "function"
#define F(fn) &(key_action_t){ \
    .type=KEY_ACTION_FUNC, \
    .func=fn \
}

// appcursor
#define CURS(on, off) &(key_action_t){ \
    .type=KEY_ACTION_APPCURSOR, \
    .val={ \
        .appcursor={K(on), K(off)} \
    } \
}

// appkeypad
#define KPAD(on, off) &(key_action_t){ \
    .type=KEY_ACTION_APPKEY, \
    .val={ \
        .appkeypad={K(on), K(off)} \
    } \
}

// xterm's 1 + (8=meta, 4=ctrl, 2=shift, 1=alt) scheme
// (note this does not work with ALTIFY at all)
// (see man 5 user_caps)
#define MODS(on, off) &(key_action_t){ \
    .type=KEY_ACTION_MODS, \
    .val={ \
        .mods={K(on), off} \
    } \
}

// actions defined in render.c
void shift_pgup(void *globals, GdkEventKey *event_key);
void shift_pgdn(void *globals, GdkEventKey *event_key);
void shift_insert(void *globals, GdkEventKey *event_key);

// 1 = match CTRL
// 2 = which CTRL matches (on or off)
#define MATCH_CTRL 1
#define CTRL_MASK 2
#define NOCTRL 1
#define CTRL_ 3

// 4 = match SHIFT
// 8 = which SHIFT matches
#define MATCH_SHIFT 4
#define SHIFT_MASK 8
#define NOSHIFT 4
#define SHIFT 12

// 16 = match ALT
// 32 = which ALT matches
#define MATCH_ALT 16
#define ALT_MASK 32
#define NOALT 16
#define ALT 48

// 64 = match META
// 128 = which META matches
#define MATCH_META 64
#define META_MASK 128
#define NOMETA 64
#define META 192

// ALTIFY, not a mask but modifies how the list should behave
#define ALTIFY 256

const unsigned int NAST_KEY_HOME = 0x80;
const unsigned int NAST_KEY_END = 0x81;
const unsigned int NAST_KEY_INSERT = 0x82;
const unsigned int NAST_KEY_DELETE = 0x83;
const unsigned int NAST_KEY_PGUP = 0x84;
const unsigned int NAST_KEY_PGDN = 0x85;
const unsigned int NAST_KEY_BKSP = 0x86;
const unsigned int NAST_KEY_ENTER = 0x87;
const unsigned int NAST_KEY_TAB = 0x88;
const unsigned int NAST_KEY_ESC = 0x89;

const unsigned int NAST_KEY_UP = 0x8a;
const unsigned int NAST_KEY_DN = 0x8b;
const unsigned int NAST_KEY_RIGHT = 0x8c;
const unsigned int NAST_KEY_LEFT = 0x8d;

const unsigned int NAST_KEY_F1 = 0x8e;
const unsigned int NAST_KEY_F2 = 0x8f;
const unsigned int NAST_KEY_F3 = 0x90;
const unsigned int NAST_KEY_F4 = 0x91;
const unsigned int NAST_KEY_F5 = 0x92;
const unsigned int NAST_KEY_F6 = 0x93;
const unsigned int NAST_KEY_F7 = 0x94;
const unsigned int NAST_KEY_F8 = 0x95;
const unsigned int NAST_KEY_F9 = 0x96;
const unsigned int NAST_KEY_F10 = 0x97;
const unsigned int NAST_KEY_F11 = 0x98;
const unsigned int NAST_KEY_F12 = 0x99;
const unsigned int NAST_KEY_F13 = 0x9a;
const unsigned int NAST_KEY_F14 = 0x9b;
const unsigned int NAST_KEY_F15 = 0x9c;
const unsigned int NAST_KEY_F16 = 0x9d;
const unsigned int NAST_KEY_F17 = 0x9e;
const unsigned int NAST_KEY_F18 = 0x9f;
const unsigned int NAST_KEY_F19 = 0xa0;
const unsigned int NAST_KEY_F20 = 0xa1;
const unsigned int NAST_KEY_F21 = 0xa2;
const unsigned int NAST_KEY_F22 = 0xa3;
const unsigned int NAST_KEY_F23 = 0xa4;
const unsigned int NAST_KEY_F24 = 0xa5;
const unsigned int NAST_KEY_F25 = 0xa6;
const unsigned int NAST_KEY_F26 = 0xa7;
const unsigned int NAST_KEY_F27 = 0xa8;
const unsigned int NAST_KEY_F28 = 0xa9;
const unsigned int NAST_KEY_F29 = 0xaa;
const unsigned int NAST_KEY_F30 = 0xab;
const unsigned int NAST_KEY_F31 = 0xac;
const unsigned int NAST_KEY_F32 = 0xad;
const unsigned int NAST_KEY_F33 = 0xae;
const unsigned int NAST_KEY_F34 = 0xaf;
const unsigned int NAST_KEY_F35 = 0xb0;
const unsigned int NAST_KEY_F36 = 0xb1;
const unsigned int NAST_KEY_F37 = 0xb2;
const unsigned int NAST_KEY_F38 = 0xb3;
const unsigned int NAST_KEY_F39 = 0xb4;
const unsigned int NAST_KEY_F40 = 0xb5;
const unsigned int NAST_KEY_F41 = 0xb6;
const unsigned int NAST_KEY_F42 = 0xb7;
const unsigned int NAST_KEY_F43 = 0xb8;
const unsigned int NAST_KEY_F44 = 0xb9;
const unsigned int NAST_KEY_F45 = 0xba;
const unsigned int NAST_KEY_F46 = 0xbb;
const unsigned int NAST_KEY_F47 = 0xbc;
const unsigned int NAST_KEY_F48 = 0xbd;
const unsigned int NAST_KEY_F49 = 0xbe;
const unsigned int NAST_KEY_F50 = 0xbf;
const unsigned int NAST_KEY_F51 = 0xc0;
const unsigned int NAST_KEY_F52 = 0xc1;
const unsigned int NAST_KEY_F53 = 0xc2;
const unsigned int NAST_KEY_F54 = 0xc3;
const unsigned int NAST_KEY_F55 = 0xc4;
const unsigned int NAST_KEY_F56 = 0xc5;
const unsigned int NAST_KEY_F57 = 0xc6;
const unsigned int NAST_KEY_F58 = 0xc7;
const unsigned int NAST_KEY_F59 = 0xc8;
const unsigned int NAST_KEY_F60 = 0xc9;
const unsigned int NAST_KEY_F61 = 0xca;
const unsigned int NAST_KEY_F62 = 0xcb;
const unsigned int NAST_KEY_F63 = 0xcc;

// The goal is to imitate masquerade as xterm.  Everything seems to work with
// xterm.  That's funny, because it used to be that xterm worked with
// everything, but it did that so well that now it's the other way around.
//
// Also see
//   man 5 terminfo
//   man 5 user_caps
//   infocmp xterm
//   https://spin0r.wordpress.com/2012/12/24/terminally-confused-part-five
//
// What about alt keys?  Why does e.g. alt-v emit "ö" ("\xc3\xb6")?  This
// is just utf8-encoding of 'v' with the 8th bit set.  `man xterm` has a
// section about eightBitInput that explains the set meta mode (smm) and
// reset meta mode (rmm) capabilities.  It seems st and urxvt both have the
// equivalent of the metaSendsEscape feature turned on permanently.
// Note that metaSendsEscape does _not_ alter the behavior of the special
// keys, like kHOM.
//
// Also: does gdk's im_context intercept keys typed with alt held down?
// Experimentally, the answer is no.

key_map_t *keymap[] = {
    // TODO: 0x00 through 0x1f I am unable to test
    (key_map_t[]){{0, K("\x00")}}, // 0x00
    (key_map_t[]){{0, K("\x01")}}, // 0x01
    (key_map_t[]){{0, K("\x02")}}, // 0x02
    (key_map_t[]){{0, K("\x03")}}, // 0x03
    (key_map_t[]){{0, K("\x04")}}, // 0x04
    (key_map_t[]){{0, K("\x05")}}, // 0x05
    (key_map_t[]){{0, K("\x06")}}, // 0x06
    (key_map_t[]){{0, K("\x07")}}, // 0x07
    (key_map_t[]){{0, K("\x08")}}, // 0x08
    (key_map_t[]){{0, K("\x09")}}, // 0x09
    (key_map_t[]){{0, K("\x0a")}}, // 0x0a
    (key_map_t[]){{0, K("\x0b")}}, // 0x0b
    (key_map_t[]){{0, K("\x0c")}}, // 0x0c
    (key_map_t[]){{0, K("\x0d")}}, // 0x0d
    (key_map_t[]){{0, K("\x0e")}}, // 0x0e
    (key_map_t[]){{0, K("\x0f")}}, // 0x0f

    (key_map_t[]){{0, K("\x10")}}, // 0x10
    (key_map_t[]){{0, K("\x11")}}, // 0x11
    (key_map_t[]){{0, K("\x12")}}, // 0x12
    (key_map_t[]){{0, K("\x13")}}, // 0x13
    (key_map_t[]){{0, K("\x14")}}, // 0x14
    (key_map_t[]){{0, K("\x15")}}, // 0x15
    (key_map_t[]){{0, K("\x16")}}, // 0x16
    (key_map_t[]){{0, K("\x17")}}, // 0x17
    (key_map_t[]){{0, K("\x18")}}, // 0x18
    (key_map_t[]){{0, K("\x19")}}, // 0x19
    (key_map_t[]){{0, K("\x1a")}}, // 0x1a
    (key_map_t[]){{0, K("\x1b")}}, // 0x1b
    (key_map_t[]){{0, K("\x1c")}}, // 0x1c
    (key_map_t[]){{0, K("\x1d")}}, // 0x1d
    (key_map_t[]){{0, K("\x1e")}}, // 0x1e
    (key_map_t[]){{0, K("\x1f")}}, // 0x1f

    (key_map_t[]){{ALTIFY | CTRL_, K("\0")}, {0, K(" ")}}, // 0x20 space
    (key_map_t[]){{ALTIFY, K("!")}}, // 0x21 !
    (key_map_t[]){{ALTIFY, K("\"")}}, // 0x22 "
    (key_map_t[]){{ALTIFY, K("#")}}, // 0x23 #
    (key_map_t[]){{ALTIFY, K("$")}}, // 0x24 $
    (key_map_t[]){{ALTIFY, K("%")}}, // 0x25 %
    (key_map_t[]){{ALTIFY, K("&")}}, // 0x26 &
    (key_map_t[]){{ALTIFY, K("'")}}, // 0x27 '
    (key_map_t[]){{ALTIFY, K("(")}}, // 0x28 (
    (key_map_t[]){{ALTIFY, K(")")}}, // 0x29 )
    (key_map_t[]){{ALTIFY, K("*")}}, // 0x2a *
    (key_map_t[]){{ALTIFY, K("+")}}, // 0x2b +
    (key_map_t[]){{ALTIFY, K(",")}}, // 0x2c ,
    (key_map_t[]){{ALTIFY, K("-")}}, // 0x2d -
    (key_map_t[]){{ALTIFY, K(".")}}, // 0x2e .
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1f")}, {0, K("/")}}, // 0x2f /

    (key_map_t[]){{ALTIFY, K("0")}}, // 0x30 0
    (key_map_t[]){{ALTIFY, K("1")}}, // 0x31 1
    (key_map_t[]){{ALTIFY | CTRL_, K("\x00")}, {0, K("2")}}, // 0x32 2
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1b")}, {0, K("3")}}, // 0x33 3
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1c")}, {0, K("4")}}, // 0x34 4
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1d")}, {0, K("5")}}, // 0x35 5
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1e")}, {0, K("6")}}, // 0x36 6
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1f")}, {0, K("7")}}, // 0x37 7
    (key_map_t[]){{ALTIFY | CTRL_, K("\x7f")}, {0, K("8")}}, // 0x38 8
    (key_map_t[]){{ALTIFY, K("9")}}, // 0x39 9
    (key_map_t[]){{ALTIFY, K(":")}}, // 0x3a :
    (key_map_t[]){{ALTIFY, K(";")}}, // 0x3b ;
    (key_map_t[]){{ALTIFY, K("<")}}, // 0x3c <
    (key_map_t[]){{ALTIFY, K("=")}}, // 0x3d =
    (key_map_t[]){{ALTIFY, K(">")}}, // 0x3e >
    (key_map_t[]){{ALTIFY | CTRL_, K("\x7f")}, {0, K("?")}}, // 0x3f ?

    (key_map_t[]){{ALTIFY | CTRL_, K("\x00")}, {0, K("@")}}, // 0x40 @
    (key_map_t[]){{ALTIFY | CTRL_, K("\x01")}, {0, K("A")}}, // 0x41 A
    (key_map_t[]){{ALTIFY | CTRL_, K("\x02")}, {0, K("B")}}, // 0x42 B
    (key_map_t[]){{ALTIFY | CTRL_, K("\x03")}, {0, K("C")}}, // 0x43 C
    (key_map_t[]){{ALTIFY | CTRL_, K("\x04")}, {0, K("D")}}, // 0x44 D
    (key_map_t[]){{ALTIFY | CTRL_, K("\x05")}, {0, K("E")}}, // 0x45 E
    (key_map_t[]){{ALTIFY | CTRL_, K("\x06")}, {0, K("F")}}, // 0x46 F
    (key_map_t[]){{ALTIFY | CTRL_, K("\x07")}, {0, K("G")}}, // 0x47 G
    (key_map_t[]){{ALTIFY | CTRL_, K("\x08")}, {0, K("H")}}, // 0x48 H
    (key_map_t[]){{ALTIFY | CTRL_, K("\x09")}, {0, K("I")}}, // 0x49 I
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0a")}, {0, K("J")}}, // 0x4a J
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0b")}, {0, K("K")}}, // 0x4b K
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0c")}, {0, K("L")}}, // 0x4c L
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0d")}, {0, K("M")}}, // 0x4d M
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0e")}, {0, K("N")}}, // 0x4e N
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0f")}, {0, K("O")}}, // 0x4f O

    (key_map_t[]){{ALTIFY | CTRL_, K("\x10")}, {0, K("P")}}, // 0x50 P
    (key_map_t[]){{ALTIFY | CTRL_, K("\x11")}, {0, K("Q")}}, // 0x51 Q
    (key_map_t[]){{ALTIFY | CTRL_, K("\x12")}, {0, K("R")}}, // 0x52 R
    (key_map_t[]){{ALTIFY | CTRL_, K("\x13")}, {0, K("S")}}, // 0x53 S
    (key_map_t[]){{ALTIFY | CTRL_, K("\x14")}, {0, K("T")}}, // 0x54 T
    (key_map_t[]){{ALTIFY | CTRL_, K("\x15")}, {0, K("U")}}, // 0x55 U
    (key_map_t[]){{ALTIFY | CTRL_, K("\x16")}, {0, K("V")}}, // 0x56 V
    (key_map_t[]){{ALTIFY | CTRL_, K("\x17")}, {0, K("W")}}, // 0x57 W
    (key_map_t[]){{ALTIFY | CTRL_, K("\x18")}, {0, K("X")}}, // 0x58 X
    (key_map_t[]){{ALTIFY | CTRL_, K("\x19")}, {0, K("Y")}}, // 0x59 Y
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1a")}, {0, K("Z")}}, // 0x5a Z
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1b")}, {0, K("[")}}, // 0x5b [
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1c")}, {0, K("\\")}}, /* 0x5c \ */
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1d")}, {0, K("]")}}, // 0x5d ]
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1e")}, {0, K("^")}}, // 0x5e ^
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1f")}, {0, K("_")}}, // 0x5f _

    (key_map_t[]){{ALTIFY | CTRL_, K("\x00")}, {0, K("`")}}, // 0x60 `
    (key_map_t[]){{ALTIFY | CTRL_, K("\x01")}, {0, K("a")}}, // 0x61 a
    (key_map_t[]){{ALTIFY | CTRL_, K("\x02")}, {0, K("b")}}, // 0x62 b
    (key_map_t[]){{ALTIFY | CTRL_, K("\x03")}, {0, K("c")}}, // 0x63 c
    (key_map_t[]){{ALTIFY | CTRL_, K("\x04")}, {0, K("d")}}, // 0x64 d
    (key_map_t[]){{ALTIFY | CTRL_, K("\x05")}, {0, K("e")}}, // 0x65 e
    (key_map_t[]){{ALTIFY | CTRL_, K("\x06")}, {0, K("f")}}, // 0x66 f
    (key_map_t[]){{ALTIFY | CTRL_, K("\x07")}, {0, K("g")}}, // 0x67 g
    (key_map_t[]){{ALTIFY | CTRL_, K("\x08")}, {0, K("h")}}, // 0x68 h
    (key_map_t[]){{ALTIFY | CTRL_, K("\x09")}, {0, K("i")}}, // 0x69 i
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0a")}, {0, K("j")}}, // 0x6a j
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0b")}, {0, K("k")}}, // 0x6b k
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0c")}, {0, K("l")}}, // 0x6c l
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0d")}, {0, K("m")}}, // 0x6d m
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0e")}, {0, K("n")}}, // 0x6e n
    (key_map_t[]){{ALTIFY | CTRL_, K("\x0f")}, {0, K("o")}}, // 0x6f o

    (key_map_t[]){{ALTIFY | CTRL_, K("\x10")}, {0, K("P")}}, // 0x70 p
    (key_map_t[]){{ALTIFY | CTRL_, K("\x11")}, {0, K("Q")}}, // 0x71 q
    (key_map_t[]){{ALTIFY | CTRL_, K("\x12")}, {0, K("R")}}, // 0x72 r
    (key_map_t[]){{ALTIFY | CTRL_, K("\x13")}, {0, K("S")}}, // 0x73 s
    (key_map_t[]){{ALTIFY | CTRL_, K("\x14")}, {0, K("T")}}, // 0x74 t
    (key_map_t[]){{ALTIFY | CTRL_, K("\x15")}, {0, K("U")}}, // 0x75 u
    (key_map_t[]){{ALTIFY | CTRL_, K("\x16")}, {0, K("V")}}, // 0x76 v
    (key_map_t[]){{ALTIFY | CTRL_, K("\x17")}, {0, K("W")}}, // 0x77 w
    (key_map_t[]){{ALTIFY | CTRL_, K("\x18")}, {0, K("X")}}, // 0x78 x
    (key_map_t[]){{ALTIFY | CTRL_, K("\x19")}, {0, K("Y")}}, // 0x79 y
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1a")}, {0, K("Z")}}, // 0x7a z
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1b")}, {0, K("{")}}, // 0x7b {
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1c")}, {0, K("|")}}, // 0x7c |
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1d")}, {0, K("}")}}, // 0x7d }
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1e")}, {0, K("~")}}, // 0x7e ~
    (key_map_t[]){{ALTIFY | CTRL_, K("\x1f")}, {0, K("\x7f")}}, // 0x7f del  // TODO: could not produce this

    // Non-ascii inputs:

    (key_map_t[]){{0, MODS("\x1b[1;%dH", CURS("\x1bOH", "\x1b[H"))}}, // NAST_KEY_HOME
    (key_map_t[]){{0, MODS("\x1b[1;%dF", CURS("\x1bOF", "\x1b[F"))}}, // NAST_KEY_END
    (key_map_t[]){{0, MODS("\x1b[2;%d~", K("\x1b[2~"))}}, // NAST_KEY_INSERT
    (key_map_t[]){{0, MODS("\x1b[3;%d~", K("\x1b[3~"))}}, // NAST_KEY_DELETE

    (key_map_t[]){{0, MODS("\x1b[5;%d~", K("\x1b[5~"))}}, // NAST_KEY_PGUP
    (key_map_t[]){{0, MODS("\x1b[6;%d~", K("\x1b[6~"))}}, // NAST_KEY_PGDN

    (key_map_t[]){{ALTIFY | CTRL_, K("\x7f")}, {0, K("\b")}}, // NAST_KEY_BKSP
    (key_map_t[]){{0, K("\r")}}, // NAST_KEY_ENTER
    (key_map_t[]){{SHIFT, K("\x1b[Z")}, {ALT, K("\xc2\x89")}, {0, K("\t")}}, // NAST_KEY_TAB
    (key_map_t[]){{0, K("\x1b")}}, // NAST_KEY_ESC

    (key_map_t[]){{0, MODS("\x1b[1;%dA", CURS("\x1bOA", "\x1b[A"))}}, // NAST_KEY_UP
    (key_map_t[]){{0, MODS("\x1b[1;%dB", CURS("\x1bOB", "\x1b[B"))}}, // NAST_KEY_DN
    (key_map_t[]){{0, MODS("\x1b[1;%dC", CURS("\x1bOC", "\x1b[C"))}}, // NAST_KEY_RIGHT
    (key_map_t[]){{0, MODS("\x1b[1;%dD", CURS("\x1bOD", "\x1b[D"))}}, // NAST_KEY_LEFT

    (key_map_t[]){{0, MODS("\x1b[1;%dP", K("\x1bOP"))}}, // NAST_KEY_F1
    (key_map_t[]){{0, MODS("\x1b[1;%dQ", K("\x1bOQ"))}}, // NAST_KEY_F2
    (key_map_t[]){{0, MODS("\x1b[1;%dR", K("\x1bOR"))}}, // NAST_KEY_F3
    (key_map_t[]){{0, MODS("\x1b[1;%dS", K("\x1bOS"))}}, // NAST_KEY_F4

    (key_map_t[]){{0, MODS("\x1b[15;%d~", K("\x1b[15~"))}}, // NAST_KEY_F5

    (key_map_t[]){{0, MODS("\x1b[17;%d~", K("\x1b[17~"))}}, // NAST_KEY_F6
    (key_map_t[]){{0, MODS("\x1b[18;%d~", K("\x1b[18~"))}}, // NAST_KEY_F7
    (key_map_t[]){{0, MODS("\x1b[19;%d~", K("\x1b[19~"))}}, // NAST_KEY_F8
    (key_map_t[]){{0, MODS("\x1b[20;%d~", K("\x1b[20~"))}}, // NAST_KEY_F9
    (key_map_t[]){{0, MODS("\x1b[21;%d~", K("\x1b[21~"))}}, // NAST_KEY_F10

    (key_map_t[]){{0, MODS("\x1b[23;%d~", K("\x1b[23~"))}}, // NAST_KEY_F11
    (key_map_t[]){{0, MODS("\x1b[24;%d~", K("\x1b[24~"))}}, // NAST_KEY_F12

    // TODO: F12-F63 are from xterm's infocmp, can't seem to test them
    // note that e.g. F13 conflicts with a modifer version of F1
    (key_map_t[]){{0, K("\x1b[1;2P")}}, // NAST_KEY_F13
    (key_map_t[]){{0, K("\x1b[1;2Q")}}, // NAST_KEY_F14
    (key_map_t[]){{0, K("\x1b[1;2R")}}, // NAST_KEY_F15
    (key_map_t[]){{0, K("\x1b[1;2S")}}, // NAST_KEY_F16
    (key_map_t[]){{0, K("\x1b[15;2~")}}, // NAST_KEY_F17
    (key_map_t[]){{0, K("\x1b[17;2~")}}, // NAST_KEY_F18
    (key_map_t[]){{0, K("\x1b[18;2~")}}, // NAST_KEY_F19
    (key_map_t[]){{0, K("\x1b[19;2~")}}, // NAST_KEY_F20
    (key_map_t[]){{0, K("\x1b[20;2~")}}, // NAST_KEY_F21
    (key_map_t[]){{0, K("\x1b[21;2~")}}, // NAST_KEY_F22
    (key_map_t[]){{0, K("\x1b[23;2~")}}, // NAST_KEY_F23
    (key_map_t[]){{0, K("\x1b[24;2~")}}, // NAST_KEY_F24
    (key_map_t[]){{0, K("\x1b[1;5P")}}, // NAST_KEY_F25
    (key_map_t[]){{0, K("\x1b[1;5Q")}}, // NAST_KEY_F26
    (key_map_t[]){{0, K("\x1b[1;5R")}}, // NAST_KEY_F27
    (key_map_t[]){{0, K("\x1b[1;5S")}}, // NAST_KEY_F28
    (key_map_t[]){{0, K("\x1b[15;5~")}}, // NAST_KEY_F29
    (key_map_t[]){{0, K("\x1b[17;5~")}}, // NAST_KEY_F30
    (key_map_t[]){{0, K("\x1b[18;5~")}}, // NAST_KEY_F31
    (key_map_t[]){{0, K("\x1b[19;5~")}}, // NAST_KEY_F32
    (key_map_t[]){{0, K("\x1b[20;5~")}}, // NAST_KEY_F33
    (key_map_t[]){{0, K("\x1b[21;5~")}}, // NAST_KEY_F34
    (key_map_t[]){{0, K("\x1b[23;5~")}}, // NAST_KEY_F35
    (key_map_t[]){{0, K("\x1b[24;5~")}}, // NAST_KEY_F36
    (key_map_t[]){{0, K("\x1b[1;6P")}}, // NAST_KEY_F37
    (key_map_t[]){{0, K("\x1b[1;6Q")}}, // NAST_KEY_F38
    (key_map_t[]){{0, K("\x1b[1;6R")}}, // NAST_KEY_F39
    (key_map_t[]){{0, K("\x1b[1;6S")}}, // NAST_KEY_F40
    (key_map_t[]){{0, K("\x1b[15;6~")}}, // NAST_KEY_F41
    (key_map_t[]){{0, K("\x1b[17;6~")}}, // NAST_KEY_F42
    (key_map_t[]){{0, K("\x1b[18;6~")}}, // NAST_KEY_F43
    (key_map_t[]){{0, K("\x1b[19;6~")}}, // NAST_KEY_F44
    (key_map_t[]){{0, K("\x1b[20;6~")}}, // NAST_KEY_F45
    (key_map_t[]){{0, K("\x1b[21;6~")}}, // NAST_KEY_F46
    (key_map_t[]){{0, K("\x1b[23;6~")}}, // NAST_KEY_F47
    (key_map_t[]){{0, K("\x1b[24;6~")}}, // NAST_KEY_F48
    (key_map_t[]){{0, K("\x1b[1;3P")}}, // NAST_KEY_F49
    (key_map_t[]){{0, K("\x1b[1;3Q")}}, // NAST_KEY_F50
    (key_map_t[]){{0, K("\x1b[1;3R")}}, // NAST_KEY_F51
    (key_map_t[]){{0, K("\x1b[1;3S")}}, // NAST_KEY_F52
    (key_map_t[]){{0, K("\x1b[15;3~")}}, // NAST_KEY_F53
    (key_map_t[]){{0, K("\x1b[17;3~")}}, // NAST_KEY_F54
    (key_map_t[]){{0, K("\x1b[18;3~")}}, // NAST_KEY_F55
    (key_map_t[]){{0, K("\x1b[19;3~")}}, // NAST_KEY_F56
    (key_map_t[]){{0, K("\x1b[20;3~")}}, // NAST_KEY_F57
    (key_map_t[]){{0, K("\x1b[21;3~")}}, // NAST_KEY_F58
    (key_map_t[]){{0, K("\x1b[23;3~")}}, // NAST_KEY_F59
    (key_map_t[]){{0, K("\x1b[24;3~")}}, // NAST_KEY_F60
    (key_map_t[]){{0, K("\x1b[1;4P")}}, // NAST_KEY_F61
    (key_map_t[]){{0, K("\x1b[1;4Q")}}, // NAST_KEY_F62
    (key_map_t[]){{0, K("\x1b[1;4R")}}, // NAST_KEY_F63
};

#undef K
#undef F
#undef CURS
#undef KPAD
#undef MODS
