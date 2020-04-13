// steal access to static functions
#include "writable.c"

#define ASSERT(expr, ...) \
    do { \
        if(!(expr)){ \
            fprintf(stderr, __VA_ARGS__); \
            return 1; \
        } \
    } while(0)

#define PROP(expr) \
    do { \
        int ret = expr; \
        if(ret) return ret; \
    } while(0)


int test_writable(){
    struct writable w = {0};

    // test ring buffer

    ASSERT(writable_ring_len(&w) == 0, "wrong ring length at start");
    ASSERT(writable_nonempty(&w) == false, "nonempty at start");

    writable_add_ring(&w, "a", 1);
    writable_add_ring(&w, "bb", 2);
    writable_add_ring(&w, "ccc", 3);

    ASSERT(writable_ring_len(&w) == 6, "wrong ring length after add ring");
    ASSERT(writable_nonempty(&w) == true, "empty after add ring");

    size_t len;
    const char *string = writable_get_string(&w, &len);
    ASSERT(len == 6, "wrong string len after add ring");
    ASSERT(strncmp("abbccc", string, len) == 0, "wrong string after add ring");

    // put it back
    writable_return_bytes(&w, len);

    char chars[6] = {'a', 'b', 'b', 'c', 'c', 'c'};

    for(size_t i = 0; i < sizeof(chars) / sizeof(*chars); i++){
        char c = writable_get_char(&w);
        ASSERT(c == chars[i], "wrong char at index %zu\n", i);
    }

    // test heap overflow

    ASSERT(writable_ring_len(&w) == 0, "wrong ring length after ring test");
    ASSERT(writable_nonempty(&w) == false, "nonempty after ring test");

    writable_add_heap(&w, "a", 1);
    writable_add_heap(&w, "bb", 2);
    writable_add_heap(&w, "ccc", 3);

    // make sure we get the first heap string
    string = writable_get_string(&w, &len);
    ASSERT(len == 1, "wrong string len after add heap");
    ASSERT(strncmp("a", string, len) == 0, "wrong string after add heap");

    // now get the second heap string
    string = writable_get_string(&w, &len);
    ASSERT(len == 2, "wrong string len after add heap");
    ASSERT(strncmp("bb", string, len) == 0, "wrong string after add heap");

    // put half of it back
    writable_return_bytes(&w, 1);

    ASSERT(writable_ring_len(&w) == 0, "wrong ring length after add heap");
    ASSERT(writable_nonempty(&w) == true, "empty after add heap");

    for(size_t i = 2; i < sizeof(chars) / sizeof(*chars); i++){
        char c = writable_get_char(&w);
        ASSERT(c == chars[i], "wrong char at index %zu\n", i);
    }

    ASSERT(writable_ring_len(&w) == 0, "wrong ring length after heap test");
    ASSERT(writable_nonempty(&w) == false, "nonempty after heap test");

    // test again with the full writable_add_bytes() api

    char buf1[16382]; // 'a'
    char buf2[10];    // 'b'

    size_t len1 = sizeof(buf1) / sizeof(*buf1);
    size_t len2 = sizeof(buf2) / sizeof(*buf2);

    memset(buf1, 'a', len1);
    memset(buf2, 'b', len2);

    writable_add_bytes(&w, buf1, len1);

    ASSERT(writable_ring_len(&w) == len1, "wrong ring length after buf1 add");
    ASSERT(writable_nonempty(&w) == true, "empty after buf1 add");

    // this write should overflow the ring

    writable_add_bytes(&w, buf2, len2);

    ASSERT(writable_ring_len(&w) == 16383, "wrong ring length after buf2 add");
    ASSERT(writable_nonempty(&w) == true, "empty after buf2 add");

    for(size_t i = 0; i < len1; i++){
        char c = writable_get_char(&w);
        ASSERT(c == 'a', "wrong char at index %zu\n", i);
    }

    for(size_t i = 0; i < len2; i++){
        char c = writable_get_char(&w);
        ASSERT(c == 'b', "wrong char at index %zu\n", i);
    }

    ASSERT(writable_ring_len(&w) == 0, "wrong ring length after full test");
    ASSERT(writable_nonempty(&w) == false, "nonempty after full test");

    return 0;
}


int main(void){

    PROP( test_writable() );

    printf("PASS\n");
    return 0;
}
