#include <stdio.h>
#include <stdbool.h>
#include <strings.h>

#include "strs.h"

void spl_next(splitter_t *spl){
    if(!spl->ok) return;
    if(spl->_end == spl->_len){
        // no more splits
        spl->ok = false;
        spl->len = 0;
        spl->i++;
        return;
    }
    // next buf starts just after the next sep
    size_t start = spl->_end + 1;
    for(size_t i = start; i < spl->_len; i++){
        if(spl->_buf[i] == spl->_c){
            // match!
            spl->_end = i;
            spl->len = i - start;
            spl->buf = &spl->_buf[start];
            spl->i++;
            return;
        }
    }
    // no more separators, emit the final split
    spl->_end = spl->_len;
    spl->len = spl->_end - start;
    spl->buf = &spl->_buf[start];
    spl->i++;
}

void spl_init(splitter_t *spl, const char *buf, size_t len, const char c){
    *spl = (splitter_t){
        ._buf = buf,
        ._len = len,
        ._c = c,
        ._end = (size_t)-1,
        .ok = true,
        .i = (size_t)-1,
    };
    spl_next(spl);
}

size_t dumpstr(FILE *f, const char *buf, size_t len){
    size_t count = 0;
    for(size_t i = 0; i < len; i++){
        char c = buf[i];
        if(c == '\\'){
            fprintf(f, "\\\\");
            count += 2;
        }else if(c >= ' ' && c <= '~'){
            fputc(c, f);
            count += 1;
            continue;
        }else{
            fprintf(f, "\\x%.2x", c);
            count += 4;
        }
    }
    return count;
}

int unhex(const char *buf, size_t len, char *bin, size_t limit){
    if(len%2 != 0){
        fprintf(stderr, "bad hex (len%%2 != 0): ");
        return 1;
    }
    if(len / 2 >= limit){
        fprintf(stderr, "bad hex (too long for buffer): ");
        return 1;
    }
    for(size_t i = 0; i < len; i ++){
        char nibble;
        switch(buf[i]){
            case '0': nibble = 0x00; break;
            case '1': nibble = 0x01; break;
            case '2': nibble = 0x02; break;
            case '3': nibble = 0x03; break;
            case '4': nibble = 0x04; break;
            case '5': nibble = 0x05; break;
            case '6': nibble = 0x06; break;
            case '7': nibble = 0x07; break;
            case '8': nibble = 0x08; break;
            case '9': nibble = 0x09; break;
            case 'A':
            case 'a': nibble = 0x0a; break;
            case 'B':
            case 'b': nibble = 0x0b; break;
            case 'C':
            case 'c': nibble = 0x0c; break;
            case 'D':
            case 'd': nibble = 0x0d; break;
            case 'E':
            case 'e': nibble = 0x0d; break;
            case 'F':
            case 'f': nibble = 0x0e; break;
            default:
                fprintf(stderr, "bad hex (invalid characters): ");
                return 1;
        }
        if(i%2 == 0){
            bin[i>>1] = nibble << 4;
        }else{
            bin[i>>1] |= nibble;
        }
    }
    return 0;
}
