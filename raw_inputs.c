#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>


#define APPCURSOR_ON "\x1b[?1h"
#define APPCURSOR_OFF "\x1b[?1l"
#define APPKEYPAD_ON "\x1b="
#define APPKEYPAD_OFF "\x1b>"

void show_characters_from_tty(int appcursor, int appkeypad){
    struct termios tios;
    // store terminal settings
    tcgetattr(0, &tios);
    struct termios raw_tios = tios;
    cfmakeraw(&raw_tios);
    tcsetattr(0, TCSANOW, &raw_tios);

    if(appcursor){
        printf("enabling appcursor mode\r\n");
        printf(APPCURSOR_ON);
        fflush(stdout);
    }
    if(appkeypad){
        printf("enabling appkeypad mode\r\n");
        printf(APPKEYPAD_ON);
        fflush(stdout);
    }

    char byte;
    while(read(0, &byte, 1) == 1){
        if(byte == 'q') break;
        int b = byte;
        if(b < 0) b += 256;
        printf("dec = %d, hex = %x, char = %c\r\n", b, b, (char)b);
    }

    if(appcursor){
        printf("disabling appcursor mode\r\n");
        printf(APPCURSOR_OFF);
        fflush(stdout);
    }
    if(appkeypad){
        printf("disabling appkeypad mode\r\n");
        printf(APPKEYPAD_OFF);
        fflush(stdout);
    }

    // reset terminal settings
    tcsetattr(0, TCSANOW, &tios);
}

int wait_for_input(int fd, suseconds_t usec, int *ok){
    *ok = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval tv = { .tv_usec = usec };

    int ret = select(1, &rfds, NULL, NULL, &tv);
    if(ret == -1){
        perror("select()");
        return 1;
    }

    *ok = !!ret;

    return 0;
}

// reads a string of input, returning after a short interval without input
int read_string(char *buf, size_t len, size_t *nread_out){
    char byte;
    size_t nread = 0;
    *nread_out = nread;
    while(nread < len - 1){
        if(nread > 0){
            int ok;
            if(wait_for_input(0, 10001, &ok)){
                return 1;
            }
            if(!ok){
                break;
            }
        }

        if(read(0, &byte, 1) != 1){
            perror("read");
            return 1;
        }
        buf[nread++] = byte;
    }
    buf[nread] = '\0';
    *nread_out = nread;
    return 0;
}

char hex(int v){
    switch(v & 0xf){
        case 0: return '0';
        case 1: return '1';
        case 2: return '2';
        case 3: return '3';
        case 4: return '4';
        case 5: return '5';
        case 6: return '6';
        case 7: return '7';
        case 8: return '8';
        case 9: return '9';
        case 10: return 'a';
        case 11: return 'b';
        case 12: return 'c';
        case 13: return 'd';
        case 14: return 'e';
        case 15: return 'f';
    }
    return 'x';
}

// log chars as you would type them into a string literal
int fstrlit(FILE *f, char *s, size_t slen){
#   define CHR(x) buf[len++] = x
    char buf[128];
    size_t len = 0, maxlen = sizeof(buf);
    char c;
    size_t i;
    for(i = 0, c = s[i]; len < maxlen && i < slen; c = s[++i]){
        if     (c == '\0'){ CHR('\\'); CHR('0'); }
        else if(c == '\r'){ CHR('\\'); CHR('r'); }
        else if(c == '\n'){ CHR('\\'); CHR('n'); }
        else if(c == '\t'){ CHR('\\'); CHR('t'); }
        else if(c == '\b'){ CHR('\\'); CHR('b'); }
        else if(c < 32 || c > 126){
            // e.g. "\x1b"
            CHR('\\');
            CHR('x');
            CHR(hex(c >> 4));
            CHR(hex(c));
        }
        else { CHR(c); }
    }
    return fprintf(f, "%.*s", (int)len, buf);
#   undef CHR
}

int run_logger(char *logfile){
    int retval = 0;

    struct termios tios;
    // store terminal settings
    tcgetattr(0, &tios);
    struct termios raw_tios = tios;

    FILE *f = fopen(logfile, "a");
    if(!f){
        perror(logfile);
        retval = 1;
        goto cu;
    }

    while(1){
        // reset terminal settings
        tcsetattr(0, TCSANOW, &tios);

        printf("key name: ");
        fflush(stdout);
        char name[128];
        char *cret = fgets(name, sizeof(name), stdin);
        if(!cret){
            perror("fgets");
            retval = 1;
            goto cu;
        }
        if(strcmp(name, "\n") == 0) break;
        size_t namelen = strlen(name);
        if(namelen > 0) name[--namelen] = '\0';

        // raw terminal
        cfmakeraw(&raw_tios);
        tcsetattr(0, TCSANOW, &raw_tios);

        char norm[16];
        size_t norm_len;
        char cur[16];
        size_t cur_len;
        char key[16];
        size_t key_len;
        char curkey[16];
        size_t curkey_len;

        printf("press it in normal mode\r\n");
        if(read_string(norm, sizeof(norm), &norm_len)){
            retval = 1;
            goto cu;
        }

        printf(APPCURSOR_ON "press it with appcursor mode\r\n");
        fflush(stdout);
        if(read_string(cur, sizeof(cur), &cur_len)){
            retval = 1;
            goto cu;
        }

        printf(APPCURSOR_OFF APPKEYPAD_ON "press it with appkeypad mode\r\n");
        fflush(stdout);
        if(read_string(key, sizeof(key), &key_len)){
            retval = 1;
            goto cu;
        }

        printf(APPCURSOR_ON "press it with both modes\r\n");
        fflush(stdout);
        if(read_string(curkey, sizeof(curkey), &curkey_len)){
            retval = 1;
            goto cu;
        }

        printf(APPCURSOR_OFF APPKEYPAD_OFF);
        fflush(stdout);

        fprintf(f, "%s,", name);
        fstrlit(f, norm, norm_len); fprintf(f, ",");
        fstrlit(f, cur, cur_len); fprintf(f, ",");
        fstrlit(f, key, key_len); fprintf(f, ",");
        fstrlit(f, curkey, curkey_len); fprintf(f, "\n");

        fflush(f);
        fsync(fileno(f));
    }


cu:
    // reset terminal settings
    tcsetattr(0, TCSANOW, &tios);

    if(f){
        int ret = fclose(f);
        if(ret != 0){
            perror("fclose");
            retval = 1;
        }
    }

    return retval;
}

int main(int argc, char **argv){
    int appcursor = 0;
    int appkeypad = 0;
    int logger = 0;
    char *logfile = NULL;
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "--appcursor") == 0){
            appcursor = 1;
            continue;
        }
        if(strcmp(argv[i], "--appkeypad") == 0){
            appkeypad = 1;
            continue;
        }
        if(strcmp(argv[i], "--logger") == 0){
            logger = 1;
            if(i + 1 == argc){
                printf("--logger requires a logfile argument!\n");
                return 1;
            }
            // eat the next argument
            logfile = argv[++i];
        }
    }

    if(logger && (appcursor || appkeypad)){
        printf("--logger conflicts with --appcursor and --appkeypad\n");
        return 1;
    }

    if(logger){
        return(run_logger(logfile));
    }

    printf("showing raw inputs (press q to quit)...\n");
    show_characters_from_tty(appcursor, appkeypad);
    return 0;
}
