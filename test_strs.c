#include <string.h>

#include "strs.c"


#define ASSERT(code) do{ \
    if(!(code)){ \
        fprintf(stderr, \
            "failed assertion: %s (%s::%s:%d)\n", \
            #code, __FILE__, __func__, __LINE__ \
        ); \
        return 1; \
    } \
} while(0)

// b is nul-terminated, but s is not
int streq(const char *s, size_t n, const char *b){
    if(strlen(b) != n) return 1;
    return strncmp(s, b, n) == 0;
}

int test_splitter(){
    splitter_t spl;
    const char *text = "a::b:cde:f::";
    spl_init(&spl, text, strlen(text), ':');
    ASSERT(spl.ok);
    ASSERT(spl.i == 0);
    ASSERT(streq(spl.buf, spl.len, "a"));

    spl_next(&spl);
    ASSERT(spl.ok);
    ASSERT(spl.i == 1);
    ASSERT(streq(spl.buf, spl.len, ""));

    spl_next(&spl);
    ASSERT(spl.ok);
    ASSERT(spl.i == 2);
    ASSERT(streq(spl.buf, spl.len, "b"));

    spl_next(&spl);
    ASSERT(spl.ok);
    ASSERT(spl.i == 3);
    ASSERT(streq(spl.buf, spl.len, "cde"));

    spl_next(&spl);
    ASSERT(spl.ok);
    ASSERT(spl.i == 4);
    ASSERT(streq(spl.buf, spl.len, "f"));

    spl_next(&spl);
    ASSERT(spl.ok);
    ASSERT(spl.i == 5);
    ASSERT(streq(spl.buf, spl.len, ""));

    spl_next(&spl);
    ASSERT(spl.ok);
    ASSERT(spl.i == 6);
    ASSERT(streq(spl.buf, spl.len, ""));

    // i increments as ok is cleared
    spl_next(&spl);
    ASSERT(!spl.ok);
    ASSERT(spl.i == 7);
    ASSERT(streq(spl.buf, spl.len, ""));

    // additional next calls are safe
    spl_next(&spl);
    ASSERT(!spl.ok);
    ASSERT(spl.i == 7);
    ASSERT(streq(spl.buf, spl.len, ""));

    // empty splits are also acceptable ("" -> [""])
    text = "";
    spl_init(&spl, text, strlen(text), ':');
    ASSERT(spl.ok);
    ASSERT(spl.i == 0);
    ASSERT(streq(spl.buf, spl.len, ""));

    spl_next(&spl);
    ASSERT(!spl.ok);
    ASSERT(spl.i == 1);
    ASSERT(streq(spl.buf, spl.len, ""));

    return 0;
}

int main(){
    int ret = 0;
    ret |= test_splitter();
    printf("%s\n", ret ? "FAIL" : "PASS");
    return ret;
}
