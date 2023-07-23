typedef struct {
    char *text;
    size_t len;
} simple_key_t;

typedef enum {
    KEY_ACTION_SIMPLE,
    KEY_ACTION_MODS,
    KEY_ACTION_SHIFT_PGUP,
    KEY_ACTION_SHIFT_PGDN,
    KEY_ACTION_SHIFT_INSERT,
} key_action_type_e;

typedef union {
    simple_key_t simple;
    char *mods;
    // remaining values need no arg
} key_action_u;

typedef struct {
    key_action_type_e type;
    key_action_u val;
} key_action_t;

typedef struct {
    unsigned int mask;
    key_action_t *action;
} key_map_t;

// 1 = match CTRL
// 2 = which CTRL matches (on or off)
#define MATCH_CTRL 1
// #define CTRL_MASK 2
#define NOCTRL 1
#define CTRL_ 3

// 4 = match SHIFT
// 8 = which SHIFT matches
#define MATCH_SHIFT 4
// #define SHIFT_MASK 8
#define NOSHIFT 4
#define SHIFT 12

// 16 = match ALT
// 32 = which ALT matches
#define MATCH_ALT 16
// #define ALT_MASK 32
#define NOALT 16
#define ALT 48

// 64 = match META
// 128 = which META matches
#define MATCH_META 64
// #define META_MASK 128
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

extern key_map_t *keymap[];
