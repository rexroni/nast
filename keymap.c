#include <stdint.h>
#include <stddef.h>

#include "keymap.h"

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
    .type=KEY_ACTION_ ## fn, \
}

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

    X({ALTIFY | MOK2, M("\x1b[27;%d;32~")}, {CTRL_, K("\0")}, {0, K(" ")}), // 0x20 space
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("!")}, {CTRL_|MOK1, M("\x1b[27;%d;33~")}, {MOK2, M("\x1b[27;%d;33~")}, {0, K("!")}), // 0x21 !
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("\"")}, {CTRL_|MOK1, M("\x1b[27;%d;34~")}, {MOK2, M("\x1b[27;%d;34~")}, {0, K("\"")}), // 0x22 "
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("#")}, {CTRL_|MOK1, M("\x1b[27;%d;35~")}, {MOK2, M("\x1b[27;%d;35~")}, {0, K("#")}), // 0x23 #
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("$")}, {CTRL_|MOK1, M("\x1b[27;%d;36~")}, {MOK2, M("\x1b[27;%d;36~")}, {0, K("$")}), // 0x24 $
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("%")}, {CTRL_|MOK1, M("\x1b[27;%d;37~")}, {MOK2, M("\x1b[27;%d;37~")}, {0, K("%")}), // 0x25 %
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("&")}, {CTRL_|MOK1, M("\x1b[27;%d;38~")}, {MOK2, M("\x1b[27;%d;38~")}, {0, K("&")}), // 0x26 &
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("'")}, {CTRL_|MOK1, M("\x1b[27;%d;39~")}, {MOK2, M("\x1b[27;%d;39~")}, {0, K("'")}), // 0x27 '
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("(")}, {CTRL_|MOK1, M("\x1b[27;%d;40~")}, {MOK2, M("\x1b[27;%d;40~")}, {0, K("(")}), // 0x28 (
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K(")")}, {CTRL_|MOK1, M("\x1b[27;%d;41~")}, {MOK2, M("\x1b[27;%d;41~")}, {0, K(")")}), // 0x29 )
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("*")}, {CTRL_|MOK1, M("\x1b[27;%d;42~")}, {MOK2, M("\x1b[27;%d;42~")}, {0, K("*")}), // 0x2a *
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("+")}, {CTRL_|MOK1, M("\x1b[27;%d;43~")}, {MOK2, M("\x1b[27;%d;43~")}, {0, K("+")}), // 0x2b +
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K(",")}, {CTRL_|MOK1, M("\x1b[27;%d;44~")}, {MOK2, M("\x1b[27;%d;44~")}, {0, K(",")}), // 0x2c ,
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("-")}, {CTRL_|MOK1, M("\x1b[27;%d;45~")}, {MOK2, M("\x1b[27;%d;45~")}, {0, K("-")}), // 0x2d -
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K(".")}, {CTRL_|MOK1, M("\x1b[27;%d;46~")}, {MOK2, M("\x1b[27;%d;46~")}, {0, K(".")}), // 0x2e .
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("/")}, {CTRL_|ALT|MOK1, M("\x1b[27;%d;47~")}, {MOK2, M("\x1b[27;%d;47~")}, {CTRL_, K("\x1f")}, {0, K("/")}), // 0x2f /

    X({ALTIFY | MOK1, M("\x1b[27;%d;48~")}, {0, K("0")}), // 0x30 0
    X({ALTIFY | MOK1, M("\x1b[27;%d;49~")}, {0, K("1")}), // 0x31 1
    X({ALTIFY | MOK2, M("\x1b[27;%d;50~")}, {CTRL_|NOALT|NOMETA, K("\x00")}, {MOK1, M("\x1b[27;%d;50~")}, {CTRL_, K("\x00")}, {0, K("2")}), // 0x32 2
    X({ALTIFY | MOK2, M("\x1b[27;%d;51~")}, {CTRL_|NOALT|NOMETA, K("\x1b")}, {MOK1, M("\x1b[27;%d;51~")}, {CTRL_, K("\x1b")}, {0, K("3")}), // 0x33 3
    X({ALTIFY | MOK2, M("\x1b[27;%d;52~")}, {CTRL_|NOALT|NOMETA, K("\x1c")}, {MOK1, M("\x1b[27;%d;52~")}, {CTRL_, K("\x1c")}, {0, K("4")}), // 0x34 4
    X({ALTIFY | MOK2, M("\x1b[27;%d;53~")}, {CTRL_|NOALT|NOMETA, K("\x1d")}, {MOK1, M("\x1b[27;%d;53~")}, {CTRL_, K("\x1d")}, {0, K("5")}), // 0x35 5
    X({ALTIFY | MOK2, M("\x1b[27;%d;54~")}, {CTRL_|NOALT|NOMETA, K("\x1e")}, {MOK1, M("\x1b[27;%d;54~")}, {CTRL_, K("\x1e")}, {0, K("6")}), // 0x36 6
    X({ALTIFY | MOK2, M("\x1b[27;%d;55~")}, {CTRL_|NOALT|NOMETA, K("\x1f")}, {MOK1, M("\x1b[27;%d;55~")}, {CTRL_, K("\x1f")}, {0, K("7")}), // 0x37 7
    X({ALTIFY | MOK2, M("\x1b[27;%d;56~")}, {CTRL_|NOALT|NOMETA, K("\x7f")}, {MOK1, M("\x1b[27;%d;56~")}, {CTRL_, K("\x7f")}, {0, K("8")}), // 0x38 8
    X({ALTIFY | MOK1, M("\x1b[27;%d;57~")}, {0, K("9")}), // 0x39 9
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K(":")}, {CTRL_|MOK1, M("\x1b[27;%d;58~")}, {MOK2, M("\x1b[27;%d;58~")}, {0, K(":")}), // 0x3a :
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K(";")}, {CTRL_|MOK1, M("\x1b[27;%d;59~")}, {MOK2, M("\x1b[27;%d;59~")}, {0, K(";")}), // 0x3b ;
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("<")}, {CTRL_|MOK1, M("\x1b[27;%d;60~")}, {MOK2, M("\x1b[27;%d;60~")}, {0, K("<")}), // 0x3c <
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("=")}, {CTRL_|MOK1, M("\x1b[27;%d;61~")}, {MOK2, M("\x1b[27;%d;61~")}, {0, K("=")}), // 0x3d =
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K(">")}, {CTRL_|MOK1, M("\x1b[27;%d;62~")}, {MOK2, M("\x1b[27;%d;62~")}, {0, K(">")}), // 0x3e >
    X({ALTIFY | NOCTRL | NOALT | NOMETA, K("?")}, {CTRL_|MOK1, M("\x1b[27;%d;63~")}, {MOK2, M("\x1b[27;%d;63~")}, {CTRL_|NOALT, K("\x7f")}, {0, K("?")}), // 0x3f ?

    X({ALTIFY | MOK2, M("\x1b[27;%d;64~")}, {CTRL_, K("\x00")}, {0, K("@")}), // 0x40 @
    X({ALTIFY | MOK2, M("\x1b[27;%d;65~")}, {CTRL_, K("\x01")}, {0, K("A")}), // 0x41 A
    X({ALTIFY | MOK2, M("\x1b[27;%d;66~")}, {CTRL_, K("\x02")}, {0, K("B")}), // 0x42 B
    X({ALTIFY | MOK2, M("\x1b[27;%d;67~")}, {CTRL_, K("\x03")}, {0, K("C")}), // 0x43 C
    X({ALTIFY | MOK2, M("\x1b[27;%d;68~")}, {CTRL_, K("\x04")}, {0, K("D")}), // 0x44 D
    X({ALTIFY | MOK2, M("\x1b[27;%d;69~")}, {CTRL_, K("\x05")}, {0, K("E")}), // 0x45 E
    X({ALTIFY | MOK2, M("\x1b[27;%d;70~")}, {CTRL_, K("\x06")}, {0, K("F")}), // 0x46 F
    X({ALTIFY | MOK2, M("\x1b[27;%d;71~")}, {CTRL_, K("\x07")}, {0, K("G")}), // 0x47 G
    X({ALTIFY | MOK2, M("\x1b[27;%d;72~")}, {CTRL_, K("\x08")}, {0, K("H")}), // 0x48 H
    X({ALTIFY | MOK2, M("\x1b[27;%d;73~")}, {CTRL_, K("\x09")}, {0, K("I")}), // 0x49 I
    X({ALTIFY | MOK2, M("\x1b[27;%d;74~")}, {CTRL_, K("\x0a")}, {0, K("J")}), // 0x4a J
    X({ALTIFY | MOK2, M("\x1b[27;%d;75~")}, {CTRL_, K("\x0b")}, {0, K("K")}), // 0x4b K
    X({ALTIFY | MOK2, M("\x1b[27;%d;76~")}, {CTRL_, K("\x0c")}, {0, K("L")}), // 0x4c L
    X({ALTIFY | MOK2, M("\x1b[27;%d;77~")}, {CTRL_, K("\x0d")}, {0, K("M")}), // 0x4d M
    X({ALTIFY | MOK2, M("\x1b[27;%d;78~")}, {CTRL_, K("\x0e")}, {0, K("N")}), // 0x4e N
    X({ALTIFY | MOK2, M("\x1b[27;%d;79~")}, {CTRL_, K("\x0f")}, {0, K("O")}), // 0x4f O

    X({ALTIFY | MOK2, M("\x1b[27;%d;80~")}, {CTRL_, K("\x10")}, {0, K("P")}), // 0x50 P
    X({ALTIFY | MOK2, M("\x1b[27;%d;81~")}, {CTRL_, K("\x11")}, {0, K("Q")}), // 0x51 Q
    X({ALTIFY | MOK2, M("\x1b[27;%d;82~")}, {CTRL_, K("\x12")}, {0, K("R")}), // 0x52 R
    X({ALTIFY | MOK2, M("\x1b[27;%d;83~")}, {CTRL_, K("\x13")}, {0, K("S")}), // 0x53 S
    X({ALTIFY | MOK2, M("\x1b[27;%d;84~")}, {CTRL_, K("\x14")}, {0, K("T")}), // 0x54 T
    X({ALTIFY | MOK2, M("\x1b[27;%d;85~")}, {CTRL_, K("\x15")}, {0, K("U")}), // 0x55 U
    X({ALTIFY | MOK2, M("\x1b[27;%d;86~")}, {CTRL_, K("\x16")}, {0, K("V")}), // 0x56 V
    X({ALTIFY | MOK2, M("\x1b[27;%d;87~")}, {CTRL_, K("\x17")}, {0, K("W")}), // 0x57 W
    X({ALTIFY | MOK2, M("\x1b[27;%d;88~")}, {CTRL_, K("\x18")}, {0, K("X")}), // 0x58 X
    X({ALTIFY | MOK2, M("\x1b[27;%d;89~")}, {CTRL_, K("\x19")}, {0, K("Y")}), // 0x59 Y
    X({ALTIFY | MOK2, M("\x1b[27;%d;90~")}, {CTRL_, K("\x1a")}, {0, K("Z")}), // 0x5a Z
    X({ALTIFY | MOK2, M("\x1b[27;%d;91~")}, {CTRL_, K("\x1b")}, {0, K("[")}), // 0x5b [
    X({ALTIFY | MOK2, M("\x1b[27;%d;92~")}, {CTRL_, K("\x1c")}, {0, K("\\")}), /* 0x5c \ */
    X({ALTIFY | MOK2, M("\x1b[27;%d;93~")}, {CTRL_, K("\x1d")}, {0, K("]")}), // 0x5d ]
    X({ALTIFY | MOK2, M("\x1b[27;%d;94~")}, {CTRL_, K("\x1e")}, {0, K("^")}), // 0x5e ^
    X({ALTIFY | MOK2, M("\x1b[27;%d;95~")}, {CTRL_, K("\x1f")}, {0, K("_")}), // 0x5f _

    X({ALTIFY | MOK2, M("\x1b[27;%d;96~")}, {CTRL_, K("\x00")}, {0, K("`")}), // 0x60 `
    X({ALTIFY | MOK2, M("\x1b[27;%d;97~")}, {CTRL_, K("\x01")}, {0, K("a")}), // 0x61 a
    X({ALTIFY | MOK2, M("\x1b[27;%d;98~")}, {CTRL_, K("\x02")}, {0, K("b")}), // 0x62 b
    X({ALTIFY | MOK2, M("\x1b[27;%d;99~")}, {CTRL_, K("\x03")}, {0, K("c")}), // 0x63 c
    X({ALTIFY | MOK2, M("\x1b[27;%d;100~")}, {CTRL_, K("\x04")}, {0, K("d")}), // 0x64 d
    X({ALTIFY | MOK2, M("\x1b[27;%d;101~")}, {CTRL_, K("\x05")}, {0, K("e")}), // 0x65 e
    X({ALTIFY | MOK2, M("\x1b[27;%d;102~")}, {CTRL_, K("\x06")}, {0, K("f")}), // 0x66 f
    X({ALTIFY | MOK2, M("\x1b[27;%d;103~")}, {CTRL_, K("\x07")}, {0, K("g")}), // 0x67 g
    X({ALTIFY | MOK2, M("\x1b[27;%d;104~")}, {CTRL_, K("\x08")}, {0, K("h")}), // 0x68 h
    X({ALTIFY | MOK2, M("\x1b[27;%d;105~")}, {CTRL_, K("\x09")}, {0, K("i")}), // 0x69 i
    X({ALTIFY | MOK2, M("\x1b[27;%d;106~")}, {CTRL_, K("\x0a")}, {0, K("j")}), // 0x6a j
    X({ALTIFY | MOK2, M("\x1b[27;%d;107~")}, {CTRL_, K("\x0b")}, {0, K("k")}), // 0x6b k
    X({ALTIFY | MOK2, M("\x1b[27;%d;108~")}, {CTRL_, K("\x0c")}, {0, K("l")}), // 0x6c l
    X({ALTIFY | MOK2, M("\x1b[27;%d;109~")}, {CTRL_, K("\x0d")}, {0, K("m")}), // 0x6d m
    X({ALTIFY | MOK2, M("\x1b[27;%d;110~")}, {CTRL_, K("\x0e")}, {0, K("n")}), // 0x6e n
    X({ALTIFY | MOK2, M("\x1b[27;%d;111~")}, {CTRL_, K("\x0f")}, {0, K("o")}), // 0x6f o

    X({ALTIFY | MOK2, M("\x1b[27;%d;112~")}, {CTRL_, K("\x10")}, {0, K("p")}), // 0x70 p
    X({ALTIFY | MOK2, M("\x1b[27;%d;113~")}, {CTRL_, K("\x11")}, {0, K("q")}), // 0x71 q
    X({ALTIFY | MOK2, M("\x1b[27;%d;114~")}, {CTRL_, K("\x12")}, {0, K("r")}), // 0x72 r
    X({ALTIFY | MOK2, M("\x1b[27;%d;115~")}, {CTRL_, K("\x13")}, {0, K("s")}), // 0x73 s
    X({ALTIFY | MOK2, M("\x1b[27;%d;116~")}, {CTRL_, K("\x14")}, {0, K("t")}), // 0x74 t
    X({ALTIFY | MOK2, M("\x1b[27;%d;117~")}, {CTRL_, K("\x15")}, {0, K("u")}), // 0x75 u
    X({ALTIFY | MOK2, M("\x1b[27;%d;118~")}, {CTRL_, K("\x16")}, {0, K("v")}), // 0x76 v
    X({ALTIFY | MOK2, M("\x1b[27;%d;119~")}, {CTRL_, K("\x17")}, {0, K("w")}), // 0x77 w
    X({ALTIFY | MOK2, M("\x1b[27;%d;120~")}, {CTRL_, K("\x18")}, {0, K("x")}), // 0x78 x
    X({ALTIFY | MOK2, M("\x1b[27;%d;121~")}, {CTRL_, K("\x19")}, {0, K("y")}), // 0x79 y
    X({ALTIFY | MOK2, M("\x1b[27;%d;122~")}, {CTRL_, K("\x1a")}, {0, K("z")}), // 0x7a z
    X({ALTIFY | MOK2, M("\x1b[27;%d;123~")}, {CTRL_, K("\x1b")}, {0, K("{")}), // 0x7b {
    X({ALTIFY | MOK2, M("\x1b[27;%d;124~")}, {CTRL_, K("\x1c")}, {0, K("|")}), // 0x7c |
    X({ALTIFY | MOK2, M("\x1b[27;%d;125~")}, {CTRL_, K("\x1d")}, {0, K("}")}), // 0x7d }
    X({ALTIFY | MOK2, M("\x1b[27;%d;126~")}, {CTRL_, K("\x1e")}, {0, K("~")}), // 0x7e ~
    X({ALTIFY | MOK2, M("\x1b[27;%d;127~")}, {CTRL_, K("\x1f")}, {0, K("\x7f")}), // 0x7f del  // TODO: could not produce this

    // Non-ascii inputs:

    X({NM|NOCURS, K("\x1b[H")}, {NM, K("\x1bOH")}, {0, M("\x1b[1;%dH")}), // NAST_KEY_HOME
    X({NM|NOCURS, K("\x1b[F")}, {NM, K("\x1bOF")}, {0, M("\x1b[1;%dF")}), // NAST_KEY_END
    X({NM, K("\x1b[2~")}, {0, M("\x1b[2;%d~")}), // NAST_KEY_INSERT
    X({NM, K("\x1b[3~")}, {0, M("\x1b[3;%d~")}), // NAST_KEY_DELETE

    X({SHIFT, F(SHIFT_PGUP)}, {NM, K("\x1b[5~")}, {0, M("\x1b[5;%d~")}), // NAST_KEY_PGUP
    X({SHIFT, F(SHIFT_PGDN)}, {NM, K("\x1b[6~")}, {0, M("\x1b[6;%d~")}), // NAST_KEY_PGDN

    X({ALTIFY | MOK2, M("\x1b[27;%d;8~")}, {CTRL_, K("\x7f")}, {0, K("\b")}), // NAST_KEY_BKSP
    X({MOK1, M("\x1b[27;%d;13~")}, {NOALT, K("\r")}, {0, K("")}), // NAST_KEY_ENTER
    X({SHIFT, K("\x1b[Z")}, {ALT, K("\xc2\x89")}, {MOK1, M("\x1b[27;%d;9~")}, {0, K("\t")}), // NAST_KEY_TAB
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
