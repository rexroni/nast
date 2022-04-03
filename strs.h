
// String splitter API:
//
// splitter_t spl;
// for(spl_init(&spl, buf, len, ':'); spl.ok; spl_next(&spl)){
//     printf("%zu: %.*s\n", spl.i, (int)spl.len, spl.buf);
// }
typedef struct {
    const char *_buf;
    size_t _len;
    char _c;
    size_t _end;
    // api
    bool ok;
    const char *buf;
    size_t len;
    size_t i;
} splitter_t;
void spl_init(splitter_t *spl, const char *buf, size_t len, const char c);
void spl_next(splitter_t *spl);

// dump a string, escaped as for a C string literal
size_t dumpstr(FILE *f, const char *buf, size_t len);

int unhex(const char *buf, size_t len, char *bin, size_t limit);
