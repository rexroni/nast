typedef struct {
    char *key;
    int (*action)(void *globals, GdkEventKey *event_key);
} key_action_t; // "key action"

#define K(str) &(key_action_t){.key=str}
#define A(fn) &(key_action_t){.action=fn}

key_action_t *keymap[][2] = {
//   ctrl               ctrl+shift
    {NULL,              NULL}, // 0x00
    {NULL,              NULL}, // 0x01
    {NULL,              NULL}, // 0x02
    {NULL,              NULL}, // 0x03
    {NULL,              NULL}, // 0x04
    {NULL,              NULL}, // 0x05
    {NULL,              NULL}, // 0x06
    {NULL,              NULL}, // 0x07
    {NULL,              NULL}, // 0x08
    {NULL,              NULL}, // 0x09
    {NULL,              NULL}, // 0x0a
    {NULL,              NULL}, // 0x0b
    {NULL,              NULL}, // 0x0c
    {NULL,              NULL}, // 0x0d
    {NULL,              NULL}, // 0x0e
    {NULL,              NULL}, // 0x0f

    {NULL,              NULL}, // 0x10
    {NULL,              NULL}, // 0x11
    {NULL,              NULL}, // 0x12
    {NULL,              NULL}, // 0x13
    {NULL,              NULL}, // 0x14
    {NULL,              NULL}, // 0x15
    {NULL,              NULL}, // 0x16
    {NULL,              NULL}, // 0x17
    {NULL,              NULL}, // 0x18
    {NULL,              NULL}, // 0x19
    {NULL,              NULL}, // 0x1a
    {NULL,              NULL}, // 0x1b
    {NULL,              NULL}, // 0x1c
    {NULL,              NULL}, // 0x1d
    {NULL,              NULL}, // 0x1e
    {NULL,              NULL}, // 0x1f

    {NULL,              NULL}, // 0x20
    {NULL,              NULL}, // 0x21
    {NULL,              NULL}, // 0x22
    {NULL,              NULL}, // 0x23
    {NULL,              NULL}, // 0x24
    {NULL,              NULL}, // 0x25
    {NULL,              NULL}, // 0x26
    {NULL,              NULL}, // 0x27
    {NULL,              NULL}, // 0x28
    {NULL,              NULL}, // 0x29
    {NULL,              NULL}, // 0x2a
    {NULL,              NULL}, // 0x2b
    {NULL,              NULL}, // 0x2c
    {NULL,              NULL}, // 0x2d
    {NULL,              NULL}, // 0x2e
    {K("\x1f"),         NULL}, // 0x2f /

    {NULL,              NULL}, // 0x30 0
    {NULL,              NULL}, // 0x31 1
    {K("\x00"),         NULL}, // 0x32 2
    {K("\x1b"),         NULL}, // 0x33 3
    {K("\x1c"),         NULL}, // 0x34 4
    {K("\x1d"),         NULL}, // 0x35 5
    {K("\x1e"),         NULL}, // 0x36 6
    {K("\x1f"),         NULL}, // 0x37 7
    {NULL,              NULL}, // 0x38 8
    {NULL,              NULL}, // 0x39 9
    {NULL,              NULL}, // 0x3a
    {NULL,              NULL}, // 0x3b
    {NULL,              NULL}, // 0x3c
    {NULL,              NULL}, // 0x3d
    {NULL,              NULL}, // 0x3e
    {NULL,              NULL}, // 0x3f ?

    {NULL,              NULL}, // 0x40
    {K("\x01"),         NULL}, // 0x41 A
    {K("\x02"),         NULL}, // 0x42 B
    {K("\x03"),         NULL}, // 0x43 C
    {K("\x04"),         NULL}, // 0x44 D
    {K("\x05"),         NULL}, // 0x45 E
    {K("\x06"),         NULL}, // 0x46 F
    {K("\x07"),         NULL}, // 0x47 G
    {K("\x08"),         NULL}, // 0x48 H
    {K("\x09"),         NULL}, // 0x49 I
    {K("\x0a"),         NULL}, // 0x4a J
    {K("\x0b"),         NULL}, // 0x4b K
    {K("\x0c"),         NULL}, // 0x4c L
    {K("\x0d"),         NULL}, // 0x4d M
    {K("\x0e"),         NULL}, // 0x4e N
    {K("\x0f"),         NULL}, // 0x4f O

    {K("\x10"),         NULL}, // 0x50 P
    {K("\x11"),         NULL}, // 0x51 Q
    {K("\x12"),         NULL}, // 0x52 R
    {K("\x13"),         NULL}, // 0x53 S
    {K("\x14"),         NULL}, // 0x54 T
    {K("\x15"),         NULL}, // 0x55 U
    {K("\x16"),         NULL}, // 0x56 V
    {K("\x17"),         NULL}, // 0x57 W
    {K("\x18"),         NULL}, // 0x58 X
    {K("\x19"),         NULL}, // 0x59 Y
    {K("\x1a"),         NULL}, // 0x5a Z
    {NULL,              NULL}, // 0x5b
    {NULL,              NULL}, // 0x5c
    {NULL,              NULL}, // 0x5d
    {NULL,              NULL}, // 0x5e
    {NULL,              NULL}, // 0x5f

    {NULL,              NULL}, // 0x60
    {K("\x01"),         NULL}, // 0x61 a
    {K("\x02"),         NULL}, // 0x62 b
    {K("\x03"),         NULL}, // 0x63 c
    {K("\x04"),         NULL}, // 0x64 d
    {K("\x05"),         NULL}, // 0x65 e
    {K("\x06"),         NULL}, // 0x66 f
    {K("\x07"),         NULL}, // 0x67 g
    {K("\x08"),         NULL}, // 0x68 h
    {K("\x09"),         NULL}, // 0x69 i
    {K("\x0a"),         NULL}, // 0x6a j
    {K("\x0b"),         NULL}, // 0x6b k
    {K("\x0c"),         NULL}, // 0x6c l
    {K("\x0d"),         NULL}, // 0x6d m
    {K("\x0e"),         NULL}, // 0x6e n
    {K("\x0f"),         NULL}, // 0x6f o

    {K("\x10"),         NULL}, // 0x70 p
    {K("\x11"),         NULL}, // 0x71 q
    {K("\x12"),         NULL}, // 0x72 r
    {K("\x13"),         NULL}, // 0x73 s
    {K("\x14"),         NULL}, // 0x74 t
    {K("\x15"),         NULL}, // 0x75 u
    {K("\x16"),         NULL}, // 0x76 v
    {K("\x17"),         NULL}, // 0x77 w
    {K("\x18"),         NULL}, // 0x78 x
    {K("\x19"),         NULL}, // 0x79 y
    {K("\x1a"),         NULL}, // 0x7a z
    {NULL,              NULL}, // 0x7b
    {NULL,              NULL}, // 0x7c
    {NULL,              NULL}, // 0x7d
    {NULL,              NULL}, // 0x7e
    {NULL,              NULL}, // 0x7f
};

#undef K
#undef A
