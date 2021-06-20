typedef struct {
    char *key;
    int (*action)(void *globals, GdkEventKey *event_key);
} key_action_t; // "key action"

#define K(str) &(key_action_t){.key=str}
#define A(fn) &(key_action_t){.action=fn}

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
};

#undef K
#undef A
