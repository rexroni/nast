typedef struct key_action_t {
    char *key;
    char *altify;
    size_t len;
    void (*func)(void *globals, GdkEventKey *event_key);
    struct key_action_t *appcursor[2];
    struct key_action_t *appkey[2];
    struct key_action_t *ctrl[2];
    struct key_action_t *shift[2];
    struct key_action_t *alt[2];
    struct key_action_t *mods[2];
} key_action_t; // "key action"

const unsigned int NAST_KEY_HOME = 0x80;
const unsigned int NAST_KEY_END = 0x81;
const unsigned int NAST_KEY_INSERT = 0x82;
const unsigned int NAST_KEY_DELETE = 0x83;
const unsigned int NAST_KEY_PGUP = 0x84;
const unsigned int NAST_KEY_PGDN = 0x85;
const unsigned int NAST_KEY_BKSP = 0x86;
const unsigned int NAST_KEY_ENTER = 0x87;

// "key"
#define K(str) &(key_action_t){.key=str, len=sizeof(str)-1}

// "function"
#define A(fn) &(key_action_t){.action=fn}

// appcursor
#define CURS(on, off) &(key_action_t){.appcursor={(on), (off)}}

// appkeypad
#define KPAD(on, off) &(key_action_t){.appkey={(on), (off)}}

#define CTRL(on, off) &(key_action_t){.ctrl={(on), (off)}}
#define SHIFT(on, off) &(key_action_t){.shift={(on), (off)}}
#define ALT(on, off) &(key_action_t){.alt={(on), (off)}}

// xterm's 1 + (4=ctrl, 2=shift, 1=alt) scheme
#define MODS(on, off) &(key_action_t){.mods={(on), (off)}}

// actions defined in render.c
void shift_pgup(void *globals, GdkEventKey *event_key);
void shift_pgdn(void *globals, GdkEventKey *event_key);
void shift_insert(void *globals, GdkEventKey *event_key);

enum mod_combo_t {
    MOD_NONE = 0,
    MOD_CTRL = 1,
    MOD_SHIFT = 2,
    MOD_ALT = 4,
};

// 1 = match CTRL
// 2 = which CTRL matches
#define MATCH_CTRL 1
#define NOCTRL 1
#define CTRL 3

// 4 = match SHIFT
// 8 = which SHIFT matches
#define MATCH_SHIFT 4
#define NOSHIFT 4
#define SHIFT 12

// 16 = match ALT
// 32 = which ALT matches
#define MATCH_ALT 16
#define NOALT 16
#define ALT 48

// 64 = match META
// 128 = which META matches
#define MATCH_META 64
#define NOMETA 64
#define META 192

// any of shift, ctrl, or alt should match
#define ANYMOD 256

// ALTIFY, not a mask but modifies how the list should behave
#define ALTIFY 512

typedef struct {
    int in;
    int mods;
    key_action_t *out;
} match_t;

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
// equivalent of the metaSendsEscape feature turned on permanently.  I
// would be ok always emitting escape+keys for the non-special keys.
// Note that metaSendsEscape does _not_ alter the behavior of the special
// keys, like kHOM.
//
// Also: does gdk's im_context intercept keys typed with alt held down?
// Experimentally, the answer is no.

key_action_t *keymap[] = {
    // TODO: 0x00 through 0x1f I am unable to test
    {{ALTIFY, K("0x00")}}, // 0x00
    {{ALTIFY, K("0x01")}}, // 0x01
    {{ALTIFY, K("0x02")}}, // 0x02
    {{ALTIFY, K("0x03")}}, // 0x03
    {{ALTIFY, K("0x04")}}, // 0x04
    {{ALTIFY, K("0x05")}}, // 0x05
    {{ALTIFY, K("0x06")}}, // 0x06
    {{ALTIFY, K("0x07")}}, // 0x07
    {{ALTIFY, K("0x08")}}, // 0x08
    {{ALTIFY, K("0x09")}}, // 0x09
    {{ALTIFY, K("0x0a")}}, // 0x0a
    {{ALTIFY, K("0x0b")}}, // 0x0b
    {{ALTIFY, K("0x0c")}}, // 0x0c
    {{ALTIFY, K("0x0d")}}, // 0x0d
    {{ALTIFY, K("0x0e")}}, // 0x0e
    {{ALTIFY, K("0x0f")}}, // 0x0f

    {{ALTIFY, K("0x10")}}, // 0x10
    {{ALTIFY, K("0x11")}}, // 0x11
    {{ALTIFY, K("0x12")}}, // 0x12
    {{ALTIFY, K("0x13")}}, // 0x13
    {{ALTIFY, K("0x14")}}, // 0x14
    {{ALTIFY, K("0x15")}}, // 0x15
    {{ALTIFY, K("0x16")}}, // 0x16
    {{ALTIFY, K("0x17")}}, // 0x17
    {{ALTIFY, K("0x18")}}, // 0x18
    {{ALTIFY, K("0x19")}}, // 0x19
    {{ALTIFY, K("0x1a")}}, // 0x1a
    {{ALTIFY, K("0x1b")}}, // 0x1b
    {{ALTIFY, K("0x1c")}}, // 0x1c
    {{ALTIFY, K("0x1d")}}, // 0x1d
    {{ALTIFY, K("0x1e")}}, // 0x1e
    {{ALTIFY, K("0x1f")}}, // 0x1f

    {{ALTIFY | CTRL, K("\0"), {0, K(" ")}}, // 0x20 space
    {{ALTIFY, K("!")}}, // 0x21 !
    {{ALTIFY, K("\"")}}, // 0x22 "
    {{ALTIFY, K("#")}}, // 0x23 #
    {{ALTIFY, K("$")}}, // 0x24 $
    {{ALTIFY, K("%")}}, // 0x25 %
    {{ALTIFY, K("&")}}, // 0x26 &
    {{ALTIFY, K("'")}}, // 0x27 '
    {{ALTIFY, K("(")}}, // 0x28 (
    {{ALTIFY, K(")")}}, // 0x29 )
    {{ALTIFY, K("*")}}, // 0x2a *
    {{ALTIFY, K("+")}}, // 0x2b +
    {{ALTIFY, K(",")}}, // 0x2c ,
    {{ALTIFY, K("-")}}, // 0x2d -
    {{ALTIFY, K(".")}}, // 0x2e .
    {{ALTIFY | CTRL, K("\x1f"), {0, K("/")}}, // 0x2f /

    {{ALTIFY, K("0")}}, // 0x30 0
    {{ALTIFY, K("1")}},      // 0x31 1
    {{ALTIFY | CTRL, K("\x00"), {0, K("2")}}, // 0x32 2
    {{ALTIFY | CTRL, K("\x1b"), {0, K("3")}}, // 0x33 3
    {{ALTIFY | CTRL, K("\x1c"), {0, K("4")}}, // 0x34 4
    {{ALTIFY | CTRL, K("\x1d"), {0, K("5")}}, // 0x35 5
    {{ALTIFY | CTRL, K("\x1e"), {0, K("6")}}, // 0x36 6
    {{ALTIFY | CTRL, K("\x1f"), {0, K("7")}}, // 0x37 7
    {{ALTIFY | CTRL, K("\x7f"), {0, K("8")}}, // 0x38 8
    {{ALTIFY, K("9")}}, // 0x39 9
    {{ALTIFY, K(":")}}, // 0x3a :
    {{ALTIFY, K(";")}}, // 0x3b ;
    {{ALTIFY, K("<")}}, // 0x3c <
    {{ALTIFY, K("=")}}, // 0x3d =
    {{ALTIFY, K(">")}}, // 0x3e >
    {{ALTIFY | CTRL, K("\x7f"), {0, K("?")}}, // 0x3f ?

    {{ALTIFY | CTRL, K("\x00"), {0, K("@")}}, // 0x40 @
    {{ALTIFY | CTRL, K("\x01"), {0, K("A")}}, // 0x41 A
    {{ALTIFY | CTRL, K("\x02"), {0, K("B")}}, // 0x42 B
    {{ALTIFY | CTRL, K("\x03"), {0, K("C")}}, // 0x43 C
    {{ALTIFY | CTRL, K("\x04"), {0, K("D")}}, // 0x44 D
    {{ALTIFY | CTRL, K("\x05"), {0, K("E")}}, // 0x45 E
    {{ALTIFY | CTRL, K("\x06"), {0, K("F")}}, // 0x46 F
    {{ALTIFY | CTRL, K("\x07"), {0, K("G")}}, // 0x47 G
    {{ALTIFY | CTRL, K("\x08"), {0, K("H")}}, // 0x48 H
    {{ALTIFY | CTRL, K("\x09"), {0, K("I")}}, // 0x49 I
    {{ALTIFY | CTRL, K("\x0a"), {0, K("J")}}, // 0x4a J
    {{ALTIFY | CTRL, K("\x0b"), {0, K("K")}}, // 0x4b K
    {{ALTIFY | CTRL, K("\x0c"), {0, K("L")}}, // 0x4c L
    {{ALTIFY | CTRL, K("\x0d"), {0, K("M")}}, // 0x4d M
    {{ALTIFY | CTRL, K("\x0e"), {0, K("N")}}, // 0x4e N
    {{ALTIFY | CTRL, K("\x0f"), {0, K("O")}}, // 0x4f O

    {{ALTIFY | CTRL, K("\x10"), {0, K("P")}}, // 0x50 P
    {{ALTIFY | CTRL, K("\x11"), {0, K("Q")}}, // 0x51 Q
    {{ALTIFY | CTRL, K("\x12"), {0, K("R")}}, // 0x52 R
    {{ALTIFY | CTRL, K("\x13"), {0, K("S")}}, // 0x53 S
    {{ALTIFY | CTRL, K("\x14"), {0, K("T")}}, // 0x54 T
    {{ALTIFY | CTRL, K("\x15"), {0, K("U")}}, // 0x55 U
    {{ALTIFY | CTRL, K("\x16"), {0, K("V")}}, // 0x56 V
    {{ALTIFY | CTRL, K("\x17"), {0, K("W")}}, // 0x57 W
    {{ALTIFY | CTRL, K("\x18"), {0, K("X")}}, // 0x58 X
    {{ALTIFY | CTRL, K("\x19"), {0, K("Y")}}, // 0x59 Y
    {{ALTIFY | CTRL, K("\x1a"), {0, K("Z")}}, // 0x5a Z
    {{ALTIFY | CTRL, K("\x1b"), {0, K("[")}}, // 0x5b [
    {{ALTIFY | CTRL, K("\x1c"), {0, K("\\")}}, /* 0x5c \ */
    {{ALTIFY | CTRL, K("\x1d"), {0, K("]")}}, // 0x5d ]
    {{ALTIFY | CTRL, K("\x1e"), {0, K("^")}}, // 0x5e ^
    {{ALTIFY | CTRL, K("\x1f"), {0, K("_")}}, // 0x5f _

    {{ALTIFY | CTRL, K("\x00"), {0, K("`")}}, // 0x60 `
    {{ALTIFY | CTRL, K("\x01"), {0, K("a")}}, // 0x61 a
    {{ALTIFY | CTRL, K("\x02"), {0, K("b")}}, // 0x62 b
    {{ALTIFY | CTRL, K("\x03"), {0, K("c")}}, // 0x63 c
    {{ALTIFY | CTRL, K("\x04"), {0, K("d")}}, // 0x64 d
    {{ALTIFY | CTRL, K("\x05"), {0, K("e")}}, // 0x65 e
    {{ALTIFY | CTRL, K("\x06"), {0, K("f")}}, // 0x66 f
    {{ALTIFY | CTRL, K("\x07"), {0, K("g")}}, // 0x67 g
    {{ALTIFY | CTRL, K("\x08"), {0, K("h")}}, // 0x68 h
    {{ALTIFY | CTRL, K("\x09"), {0, K("i")}}, // 0x69 i
    {{ALTIFY | CTRL, K("\x0a"), {0, K("j")}}, // 0x6a j
    {{ALTIFY | CTRL, K("\x0b"), {0, K("k")}}, // 0x6b k
    {{ALTIFY | CTRL, K("\x0c"), {0, K("l")}}, // 0x6c l
    {{ALTIFY | CTRL, K("\x0d"), {0, K("m")}}, // 0x6d m
    {{ALTIFY | CTRL, K("\x0e"), {0, K("n")}}, // 0x6e n
    {{ALTIFY | CTRL, K("\x0f"), {0, K("o")}}, // 0x6f o

    {{ALTIFY | CTRL, K("\x10"), {0, K("P")}}, // 0x70 p
    {{ALTIFY | CTRL, K("\x11"), {0, K("Q")}}, // 0x71 q
    {{ALTIFY | CTRL, K("\x12"), {0, K("R")}}, // 0x72 r
    {{ALTIFY | CTRL, K("\x13"), {0, K("S")}}, // 0x73 s
    {{ALTIFY | CTRL, K("\x14"), {0, K("T")}}, // 0x74 t
    {{ALTIFY | CTRL, K("\x15"), {0, K("U")}}, // 0x75 u
    {{ALTIFY | CTRL, K("\x16"), {0, K("V")}}, // 0x76 v
    {{ALTIFY | CTRL, K("\x17"), {0, K("W")}}, // 0x77 w
    {{ALTIFY | CTRL, K("\x18"), {0, K("X")}}, // 0x78 x
    {{ALTIFY | CTRL, K("\x19"), {0, K("Y")}}, // 0x79 y
    {{ALTIFY | CTRL, K("\x1a"), {0, K("Z")}}, // 0x7a z
    {{ALTIFY | CTRL, K("\x1b"), {0, K("{")}}, // 0x7b {
    {{ALTIFY | CTRL, K("\x1c"), {0, K("|")}}, // 0x7c |
    {{ALTIFY | CTRL, K("\x1d"), {0, K("}")}}, // 0x7d }
    {{ALTIFY | CTRL, K("\x1e"), {0, K("~")}}, // 0x7e ~
    {{ALTIFY | CTRL, K("\x1f"), {0, K("\x7f")}}, // 0x7f del  // TODO: could not produce this

    // Non-ascii inputs:

    {{0, MODS("\x1b[1;%xH", CURS("\x1bOH", "\x1b[H"))}}, // NAST_KEY_HOME
    {{0, MODS("\x1b[1;%xF", CURS("\x1bOF", "\x1b[F"))}}, // NAST_KEY_END
    {{0, MODS("\x1b[2;%x~", K("\x1b[2~"))}}, // NAST_KEY_INSERT
    {{0, MODS("\x1b[3;%x~", K("\x1b[3~"))}}, // NAST_KEY_DELETE

    {{0, MODS("\x1b[5;%x~", K("\x1b[5~"))}}, // NAST_KEY_PGUP
    {{0, MODS("\x1b[6;%x~", K("\x1b[6~"))}}, // NAST_KEY_PGDN

    {{ALTIFY | CTRL, K("\x7f")}, {0, K("\b")}}, // NAST_KEY_BKSP
    {{0, K("\r")}}, // NAST_KEY_ENTER

    {{0, MODS("\x1b[1;%xA", CURS("\x1bOA", "\x1b[A"))}}, // NAST_KEY_UP
    {{0, MODS("\x1b[1;%xB", CURS("\x1bOB", "\x1b[B"))}}, // NAST_KEY_DN
    {{0, MODS("\x1b[1;%xC", CURS("\x1bOC", "\x1b[C"))}}, // NAST_KEY_RIGHT
    {{0, MODS("\x1b[1;%xD", CURS("\x1bOD", "\x1b[D"))}}, // NAST_KEY_LEFT

    {{0, MODS("\x1b[1;%xP", K("\x1bOP"))}}, // NAST_KEY_F1
    {{0, MODS("\x1b[1;%xQ", K("\x1bOQ"))}}, // NAST_KEY_F2
    {{0, MODS("\x1b[1;%xR", K("\x1bOR"))}}, // NAST_KEY_F3
    {{0, MODS("\x1b[1;%xS", K("\x1bOS"))}}, // NAST_KEY_F4

    {{0, MODS("\x1b[15;%x~", K("\x1b[15~"))}}, // NAST_KEY_F5
                                             ,
    {{0, MODS("\x1b[17;%x~", K("\x1b[17~"))}}, // NAST_KEY_F6
    {{0, MODS("\x1b[18;%x~", K("\x1b[18~"))}}, // NAST_KEY_F7
    {{0, MODS("\x1b[19;%x~", K("\x1b[19~"))}}, // NAST_KEY_F8
    {{0, MODS("\x1b[20;%x~", K("\x1b[20~"))}}, // NAST_KEY_F9
    {{0, MODS("\x1b[21;%x~", K("\x1b[21~"))}}, // NAST_KEY_F10
                                             ,
    {{0, MODS("\x1b[23;%x~", K("\x1b[23~"))}}, // NAST_KEY_F11
    {{0, MODS("\x1b[24;%x~", K("\x1b[24~"))}}, // NAST_KEY_F12

    // TODO: F12-F63 are from xterm's infocmp, can't seem to test them
    // note that e.g. F13 conflicts with a modifer version of F1
    {{0, K("\x1b[1;2P")}}, // NAST_KEY_F13
    {{0, K("\x1b[1;2Q")}}, // NAST_KEY_F14
    {{0, K("\x1b[1;2R")}}, // NAST_KEY_F15
    {{0, K("\x1b[1;2S")}}, // NAST_KEY_F16
    {{0, K("\x1b[15;2~")}}, // NAST_KEY_F17
    {{0, K("\x1b[17;2~")}}, // NAST_KEY_F18
    {{0, K("\x1b[18;2~")}}, // NAST_KEY_F19
    {{0, K("\x1b[19;2~")}}, // NAST_KEY_F20
    {{0, K("\x1b[20;2~")}}, // NAST_KEY_F21
    {{0, K("\x1b[21;2~")}}, // NAST_KEY_F22
    {{0, K("\x1b[23;2~")}}, // NAST_KEY_F23
    {{0, K("\x1b[24;2~")}}, // NAST_KEY_F24
    {{0, K("\x1b[1;5P")}}, // NAST_KEY_F25
    {{0, K("\x1b[1;5Q")}}, // NAST_KEY_F26
    {{0, K("\x1b[1;5R")}}, // NAST_KEY_F27
    {{0, K("\x1b[1;5S")}}, // NAST_KEY_F28
    {{0, K("\x1b[15;5~")}}, // NAST_KEY_F29
    {{0, K("\x1b[17;5~")}}, // NAST_KEY_F30
    {{0, K("\x1b[18;5~")}}, // NAST_KEY_F31
    {{0, K("\x1b[19;5~")}}, // NAST_KEY_F32
    {{0, K("\x1b[20;5~")}}, // NAST_KEY_F33
    {{0, K("\x1b[21;5~")}}, // NAST_KEY_F34
    {{0, K("\x1b[23;5~")}}, // NAST_KEY_F35
    {{0, K("\x1b[24;5~")}}, // NAST_KEY_F36
    {{0, K("\x1b[1;6P")}}, // NAST_KEY_F37
    {{0, K("\x1b[1;6Q")}}, // NAST_KEY_F38
    {{0, K("\x1b[1;6R")}}, // NAST_KEY_F39
    {{0, K("\x1b[1;6S")}}, // NAST_KEY_F40
    {{0, K("\x1b[15;6~")}}, // NAST_KEY_F41
    {{0, K("\x1b[17;6~")}}, // NAST_KEY_F42
    {{0, K("\x1b[18;6~")}}, // NAST_KEY_F43
    {{0, K("\x1b[19;6~")}}, // NAST_KEY_F44
    {{0, K("\x1b[20;6~")}}, // NAST_KEY_F45
    {{0, K("\x1b[21;6~")}}, // NAST_KEY_F46
    {{0, K("\x1b[23;6~")}}, // NAST_KEY_F47
    {{0, K("\x1b[24;6~")}}, // NAST_KEY_F48
    {{0, K("\x1b[1;3P")}}, // NAST_KEY_F49
    {{0, K("\x1b[1;3Q")}}, // NAST_KEY_F50
    {{0, K("\x1b[1;3R")}}, // NAST_KEY_F51
    {{0, K("\x1b[1;3S")}}, // NAST_KEY_F52
    {{0, K("\x1b[15;3~")}}, // NAST_KEY_F53
    {{0, K("\x1b[17;3~")}}, // NAST_KEY_F54
    {{0, K("\x1b[18;3~")}}, // NAST_KEY_F55
    {{0, K("\x1b[19;3~")}}, // NAST_KEY_F56
    {{0, K("\x1b[20;3~")}}, // NAST_KEY_F57
    {{0, K("\x1b[21;3~")}}, // NAST_KEY_F58
    {{0, K("\x1b[23;3~")}}, // NAST_KEY_F59
    {{0, K("\x1b[24;3~")}}, // NAST_KEY_F60
    {{0, K("\x1b[1;4P")}}, // NAST_KEY_F61
    {{0, K("\x1b[1;4Q")}}, // NAST_KEY_F62
    {{0, K("\x1b[1;4R")}}, // NAST_KEY_F63
};



key_action_t *keymap[][4] = {
//   none               ctrl                shift               ctrl+shift
    {NULL,              NULL,               NULL,               NULL}, // 0x00
    {NULL,              NULL,               NULL,               NULL}, // 0x01
    {NULL,              NULL,               NULL,               NULL}, // 0x02
    {NULL,              NULL,               NULL,               NULL}, // 0x03
    {NULL,              NULL,               NULL,               NULL}, // 0x04
    {NULL,              NULL,               NULL,               NULL}, // 0x05
    {NULL,              NULL,               NULL,               NULL}, // 0x06
    {NULL,              NULL,               NULL,               NULL}, // 0x07
    {NULL,              NULL,               NULL,               NULL}, // 0x08
    {NULL,              NULL,               NULL,               NULL}, // 0x09
    {NULL,              NULL,               NULL,               NULL}, // 0x0a
    {NULL,              NULL,               NULL,               NULL}, // 0x0b
    {NULL,              NULL,               NULL,               NULL}, // 0x0c
    {NULL,              NULL,               NULL,               NULL}, // 0x0d
    {NULL,              NULL,               NULL,               NULL}, // 0x0e
    {NULL,              NULL,               NULL,               NULL}, // 0x0f

    {NULL,              NULL,               NULL,               NULL}, // 0x10
    {NULL,              NULL,               NULL,               NULL}, // 0x11
    {NULL,              NULL,               NULL,               NULL}, // 0x12
    {NULL,              NULL,               NULL,               NULL}, // 0x13
    {NULL,              NULL,               NULL,               NULL}, // 0x14
    {NULL,              NULL,               NULL,               NULL}, // 0x15
    {NULL,              NULL,               NULL,               NULL}, // 0x16
    {NULL,              NULL,               NULL,               NULL}, // 0x17
    {NULL,              NULL,               NULL,               NULL}, // 0x18
    {NULL,              NULL,               NULL,               NULL}, // 0x19
    {NULL,              NULL,               NULL,               NULL}, // 0x1a
    {NULL,              NULL,               NULL,               NULL}, // 0x1b
    {NULL,              NULL,               NULL,               NULL}, // 0x1c
    {NULL,              NULL,               NULL,               NULL}, // 0x1d
    {NULL,              NULL,               NULL,               NULL}, // 0x1e
    {NULL,              NULL,               NULL,               NULL}, // 0x1f

    {NULL,              NULL,               NULL,               NULL}, // 0x20
    {NULL,              NULL,               NULL,               NULL}, // 0x21
    {NULL,              NULL,               NULL,               NULL}, // 0x22
    {NULL,              NULL,               NULL,               NULL}, // 0x23
    {NULL,              NULL,               NULL,               NULL}, // 0x24
    {NULL,              NULL,               NULL,               NULL}, // 0x25
    {NULL,              NULL,               NULL,               NULL}, // 0x26
    {NULL,              NULL,               NULL,               NULL}, // 0x27
    {NULL,              NULL,               NULL,               NULL}, // 0x28
    {NULL,              NULL,               NULL,               NULL}, // 0x29
    {NULL,              NULL,               NULL,               NULL}, // 0x2a
    {NULL,              NULL,               NULL,               NULL}, // 0x2b
    {NULL,              NULL,               NULL,               NULL}, // 0x2c
    {NULL,              NULL,               NULL,               NULL}, // 0x2d
    {NULL,              NULL,               NULL,               NULL}, // 0x2e
    {NULL,              K("\x1f"),          NULL,               NULL}, // 0x2f /

    {NULL,              NULL,               NULL,               NULL}, // 0x30 0
    {NULL,              NULL,               NULL,               NULL}, // 0x31 1
    {NULL,              K("\x00"),          NULL,               NULL}, // 0x32 2
    {NULL,              K("\x1b"),          NULL,               NULL}, // 0x33 3
    {NULL,              K("\x1c"),          NULL,               NULL}, // 0x34 4
    {NULL,              K("\x1d"),          NULL,               NULL}, // 0x35 5
    {NULL,              K("\x1e"),          NULL,               NULL}, // 0x36 6
    {NULL,              K("\x1f"),          NULL,               NULL}, // 0x37 7
    {NULL,              NULL,               NULL,               NULL}, // 0x38 8
    {NULL,              NULL,               NULL,               NULL}, // 0x39 9
    {NULL,              NULL,               NULL,               NULL}, // 0x3a
    {NULL,              NULL,               NULL,               NULL}, // 0x3b
    {NULL,              NULL,               NULL,               NULL}, // 0x3c
    {NULL,              NULL,               NULL,               NULL}, // 0x3d
    {NULL,              NULL,               NULL,               NULL}, // 0x3e
    {NULL,              NULL,               NULL,               NULL}, // 0x3f ?

    {NULL,              NULL,               NULL,               NULL}, // 0x40
    {NULL,              K("\x01"),          NULL,               NULL}, // 0x41 A
    {NULL,              K("\x02"),          NULL,               NULL}, // 0x42 B
    {NULL,              K("\x03"),          NULL,               NULL}, // 0x43 C
    {NULL,              K("\x04"),          NULL,               NULL}, // 0x44 D
    {NULL,              K("\x05"),          NULL,               NULL}, // 0x45 E
    {NULL,              K("\x06"),          NULL,               NULL}, // 0x46 F
    {NULL,              K("\x07"),          NULL,               NULL}, // 0x47 G
    {NULL,              K("\x08"),          NULL,               NULL}, // 0x48 H
    {NULL,              K("\x09"),          NULL,               NULL}, // 0x49 I
    {NULL,              K("\x0a"),          NULL,               NULL}, // 0x4a J
    {NULL,              K("\x0b"),          NULL,               NULL}, // 0x4b K
    {NULL,              K("\x0c"),          NULL,               NULL}, // 0x4c L
    {NULL,              K("\x0d"),          NULL,               NULL}, // 0x4d M
    {NULL,              K("\x0e"),          NULL,               NULL}, // 0x4e N
    {NULL,              K("\x0f"),          NULL,               NULL}, // 0x4f O

    {NULL,              K("\x10"),          NULL,               NULL}, // 0x50 P
    {NULL,              K("\x11"),          NULL,               NULL}, // 0x51 Q
    {NULL,              K("\x12"),          NULL,               NULL}, // 0x52 R
    {NULL,              K("\x13"),          NULL,               NULL}, // 0x53 S
    {NULL,              K("\x14"),          NULL,               NULL}, // 0x54 T
    {NULL,              K("\x15"),          NULL,               NULL}, // 0x55 U
    {NULL,              K("\x16"),          NULL,               NULL}, // 0x56 V
    {NULL,              K("\x17"),          NULL,               NULL}, // 0x57 W
    {NULL,              K("\x18"),          NULL,               NULL}, // 0x58 X
    {NULL,              K("\x19"),          NULL,               NULL}, // 0x59 Y
    {NULL,              K("\x1a"),          NULL,               NULL}, // 0x5a Z
    {NULL,              NULL,               NULL,               NULL}, // 0x5b
    {NULL,              NULL,               NULL,               NULL}, // 0x5c
    {NULL,              NULL,               NULL,               NULL}, // 0x5d
    {NULL,              NULL,               NULL,               NULL}, // 0x5e
    {NULL,              NULL,               NULL,               NULL}, // 0x5f

    {NULL,              NULL,               NULL,               NULL}, // 0x60
    {NULL,              K("\x01"),          NULL,               NULL}, // 0x61 a
    {NULL,              K("\x02"),          NULL,               NULL}, // 0x62 b
    {NULL,              K("\x03"),          NULL,               NULL}, // 0x63 c
    {NULL,              K("\x04"),          NULL,               NULL}, // 0x64 d
    {NULL,              K("\x05"),          NULL,               NULL}, // 0x65 e
    {NULL,              K("\x06"),          NULL,               NULL}, // 0x66 f
    {NULL,              K("\x07"),          NULL,               NULL}, // 0x67 g
    {NULL,              K("\x08"),          NULL,               NULL}, // 0x68 h
    {NULL,              K("\x09"),          NULL,               NULL}, // 0x69 i
    {NULL,              K("\x0a"),          NULL,               NULL}, // 0x6a j
    {NULL,              K("\x0b"),          NULL,               NULL}, // 0x6b k
    {NULL,              K("\x0c"),          NULL,               NULL}, // 0x6c l
    {NULL,              K("\x0d"),          NULL,               NULL}, // 0x6d m
    {NULL,              K("\x0e"),          NULL,               NULL}, // 0x6e n
    {NULL,              K("\x0f"),          NULL,               NULL}, // 0x6f o

    {NULL,              K("\x10"),          NULL,               NULL}, // 0x70 p
    {NULL,              K("\x11"),          NULL,               NULL}, // 0x71 q
    {NULL,              K("\x12"),          NULL,               NULL}, // 0x72 r
    {NULL,              K("\x13"),          NULL,               NULL}, // 0x73 s
    {NULL,              K("\x14"),          NULL,               NULL}, // 0x74 t
    {NULL,              K("\x15"),          NULL,               NULL}, // 0x75 u
    {NULL,              K("\x16"),          NULL,               NULL}, // 0x76 v
    {NULL,              K("\x17"),          NULL,               NULL}, // 0x77 w
    {NULL,              K("\x18"),          NULL,               NULL}, // 0x78 x
    {NULL,              K("\x19"),          NULL,               NULL}, // 0x79 y
    {NULL,              K("\x1a"),          NULL,               NULL}, // 0x7a z
    {NULL,              NULL,               NULL,               NULL}, // 0x7b
    {NULL,              NULL,               NULL,               NULL}, // 0x7c
    {NULL,              NULL,               NULL,               NULL}, // 0x7d
    {NULL,              NULL,               NULL,               NULL}, // 0x7e
    {NULL,              NULL,               NULL,               NULL}, // 0x7f

// NON-ASCII KEYMAP


    // home                                 kHOM
    {K("\x1b[H"),       K("\x1b[H"),        K("\x1b[2J"),    K("\x1b[H")}, // NAST_KEY_HOME
    // smir             il1
    {K("\x1b[4h"),      K("\x1b[L"),        A(shift_insert), NULL}, // NAST_KEY_INSERT
    // [2]                                  [2]
    {K("\x1b[3~"),      NULL,               K("\x1b[2K"),    NULL}, // NAST_KEY_DELETE
    // [2]                                  [2]
    {K("\x1b[4~"),      NULL,               K("\x1b[K"),     NULL}, // NAST_KEY_END
    // [2]
    {K("\x1b[5~"),      NULL,               A(shift_pgup),   NULL}, // NAST_KEY_PGUP
    // [2]
    {K("\x1b[6~"),      NULL,               A(shift_pgdn),   NULL}, // NAST_KEY_PGDN

    // [2]              [2]                 [2]              [2]
    {K("\x7f"),         K("\x17"),          K("\x08"),       K("\x17")}, // NAST_KEY_BKSP
    {K("\x7f"),         K("\x17"),          K("\x08"),       K("\x17")}, // NAST_KEY_ENTER

    // arrow keys
    // https://vt100.net/docs/vt100-ug/chapter3.html#S3.3 table 3-6
    // {C(K("\x1b[A"), K("\x1bOA")),      NULL,               A(shift_pgdn),   NULL}, // NAST_KEY_UP
};

#undef K
#undef A
