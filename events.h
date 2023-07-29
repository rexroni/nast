// events that the backend provide to the terminal

typedef enum {
    MOUSE_EV_PRESS,
    MOUSE_EV_RELEASE,
    MOUSE_EV_MOTION,
    MOUSE_EV_SCROLL,
} mouse_ev_e;

typedef struct {
    mouse_ev_e type;
    unsigned int mods;
    // time in ms, for calculating double/triple clicks
    uint32_t ms;
    int n;
    int x;
    int y;
    // are x and y expressed as pixel coords? (or term coords?)
    bool pix_coords;
} mouse_ev_t;

//

typedef struct {
    int key;  // plain ascii or NAST_KEY_*, below
    unsigned int mods; // see *_MASK, below
} key_ev_t;

#define CTRL_MASK 2
#define SHIFT_MASK 8
#define ALT_MASK 32
#define META_MASK 128

#define NAST_KEY_HOME 0x80
#define NAST_KEY_END 0x81
#define NAST_KEY_INSERT 0x82
#define NAST_KEY_DELETE 0x83
#define NAST_KEY_PGUP 0x84
#define NAST_KEY_PGDN 0x85
#define NAST_KEY_BKSP 0x86
#define NAST_KEY_ENTER 0x87
#define NAST_KEY_TAB 0x88
#define NAST_KEY_ESC 0x89

#define NAST_KEY_UP 0x8a
#define NAST_KEY_DN 0x8b
#define NAST_KEY_RIGHT 0x8c
#define NAST_KEY_LEFT 0x8d

#define NAST_KEY_KP0 0x8e
#define NAST_KEY_KP1 0x8f
#define NAST_KEY_KP2 0x90
#define NAST_KEY_KP3 0x91
#define NAST_KEY_KP4 0x92
#define NAST_KEY_KP5 0x93
#define NAST_KEY_KP6 0x94
#define NAST_KEY_KP7 0x95
#define NAST_KEY_KP8 0x96
#define NAST_KEY_KP9 0x97

#define NAST_KEY_KPASTERISK 0x98
#define NAST_KEY_KPMINUS 0x99
#define NAST_KEY_KPPLUS 0x9a
#define NAST_KEY_KPCOMMA 0x9b
#define NAST_KEY_KPSLASH 0x9c
#define NAST_KEY_KPENTER 0x9d

// "u"nlocked keypad numbers (without numlock set)
#define NAST_KEY_KP0u 0x9e
#define NAST_KEY_KP1u 0x9f
#define NAST_KEY_KP2u 0xa0
#define NAST_KEY_KP3u 0xa1
#define NAST_KEY_KP4u 0xa2
#define NAST_KEY_KP5u 0xa3
#define NAST_KEY_KP6u 0xa4
#define NAST_KEY_KP7u 0xa5
#define NAST_KEY_KP8u 0xa6
#define NAST_KEY_KP9u 0xa7

#define NAST_KEY_KPCOMMAu 0xa8

#define NAST_KEY_F1 0xa9
#define NAST_KEY_F2 0xaa
#define NAST_KEY_F3 0xab
#define NAST_KEY_F4 0xac
#define NAST_KEY_F5 0xad
#define NAST_KEY_F6 0xae
#define NAST_KEY_F7 0xaf
#define NAST_KEY_F8 0xb0
#define NAST_KEY_F9 0xb1
#define NAST_KEY_F10 0xb2
#define NAST_KEY_F11 0xb3
#define NAST_KEY_F12 0xb4
#define NAST_KEY_F13 0xb5
#define NAST_KEY_F14 0xb6
#define NAST_KEY_F15 0xb7
#define NAST_KEY_F16 0xb8
#define NAST_KEY_F17 0xb9
#define NAST_KEY_F18 0xba
#define NAST_KEY_F19 0xbb
#define NAST_KEY_F20 0xbc
#define NAST_KEY_F21 0xbd
#define NAST_KEY_F22 0xbe
#define NAST_KEY_F23 0xbf
#define NAST_KEY_F24 0xc0
#define NAST_KEY_F25 0xc1
#define NAST_KEY_F26 0xc2
#define NAST_KEY_F27 0xc3
#define NAST_KEY_F28 0xc4
#define NAST_KEY_F29 0xc5
#define NAST_KEY_F30 0xc6
#define NAST_KEY_F31 0xc7
#define NAST_KEY_F32 0xc8
#define NAST_KEY_F33 0xc9
#define NAST_KEY_F34 0xca
#define NAST_KEY_F35 0xcb
#define NAST_KEY_F36 0xcc
#define NAST_KEY_F37 0xcd
#define NAST_KEY_F38 0xce
#define NAST_KEY_F39 0xcf
#define NAST_KEY_F40 0xd0
#define NAST_KEY_F41 0xd1
#define NAST_KEY_F42 0xd2
#define NAST_KEY_F43 0xd3
#define NAST_KEY_F44 0xd4
#define NAST_KEY_F45 0xd5
#define NAST_KEY_F46 0xd6
#define NAST_KEY_F47 0xd7
#define NAST_KEY_F48 0xd8
#define NAST_KEY_F49 0xd9
#define NAST_KEY_F50 0xda
#define NAST_KEY_F51 0xdb
#define NAST_KEY_F52 0xdc
#define NAST_KEY_F53 0xdd
#define NAST_KEY_F54 0xde
#define NAST_KEY_F55 0xdf
#define NAST_KEY_F56 0xe0
#define NAST_KEY_F57 0xe1
#define NAST_KEY_F58 0xe2
#define NAST_KEY_F59 0xe3
#define NAST_KEY_F60 0xe4
#define NAST_KEY_F61 0xe5
#define NAST_KEY_F62 0xe6
#define NAST_KEY_F63 0xe7
