#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

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

// always reads 2 characters
int unhex(const char *buf, size_t len, char *bin, size_t limit){
    if(len%2 != 0){
        fprintf(stderr, "bad hex (len%%2 != 0): %.*s\r\n", (int)len, buf);
        return 1;
    }
    if(len / 2 > limit){
        fprintf(
            stderr, "bad hex (too long for buffer): %.*s\r\n", (int)len, buf
        );
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
                fprintf(
                    stderr,
                    "bad hex (invalid characters): %.*s\r\n",
                    (int)len, buf
                );
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

int main(void){
    // find a tty
    int ttyfd;
    if(isatty(0)) ttyfd = 0;
    else if(isatty(1)) ttyfd = 1;
    else if(isatty(2)) ttyfd = 2;
    else{
        fprintf(stderr, "unable to find a tty\n");
        return 1;
    }

    int retval = 0;
    struct termios tios;
    // store terminal settings
    tcgetattr(ttyfd, &tios);
    struct termios raw_tios = tios;
    cfmakeraw(&raw_tios);
    tcsetattr(ttyfd, TCSANOW, &raw_tios);

    // write \n to a file, or \r\n to tty (since it's in raw mode)
    char *ending = isatty(1) ? "\r\n" : "\n";

    // query all possible 2-character codes
    for(unsigned int i = 0; i < 256; i++){
        for(unsigned int j = 0; j < 256; j++){
            char buf[32];
            int len = sprintf(buf, "\x1bP+q%.2x%.2x\x1b\\", i, j);
            if(len < 1 || len > sizeof(buf)-1){
                retval = 1;
                goto done;
            }

            write(ttyfd, buf, len);
            int was_esc = 0;
            int fail = 0;
            char byte;
            char out[1024];
            size_t outlen = 0;
            while(1){
                if(outlen == sizeof(out)){
                    fprintf(stderr, "too long\r\n");
                    retval = 1;
                    goto done;
                }
                int nread = read(0, &byte, 1);
                if(nread != 1){
                    fprintf(stderr, "bad read\r\n");
                    retval = 1;
                    goto done;
                }
                if(byte == 3){
                    fprintf(stderr, "\r\ncanceled!\r\n");
                    retval = 1;
                    goto done;
                }
                if(outlen == 2) fail = (byte != '1');
                out[outlen++] = byte;
                if(was_esc && byte == '\\'){
                    // ST found, we're done
                    break;
                }
                was_esc = (byte == 0x1b);
            }

            size_t l = 0;

            // detect unexpected failure responses
            if(fail){
                if(outlen != 7 || strncmp(out, "\x1bP0+r\x1b\\", 7) != 0){
                    // unexpected failure string
                    printf("failed!: \"");
                    dumpstr(stdout, buf, len);
                    printf("\", (%c%c) got: \"", (char)i, (char)j);
                    dumpstr(stdout, out, outlen);
                    printf("\"\r\n");
                }
            }else{
                l += printf("    {\"%.2x%.2x\", \"", i, j);
                l += dumpstr(stdout, out, outlen);
                l += printf("\"},");
                while(l++ < 51) fputc(' ', stdout);
                printf("// %c%c -> \"", (char)i, (char)j);
                char bin[1024];
                // DCS(2) 1(1) +r(2) code(4) =(1) response ST(2)
                int ret = unhex(out+10, outlen-12, bin, sizeof(bin));
                if(ret){
                    retval = 1;
                    goto done;
                }
                l += dumpstr(stdout, bin, (outlen-12)>>1);
                printf("\"%s", ending);
            }
        }
    }

done:
    // reset terminal settings
    tcsetattr(ttyfd, TCSANOW, &tios);

    return retval;
}

