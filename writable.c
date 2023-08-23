#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "writable.h"

static void writable_add_link(struct writable *w, struct writable_heap *link){
    if(w->list == NULL){
        w->list = link;
        link->next = link;
        link->prev = link;
    }else{
        link->next = w->list;
        link->prev = w->list->prev;
        link->next->prev = link;
        link->prev->next = link;
    }
}

// add to the ring buffer (must already fit in ring buffer)
static void writable_add_ring(struct writable *w, const char *s, size_t n){
    // check maximum size of copy
    size_t cp_lim = sizeof(w->ring) - w->end;
    if(n < cp_lim){
        memcpy(&w->ring[w->end], s, n);
    }else{
        // first copy, to physical end of ring buffer
        memcpy(&w->ring[w->end], s, cp_lim);
        // second copy, from physical start of ring buffer
        memcpy(w->ring, &s[cp_lim], n - cp_lim);
    }
    w->end = (w->end + n) % sizeof(w->ring);
}

// add to the writable heap linked list, any length is allowed
static void writable_add_heap(struct writable *w, const char *s, size_t n){
    struct writable_heap *link = malloc(sizeof(*link) + n);
    if(!link){
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    link->written = 0;
    link->len = n;
    link->next = link;
    link->prev = link;
    memcpy(link->data, s, n);
    writable_add_link(w, link);
}

// get the length of the ring buffer (just the ring, not the whole thing)
static inline size_t writable_ring_len(struct writable *w){
    return ((w->start > w->end)*sizeof(w->ring) + w->end) - w->start;
}

// all bytes are returnable until the next call; this is for "the next call"
static void drop_returnable(struct writable *w){
    if(w->returnable == RETURNABLE_HEAP){
        struct writable_heap *link = w->list;
        // are we done with this link?
        if(link->written == link->len){
            // was this the last link?
            if(link->next == link){
                w->list = NULL;
            }else{
                w->list = link->next;
                link->next->prev = link->prev;
                link->prev->next = link->next;
            }
            free(link);
        }
    }

    w->returnable = RETURNABLE_NONE;
}

// add some bytes to what's writable, and sort out where they need to go
void writable_add_bytes(struct writable *w, const char *s, size_t n){
    drop_returnable(w);

    // if there is already a heap, we just pile onto it
    if(w->list != NULL){
        writable_add_heap(w, s, n);
    }else{
        // see how much of the buffer fits in the ring
        size_t ringable = sizeof(w->ring) - writable_ring_len(w) - 1;
        if(!ringable){
            // none of it fits
            writable_add_heap(w, s, n);
        }else if(n > ringable){
            // some of it fits
            writable_add_ring(w, s, ringable);
            writable_add_heap(w, &s[ringable], n - ringable);
        }else{
            // all of it fits
            writable_add_ring(w, s, n);
        }
    }
}

// get a byte to write, there must be a writable character
char writable_get_char(struct writable *w){
    drop_returnable(w);

    char out;
    if(writable_ring_len(w)){
        out = w->ring[w->start];
        w->start = (w->start + 1) % sizeof(w->ring);
        w->returnable = RETURNABLE_RING;
    }else{
        struct writable_heap *link = w->list;
        out = link->data[link->written++];
        w->returnable = RETURNABLE_HEAP;
    }
    return out;
}

bool writable_nonempty(struct writable *w){
    drop_returnable(w);

    return w->list || writable_ring_len(w);
}

const char *writable_get_string(struct writable *w, size_t *n){
    drop_returnable(w);

    char *out = NULL;
    size_t len = 0;

    if(writable_ring_len(w)){
        out = &w->ring[w->start];
        if(w->start > w->end){
            // give out the string to the physical end of the ringbuffer
            len = sizeof(w->ring) - w->start;
        }else{
            // give out the whole string
            len = w->end - w->start;
        }
        w->start = (w->start + len) % sizeof(w->ring);
        w->returnable = RETURNABLE_RING;
    }else if(w->list){
        struct writable_heap *link = w->list;
        out = &link->data[link->written];
        len = link->len - link->written;
        // that's all there is
        link->written = link->len;
        w->returnable = RETURNABLE_HEAP;
    }

    *n = len;
    return out;
}


void writable_return_bytes(struct writable *w, size_t unneeded){
    switch(w->returnable){
        case RETURNABLE_NONE:
            break;
        case RETURNABLE_RING:
            w->start = (w->start - unneeded) % sizeof(w->ring);
            break;
        case RETURNABLE_HEAP:
            w->list->written -= unneeded;
            break;
    }

    w->returnable = RETURNABLE_NONE;
}
