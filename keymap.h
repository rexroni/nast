typedef struct {
    char *text;
    size_t len;
} simple_key_t;

typedef enum {
    KEY_ACTION_SIMPLE,
    KEY_ACTION_MODS,
    KEY_ACTION_FUNC,
} key_action_type_e;

typedef union {
    simple_key_t simple;
    char *mods;
    void (*func)(void *globals, GdkEventKey *event_key);
} key_action_u;

typedef struct {
    key_action_type_e type;
    key_action_u val;
} key_action_t;

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

// "modifiers"
// xterm's 1 + (8=meta, 4=ctrl, 2=shift, 1=alt) scheme
// (note this does not work with ALTIFY at all)
// (see man 5 user_caps)
#define M(pattern) &(key_action_t){ \
    .type=KEY_ACTION_MODS, \
    .val={ \
        .mods=pattern \
    } \
}

// "function"
#define F(fn) &(key_action_t){ \
    .type=KEY_ACTION_FUNC, \
    .val={ \
        .func=fn \
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

// 256 = match appcursor
// 512 = which appcursor matches
#define MATCH_CURS 256
#define CURS_MASK 512
#define NOCURS 256
#define CURS 768

// 1024 = match appkeypad
// 2048 = which appkeypad matches
#define MATCH_KPAD 1024
#define KPAD_MASK 2048
#define NOKPAD 1024
#define KPAD 3072

// 4096 = match modifyotherkeys.lvl=1
// 8192 = which modifyotherkeys.lvl=1 matches
#define MATCH_MOK1 4096
#define MOK1_MASK 8192
#define NOMOK1 4096
#define MOK1 12288

// 16384 = match modifyotherkeys.lvl=2
// 32768 = which modifyotherkeys.lvl=2 matches
#define MATCH_MOK2 16384
#define MOK2_MASK 32768
#define NOMOK2 16384
#define MOK2 49152

// "not mods", aka not ctrl, alt, shift, or meta
#define NM (NOCTRL|NOSHIFT|NOALT|NOMETA)

#define MOD_SELECTOR ( \
    MATCH_CTRL | MATCH_SHIFT | MATCH_ALT | MATCH_META \
    | MATCH_CURS | MATCH_KPAD | MATCH_MOK1 | MATCH_MOK2 \
)

// ALTIFY, not a mask but modifies how the list should behave
#define ALTIFY 2147483648

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

const unsigned int NAST_KEY_KP0 = 0x8e;
const unsigned int NAST_KEY_KP1 = 0x8f;
const unsigned int NAST_KEY_KP2 = 0x90;
const unsigned int NAST_KEY_KP3 = 0x91;
const unsigned int NAST_KEY_KP4 = 0x92;
const unsigned int NAST_KEY_KP5 = 0x93;
const unsigned int NAST_KEY_KP6 = 0x94;
const unsigned int NAST_KEY_KP7 = 0x95;
const unsigned int NAST_KEY_KP8 = 0x96;
const unsigned int NAST_KEY_KP9 = 0x97;

const unsigned int NAST_KEY_KPASTERISK = 0x98;
const unsigned int NAST_KEY_KPMINUS = 0x99;
const unsigned int NAST_KEY_KPPLUS = 0x9a;
const unsigned int NAST_KEY_KPCOMMA = 0x9b;
const unsigned int NAST_KEY_KPSLASH = 0x9c;
const unsigned int NAST_KEY_KPENTER = 0x9d;

// "u"nlocked keypad numbers (without numlock set)
const unsigned int NAST_KEY_KP0u = 0x9e;
const unsigned int NAST_KEY_KP1u = 0x9f;
const unsigned int NAST_KEY_KP2u = 0xa0;
const unsigned int NAST_KEY_KP3u = 0xa1;
const unsigned int NAST_KEY_KP4u = 0xa2;
const unsigned int NAST_KEY_KP5u = 0xa3;
const unsigned int NAST_KEY_KP6u = 0xa4;
const unsigned int NAST_KEY_KP7u = 0xa5;
const unsigned int NAST_KEY_KP8u = 0xa6;
const unsigned int NAST_KEY_KP9u = 0xa7;

const unsigned int NAST_KEY_KPCOMMAu = 0xa8;

const unsigned int NAST_KEY_F1 = 0xa9;
const unsigned int NAST_KEY_F2 = 0xaa;
const unsigned int NAST_KEY_F3 = 0xab;
const unsigned int NAST_KEY_F4 = 0xac;
const unsigned int NAST_KEY_F5 = 0xad;
const unsigned int NAST_KEY_F6 = 0xae;
const unsigned int NAST_KEY_F7 = 0xaf;
const unsigned int NAST_KEY_F8 = 0xb0;
const unsigned int NAST_KEY_F9 = 0xb1;
const unsigned int NAST_KEY_F10 = 0xb2;
const unsigned int NAST_KEY_F11 = 0xb3;
const unsigned int NAST_KEY_F12 = 0xb4;
const unsigned int NAST_KEY_F13 = 0xb5;
const unsigned int NAST_KEY_F14 = 0xb6;
const unsigned int NAST_KEY_F15 = 0xb7;
const unsigned int NAST_KEY_F16 = 0xb8;
const unsigned int NAST_KEY_F17 = 0xb9;
const unsigned int NAST_KEY_F18 = 0xba;
const unsigned int NAST_KEY_F19 = 0xbb;
const unsigned int NAST_KEY_F20 = 0xbc;
const unsigned int NAST_KEY_F21 = 0xbd;
const unsigned int NAST_KEY_F22 = 0xbe;
const unsigned int NAST_KEY_F23 = 0xbf;
const unsigned int NAST_KEY_F24 = 0xc0;
const unsigned int NAST_KEY_F25 = 0xc1;
const unsigned int NAST_KEY_F26 = 0xc2;
const unsigned int NAST_KEY_F27 = 0xc3;
const unsigned int NAST_KEY_F28 = 0xc4;
const unsigned int NAST_KEY_F29 = 0xc5;
const unsigned int NAST_KEY_F30 = 0xc6;
const unsigned int NAST_KEY_F31 = 0xc7;
const unsigned int NAST_KEY_F32 = 0xc8;
const unsigned int NAST_KEY_F33 = 0xc9;
const unsigned int NAST_KEY_F34 = 0xca;
const unsigned int NAST_KEY_F35 = 0xcb;
const unsigned int NAST_KEY_F36 = 0xcc;
const unsigned int NAST_KEY_F37 = 0xcd;
const unsigned int NAST_KEY_F38 = 0xce;
const unsigned int NAST_KEY_F39 = 0xcf;
const unsigned int NAST_KEY_F40 = 0xd0;
const unsigned int NAST_KEY_F41 = 0xd1;
const unsigned int NAST_KEY_F42 = 0xd2;
const unsigned int NAST_KEY_F43 = 0xd3;
const unsigned int NAST_KEY_F44 = 0xd4;
const unsigned int NAST_KEY_F45 = 0xd5;
const unsigned int NAST_KEY_F46 = 0xd6;
const unsigned int NAST_KEY_F47 = 0xd7;
const unsigned int NAST_KEY_F48 = 0xd8;
const unsigned int NAST_KEY_F49 = 0xd9;
const unsigned int NAST_KEY_F50 = 0xda;
const unsigned int NAST_KEY_F51 = 0xdb;
const unsigned int NAST_KEY_F52 = 0xdc;
const unsigned int NAST_KEY_F53 = 0xdd;
const unsigned int NAST_KEY_F54 = 0xde;
const unsigned int NAST_KEY_F55 = 0xdf;
const unsigned int NAST_KEY_F56 = 0xe0;
const unsigned int NAST_KEY_F57 = 0xe1;
const unsigned int NAST_KEY_F58 = 0xe2;
const unsigned int NAST_KEY_F59 = 0xe3;
const unsigned int NAST_KEY_F60 = 0xe4;
const unsigned int NAST_KEY_F61 = 0xe5;
const unsigned int NAST_KEY_F62 = 0xe6;
const unsigned int NAST_KEY_F63 = 0xe7;


// The goal is to masquerade as xterm.  Everything seems to work with xterm.
// That's funny, because it used to be that xterm worked with everything, but
// it did that so well that now it's the other way around.
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

#define X(...) (key_map_t[]){__VA_ARGS__}

key_map_t *keymap[] = {
    // TODO: 0x00 through 0x1f I am unable to test
    X({0, K("\x00")}), // 0x00
    X({0, K("\x01")}), // 0x01
    X({0, K("\x02")}), // 0x02
    X({0, K("\x03")}), // 0x03
    X({0, K("\x04")}), // 0x04
    X({0, K("\x05")}), // 0x05
    X({0, K("\x06")}), // 0x06
    X({0, K("\x07")}), // 0x07
    X({0, K("\x08")}), // 0x08
    X({0, K("\x09")}), // 0x09
    X({0, K("\x0a")}), // 0x0a
    X({0, K("\x0b")}), // 0x0b
    X({0, K("\x0c")}), // 0x0c
    X({0, K("\x0d")}), // 0x0d
    X({0, K("\x0e")}), // 0x0e
    X({0, K("\x0f")}), // 0x0f

    X({0, K("\x10")}), // 0x10
    X({0, K("\x11")}), // 0x11
    X({0, K("\x12")}), // 0x12
    X({0, K("\x13")}), // 0x13
    X({0, K("\x14")}), // 0x14
    X({0, K("\x15")}), // 0x15
    X({0, K("\x16")}), // 0x16
    X({0, K("\x17")}), // 0x17
    X({0, K("\x18")}), // 0x18
    X({0, K("\x19")}), // 0x19
    X({0, K("\x1a")}), // 0x1a
    X({0, K("\x1b")}), // 0x1b
    X({0, K("\x1c")}), // 0x1c
    X({0, K("\x1d")}), // 0x1d
    X({0, K("\x1e")}), // 0x1e
    X({0, K("\x1f")}), // 0x1f

    X({ALTIFY | CTRL_, K("\0")}, {0, K(" ")}), // 0x20 space
    X({ALTIFY, K("!")}), // 0x21 !
    X({ALTIFY, K("\"")}), // 0x22 "
    X({ALTIFY, K("#")}), // 0x23 #
    X({ALTIFY, K("$")}), // 0x24 $
    X({ALTIFY, K("%")}), // 0x25 %
    X({ALTIFY, K("&")}), // 0x26 &
    X({ALTIFY, K("'")}), // 0x27 '
    X({ALTIFY, K("(")}), // 0x28 (
    X({ALTIFY, K(")")}), // 0x29 )
    X({ALTIFY, K("*")}), // 0x2a *
    X({ALTIFY, K("+")}), // 0x2b +
    X({ALTIFY, K(",")}), // 0x2c ,
    X({ALTIFY, K("-")}), // 0x2d -
    X({ALTIFY, K(".")}), // 0x2e .
    X({ALTIFY | CTRL_, K("\x1f")}, {0, K("/")}), // 0x2f /

    X({ALTIFY, K("0")}), // 0x30 0
    X({ALTIFY, K("1")}), // 0x31 1
    X({ALTIFY | CTRL_, K("\x00")}, {0, K("2")}), // 0x32 2
    X({ALTIFY | CTRL_, K("\x1b")}, {0, K("3")}), // 0x33 3
    X({ALTIFY | CTRL_, K("\x1c")}, {0, K("4")}), // 0x34 4
    X({ALTIFY | CTRL_, K("\x1d")}, {0, K("5")}), // 0x35 5
    X({ALTIFY | CTRL_, K("\x1e")}, {0, K("6")}), // 0x36 6
    X({ALTIFY | CTRL_, K("\x1f")}, {0, K("7")}), // 0x37 7
    X({ALTIFY | CTRL_, K("\x7f")}, {0, K("8")}), // 0x38 8
    X({ALTIFY, K("9")}), // 0x39 9
    X({ALTIFY, K(":")}), // 0x3a :
    X({ALTIFY, K(";")}), // 0x3b ;
    X({ALTIFY, K("<")}), // 0x3c <
    X({ALTIFY, K("=")}), // 0x3d =
    X({ALTIFY, K(">")}), // 0x3e >
    X({ALTIFY | CTRL_, K("\x7f")}, {0, K("?")}), // 0x3f ?

    X({ALTIFY | CTRL_, K("\x00")}, {0, K("@")}), // 0x40 @
    X({ALTIFY | CTRL_, K("\x01")}, {0, K("A")}), // 0x41 A
    X({ALTIFY | CTRL_, K("\x02")}, {0, K("B")}), // 0x42 B
    X({ALTIFY | CTRL_, K("\x03")}, {0, K("C")}), // 0x43 C
    X({ALTIFY | CTRL_, K("\x04")}, {0, K("D")}), // 0x44 D
    X({ALTIFY | CTRL_, K("\x05")}, {0, K("E")}), // 0x45 E
    X({ALTIFY | CTRL_, K("\x06")}, {0, K("F")}), // 0x46 F
    X({ALTIFY | CTRL_, K("\x07")}, {0, K("G")}), // 0x47 G
    X({ALTIFY | CTRL_, K("\x08")}, {0, K("H")}), // 0x48 H
    X({ALTIFY | CTRL_, K("\x09")}, {0, K("I")}), // 0x49 I
    X({ALTIFY | CTRL_, K("\x0a")}, {0, K("J")}), // 0x4a J
    X({ALTIFY | CTRL_, K("\x0b")}, {0, K("K")}), // 0x4b K
    X({ALTIFY | CTRL_, K("\x0c")}, {0, K("L")}), // 0x4c L
    X({ALTIFY | CTRL_, K("\x0d")}, {0, K("M")}), // 0x4d M
    X({ALTIFY | CTRL_, K("\x0e")}, {0, K("N")}), // 0x4e N
    X({ALTIFY | CTRL_, K("\x0f")}, {0, K("O")}), // 0x4f O

    X({ALTIFY | CTRL_, K("\x10")}, {0, K("P")}), // 0x50 P
    X({ALTIFY | CTRL_, K("\x11")}, {0, K("Q")}), // 0x51 Q
    X({ALTIFY | CTRL_, K("\x12")}, {0, K("R")}), // 0x52 R
    X({ALTIFY | CTRL_, K("\x13")}, {0, K("S")}), // 0x53 S
    X({ALTIFY | CTRL_, K("\x14")}, {0, K("T")}), // 0x54 T
    X({ALTIFY | CTRL_, K("\x15")}, {0, K("U")}), // 0x55 U
    X({ALTIFY | CTRL_, K("\x16")}, {0, K("V")}), // 0x56 V
    X({ALTIFY | CTRL_, K("\x17")}, {0, K("W")}), // 0x57 W
    X({ALTIFY | CTRL_, K("\x18")}, {0, K("X")}), // 0x58 X
    X({ALTIFY | CTRL_, K("\x19")}, {0, K("Y")}), // 0x59 Y
    X({ALTIFY | CTRL_, K("\x1a")}, {0, K("Z")}), // 0x5a Z
    X({ALTIFY | CTRL_, K("\x1b")}, {0, K("[")}), // 0x5b [
    X({ALTIFY | CTRL_, K("\x1c")}, {0, K("\\")}), /* 0x5c \ */
    X({ALTIFY | CTRL_, K("\x1d")}, {0, K("]")}), // 0x5d ]
    X({ALTIFY | CTRL_, K("\x1e")}, {0, K("^")}), // 0x5e ^
    X({ALTIFY | CTRL_, K("\x1f")}, {0, K("_")}), // 0x5f _

    X({ALTIFY | CTRL_, K("\x00")}, {0, K("`")}), // 0x60 `
    X({ALTIFY | CTRL_, K("\x01")}, {0, K("a")}), // 0x61 a
    X({ALTIFY | CTRL_, K("\x02")}, {0, K("b")}), // 0x62 b
    X({ALTIFY | CTRL_, K("\x03")}, {0, K("c")}), // 0x63 c
    X({ALTIFY | CTRL_, K("\x04")}, {0, K("d")}), // 0x64 d
    X({ALTIFY | CTRL_, K("\x05")}, {0, K("e")}), // 0x65 e
    X({ALTIFY | CTRL_, K("\x06")}, {0, K("f")}), // 0x66 f
    X({ALTIFY | CTRL_, K("\x07")}, {0, K("g")}), // 0x67 g
    X({ALTIFY | CTRL_, K("\x08")}, {0, K("h")}), // 0x68 h
    X({ALTIFY | CTRL_, K("\x09")}, {0, K("i")}), // 0x69 i
    X({ALTIFY | CTRL_, K("\x0a")}, {0, K("j")}), // 0x6a j
    X({ALTIFY | CTRL_, K("\x0b")}, {0, K("k")}), // 0x6b k
    X({ALTIFY | CTRL_, K("\x0c")}, {0, K("l")}), // 0x6c l
    X({ALTIFY | CTRL_, K("\x0d")}, {0, K("m")}), // 0x6d m
    X({ALTIFY | CTRL_, K("\x0e")}, {0, K("n")}), // 0x6e n
    X({ALTIFY | CTRL_, K("\x0f")}, {0, K("o")}), // 0x6f o

    X({ALTIFY | CTRL_, K("\x10")}, {0, K("p")}), // 0x70 p
    X({ALTIFY | CTRL_, K("\x11")}, {0, K("q")}), // 0x71 q
    X({ALTIFY | CTRL_, K("\x12")}, {0, K("r")}), // 0x72 r
    X({ALTIFY | CTRL_, K("\x13")}, {0, K("s")}), // 0x73 s
    X({ALTIFY | CTRL_, K("\x14")}, {0, K("t")}), // 0x74 t
    X({ALTIFY | CTRL_, K("\x15")}, {0, K("u")}), // 0x75 u
    X({ALTIFY | CTRL_, K("\x16")}, {0, K("v")}), // 0x76 v
    X({ALTIFY | CTRL_, K("\x17")}, {0, K("w")}), // 0x77 w
    X({ALTIFY | CTRL_, K("\x18")}, {0, K("x")}), // 0x78 x
    X({ALTIFY | CTRL_, K("\x19")}, {0, K("y")}), // 0x79 y
    X({ALTIFY | CTRL_, K("\x1a")}, {0, K("z")}), // 0x7a z
    X({ALTIFY | CTRL_, K("\x1b")}, {0, K("{")}), // 0x7b {
    X({ALTIFY | CTRL_, K("\x1c")}, {0, K("|")}), // 0x7c |
    X({ALTIFY | CTRL_, K("\x1d")}, {0, K("}")}), // 0x7d }
    X({ALTIFY | CTRL_, K("\x1e")}, {0, K("~")}), // 0x7e ~
    X({ALTIFY | CTRL_, K("\x1f")}, {0, K("\x7f")}), // 0x7f del  // TODO: could not produce this

    // Non-ascii inputs:

    X({NM|NOCURS, K("\x1b[H")}, {NM, K("\x1bOH")}, {0, M("\x1b[1;%dH")}), // NAST_KEY_HOME
    X({NM|NOCURS, K("\x1b[F")}, {NM, K("\x1bOF")}, {0, M("\x1b[1;%dF")}), // NAST_KEY_END
    X({NM, K("\x1b[2~")}, {0, M("\x1b[2;%d~")}), // NAST_KEY_INSERT
    X({NM, K("\x1b[3~")}, {0, M("\x1b[3;%d~")}), // NAST_KEY_DELETE

    X({SHIFT, F(shift_pgup)}, {NM, K("\x1b[5~")}, {0, M("\x1b[5;%d~")}), // NAST_KEY_PGUP
    X({SHIFT, F(shift_pgdn)}, {NM, K("\x1b[6~")}, {0, M("\x1b[6;%d~")}), // NAST_KEY_PGDN

    X({ALTIFY | CTRL_, K("\x7f")}, {0, K("\b")}), // NAST_KEY_BKSP
    X({NOALT, K("\r")}, {0, K("")}), // NAST_KEY_ENTER
    X({SHIFT, K("\x1b[Z")}, {ALT, K("\xc2\x89")}, {0, K("\t")}), // NAST_KEY_TAB
    X({0, K("\x1b")}), // NAST_KEY_ESC

    X({NM|NOCURS, K("\x1b[A")}, {NM, K("\x1bOA")}, {0, M("\x1b[1;%dA")}), // NAST_KEY_UP
    X({NM|NOCURS, K("\x1b[B")}, {NM, K("\x1bOB")}, {0, M("\x1b[1;%dB")}), // NAST_KEY_DN
    X({NM|NOCURS, K("\x1b[C")}, {NM, K("\x1bOC")}, {0, M("\x1b[1;%dC")}), // NAST_KEY_RIGHT
    X({NM|NOCURS, K("\x1b[D")}, {NM, K("\x1bOD")}, {0, M("\x1b[1;%dD")}), // NAST_KEY_LEFT

    // keypad inputs:

    // kp0-kp9 only appear with numlock on, and only when shift is not pressed
    X({KPAD|NM, K("\x1bOp")}, {KPAD, M("\x1bO%dp")}, {0, K("0")}), // NAST_KEY_KP0
    X({KPAD|NM, K("\x1bOq")}, {KPAD, M("\x1bO%dq")}, {0, K("1")}), // NAST_KEY_KP1
    X({KPAD|NM, K("\x1bOr")}, {KPAD, M("\x1bO%dr")}, {0, K("2")}), // NAST_KEY_KP2
    X({KPAD|NM, K("\x1bOs")}, {KPAD, M("\x1bO%ds")}, {0, K("3")}), // NAST_KEY_KP3
    X({KPAD|NM, K("\x1bOt")}, {KPAD, M("\x1bO%dt")}, {0, K("4")}), // NAST_KEY_KP4
    X({KPAD|NM, K("\x1bOu")}, {KPAD, M("\x1bO%du")}, {0, K("5")}), // NAST_KEY_KP5
    X({KPAD|NM, K("\x1bOv")}, {KPAD, M("\x1bO%dv")}, {0, K("6")}), // NAST_KEY_KP6
    X({KPAD|NM, K("\x1bOw")}, {KPAD, M("\x1bO%dw")}, {0, K("7")}), // NAST_KEY_KP7
    X({KPAD|NM, K("\x1bOx")}, {KPAD, M("\x1bO%dx")}, {0, K("8")}), // NAST_KEY_KP8
    X({KPAD|NM, K("\x1bOy")}, {KPAD, M("\x1bO%dy")}, {0, K("9")}), // NAST_KEY_KP9

    X({KPAD|NM, K("\x1bOj")},                 {KPAD, M("\x1bO%dj")}, {0, K("*")}), // NAST_KEY_KPASTERISK
    X({KPAD|NM, K("\x1bOm")}, {SHIFT, K("")}, {KPAD, M("\x1bO%dm")}, {0, K("-")}), // NAST_KEY_KPMINUS
    X({KPAD|NM, K("\x1bOk")}, {SHIFT, K("")}, {KPAD, M("\x1bO%dk")}, {0, K("+")}), // NAST_KEY_KPPLUS
    X({KPAD|NM, K("\x1bOn")},                 {KPAD, M("\x1bO%dn")}, {0, K(".")}), // NAST_KEY_KPCOMMA
    X({KPAD|NM, K("\x1bOo")},                 {KPAD, M("\x1bO%do")}, {0, K("/")}), // NAST_KEY_KPSLASH
    X({KPAD|NM, K("\x1bOM")},                 {KPAD, M("\x1bO%dM")}, {0, K("\r")}), // NAST_KEY_KPENTER

    // keypad numbers without numlock
    X({NM, K("\x1b[2~")}, {0, M("\x1b[2;%d~")}), // NAST_KEY_KP0u
    X({NM, K("\x1b[F")},  {0, M("\x1b[1;%dF")}), // NAST_KEY_KP1u
    X({NM, K("\x1b[B")},  {0, M("\x1b[1;%dB")}), // NAST_KEY_KP2u
    X({NM, K("\x1b[6~")}, {0, M("\x1b[6;%d~")}), // NAST_KEY_KP3u
    X({NM, K("\x1b[D")},  {0, M("\x1b[1;%dD")}), // NAST_KEY_KP4u
    X({NM, K("\x1b[E")},  {0, M("\x1b[1;%dE")}), // NAST_KEY_KP5u
    X({NM, K("\x1b[C")},  {0, M("\x1b[1;%dC")}), // NAST_KEY_KP6u
    X({NM, K("\x1b[H")},  {0, M("\x1b[1;%dH")}), // NAST_KEY_KP7u
    X({NM, K("\x1b[A")},  {0, M("\x1b[1;%dA")}), // NAST_KEY_KP8u
    X({NM, K("\x1b[5~")}, {0, M("\x1b[5;%d~")}), // NAST_KEY_KP9u

    X({0, K(".")}), // NAST_KEY_KPCOMMAu

    X({NM, K("\x1bOP")}, {0, M("\x1b[1;%dP")}), // NAST_KEY_F1
    X({NM, K("\x1bOQ")}, {0, M("\x1b[1;%dQ")}), // NAST_KEY_F2
    X({NM, K("\x1bOR")}, {0, M("\x1b[1;%dR")}), // NAST_KEY_F3
    X({NM, K("\x1bOS")}, {0, M("\x1b[1;%dS")}), // NAST_KEY_F4

    X({NM, K("\x1b[15~")}, {0, M("\x1b[15;%d~")}), // NAST_KEY_F5

    X({NM, K("\x1b[17~")}, {0, M("\x1b[17;%d~")}), // NAST_KEY_F6
    X({NM, K("\x1b[18~")}, {0, M("\x1b[18;%d~")}), // NAST_KEY_F7
    X({NM, K("\x1b[19~")}, {0, M("\x1b[19;%d~")}), // NAST_KEY_F8
    X({NM, K("\x1b[20~")}, {0, M("\x1b[20;%d~")}), // NAST_KEY_F9
    X({NM, K("\x1b[21~")}, {0, M("\x1b[21;%d~")}), // NAST_KEY_F10

    X({NM, K("\x1b[23~")}, {0, M("\x1b[23;%d~")}), // NAST_KEY_F11
    X({NM, K("\x1b[24~")}, {0, M("\x1b[24;%d~")}), // NAST_KEY_F12

    // TODO: F12-F63 are from xterm's infocmp, can't seem to test them
    // note that e.g. F13 conflicts with a modifer version of F1
    X({0, K("\x1b[1;2P")}), // NAST_KEY_F13
    X({0, K("\x1b[1;2Q")}), // NAST_KEY_F14
    X({0, K("\x1b[1;2R")}), // NAST_KEY_F15
    X({0, K("\x1b[1;2S")}), // NAST_KEY_F16
    X({0, K("\x1b[15;2~")}), // NAST_KEY_F17
    X({0, K("\x1b[17;2~")}), // NAST_KEY_F18
    X({0, K("\x1b[18;2~")}), // NAST_KEY_F19
    X({0, K("\x1b[19;2~")}), // NAST_KEY_F20
    X({0, K("\x1b[20;2~")}), // NAST_KEY_F21
    X({0, K("\x1b[21;2~")}), // NAST_KEY_F22
    X({0, K("\x1b[23;2~")}), // NAST_KEY_F23
    X({0, K("\x1b[24;2~")}), // NAST_KEY_F24
    X({0, K("\x1b[1;5P")}), // NAST_KEY_F25
    X({0, K("\x1b[1;5Q")}), // NAST_KEY_F26
    X({0, K("\x1b[1;5R")}), // NAST_KEY_F27
    X({0, K("\x1b[1;5S")}), // NAST_KEY_F28
    X({0, K("\x1b[15;5~")}), // NAST_KEY_F29
    X({0, K("\x1b[17;5~")}), // NAST_KEY_F30
    X({0, K("\x1b[18;5~")}), // NAST_KEY_F31
    X({0, K("\x1b[19;5~")}), // NAST_KEY_F32
    X({0, K("\x1b[20;5~")}), // NAST_KEY_F33
    X({0, K("\x1b[21;5~")}), // NAST_KEY_F34
    X({0, K("\x1b[23;5~")}), // NAST_KEY_F35
    X({0, K("\x1b[24;5~")}), // NAST_KEY_F36
    X({0, K("\x1b[1;6P")}), // NAST_KEY_F37
    X({0, K("\x1b[1;6Q")}), // NAST_KEY_F38
    X({0, K("\x1b[1;6R")}), // NAST_KEY_F39
    X({0, K("\x1b[1;6S")}), // NAST_KEY_F40
    X({0, K("\x1b[15;6~")}), // NAST_KEY_F41
    X({0, K("\x1b[17;6~")}), // NAST_KEY_F42
    X({0, K("\x1b[18;6~")}), // NAST_KEY_F43
    X({0, K("\x1b[19;6~")}), // NAST_KEY_F44
    X({0, K("\x1b[20;6~")}), // NAST_KEY_F45
    X({0, K("\x1b[21;6~")}), // NAST_KEY_F46
    X({0, K("\x1b[23;6~")}), // NAST_KEY_F47
    X({0, K("\x1b[24;6~")}), // NAST_KEY_F48
    X({0, K("\x1b[1;3P")}), // NAST_KEY_F49
    X({0, K("\x1b[1;3Q")}), // NAST_KEY_F50
    X({0, K("\x1b[1;3R")}), // NAST_KEY_F51
    X({0, K("\x1b[1;3S")}), // NAST_KEY_F52
    X({0, K("\x1b[15;3~")}), // NAST_KEY_F53
    X({0, K("\x1b[17;3~")}), // NAST_KEY_F54
    X({0, K("\x1b[18;3~")}), // NAST_KEY_F55
    X({0, K("\x1b[19;3~")}), // NAST_KEY_F56
    X({0, K("\x1b[20;3~")}), // NAST_KEY_F57
    X({0, K("\x1b[21;3~")}), // NAST_KEY_F58
    X({0, K("\x1b[23;3~")}), // NAST_KEY_F59
    X({0, K("\x1b[24;3~")}), // NAST_KEY_F60
    X({0, K("\x1b[1;4P")}), // NAST_KEY_F61
    X({0, K("\x1b[1;4Q")}), // NAST_KEY_F62
    X({0, K("\x1b[1;4R")}), // NAST_KEY_F63
};

#undef K
#undef F
#undef CURS
#undef KPAD
#undef MODS
