#include <stdbool.h>

struct writable_heap;

struct writable_heap {
    struct writable_heap *next;
    struct writable_heap *prev;
    // how much is there to write?
    size_t len;
    // how much has already been written?
    size_t written;
    char data[0];
};

struct writable {
    char ring[16384];
    size_t start;
    size_t end;
    // list points to first link in list (or NULL)
    struct writable_heap *list;
    enum {
        RETURNABLE_NONE,
        RETURNABLE_RING,
        RETURNABLE_HEAP,
    } returnable;
};

// add some bytes to what's writable, and sort out where they need to go
void writable_add_bytes(struct writable *w, const char *s, size_t n);

bool writable_nonempty(struct writable *w);

// get a byte to write, there must be a writable character
char writable_get_char(struct writable *w);

// returns NULL when writable is empty; char* valid until next writable call
const char *writable_get_string(struct writable *w, size_t *n);

// only valid right after writable_get_*()
// after writable_get_char(), ensure that unneeded == 1
// after writable_get_string(), ensure that 0 < unneeded <= n
void writable_return_bytes(struct writable *w, size_t unneeded);
