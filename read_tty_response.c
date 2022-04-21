#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

#include <strs.h>

char next_in(char **in){
    (*in)++;
    if(**in == '\0'){
        fprintf(stderr, "invalid input, incomplete escape sequence\n");
        exit(2);
    }
    return **in;
}

size_t decode_str(char *arg){
    char *in = arg;
    char *out = arg;
    size_t len = 0;

    for(; *in != '\0'; in++, out++, len++){
        if(*in != '\\'){
            *out = *in;
            continue;
        }
        switch(next_in(&in)){
            case '\\': *out = '\\'; break;
            case 'n': *out = '\n'; break;
            case 'r': *out = '\r'; break;
            case 't': *out = '\t'; break;
            case 'b': *out = '\b'; break;
            case '0': *out = '\0'; break;
            case 'x': {
                char buf[2] = {next_in(&in), next_in(&in)};
                char bin;
                int ret = unhex(buf, 2, &bin, 1);
                if(ret){
                    fprintf(stderr, "invalid input, invalid hex\n");
                    exit(2);
                }
                *out = bin;
            } break;
            default:
                fprintf(stderr, "invalid input, invalid escape sequence\n");
                exit(2);
        }
    }
    return len;
}

int main(int argc, char **argv){
    if(argc < 2){
        fprintf(stderr, "usage: %s STR\n", argv[0]);
        return 1;
    }

    struct termios tios;
    // store terminal settings
    tcgetattr(0, &tios);
    struct termios raw_tios = tios;
    cfmakeraw(&raw_tios);
    tcsetattr(0, TCSANOW, &raw_tios);

    // write to stdin, which must be a tty anyway
    size_t decoded_len = decode_str(argv[1]);
    write(0, argv[1], decoded_len);

    char byte;
    while(read(0, &byte, 1) == 1){
        // enter to quit
        if(byte == '\r') break;
        int b = byte;
        if(b < 0) b += 256;
        printf("dec = %d, hex = %x, char = %c\r\n", b, b, (char)b);
    }

    // reset terminal settings
    tcsetattr(0, TCSANOW, &tios);

    return 0;
}
