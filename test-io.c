/*
   IO test:

   main process
   │
   └ sudo main process
      |
      ├─ uintput keyboard
      │
     ... main process drops privileges
      │
      ├─ test.sock <--+
      │               |
      └─ terminal     |
          │           |
          └─ ./io ----+
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <fcntl.h>

#define APPCURSOR_ON "\x1b[?1h"
#define APPCURSOR_OFF "\x1b[?1l"
#define APPKEYPAD_ON "\x1b="
#define APPKEYPAD_OFF "\x1b>"

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
int fstrlit(FILE *f, const char *s, size_t slen){
    int ret = 0;
#   define CHR(x) if(ret != EOF) fputc((int)x, f)
    CHR('"');
    char c;
    size_t i;
    for(i = 0, c = s[i]; i < slen; c = s[++i]){
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
    CHR('"');
    if(ret == EOF){
        perror("fputc");
        return 1;
    }
    return 0;
#   undef CHR
}

int pstrlit(char *s, size_t slen){
    return fstrlit(stdout, s, slen);
}

int keyev(int ret, int kbd, int key, int pressed){
    if(ret) return ret;
    struct input_event events[] = {
        {
            .type = EV_KEY,
            .code = key,
            .value = pressed,
        },
        {
            .type = EV_SYN,
            .code = SYN_REPORT,
        },
    };
    /* a pause after emitting a keypress seems to improve stability, though
       it isn't clear why or if there's somehwere else that would be more
       efficient to pause */
    usleep(100);
    // for(int i = 0; i < 2; i++){
    //     struct input_event ev = events[i];
    //     printf(
    //         "input_ev {.type=%d, .code=%d, .value=%d}\n",
    //         (int)ev.type, (int)ev.code, (int)ev.value
    //     );
    // }
    ssize_t n = write(kbd, events, sizeof(events));
    if(n < sizeof(events)){
        perror("write(kbd)");
        return 1;
    }
    return 0;
}

int start_kbd(void){
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    int ret = ioctl(fd, UI_SET_EVBIT, EV_KEY);
    if(ret == -1){
        perror("ioctl(UI_SET_EVBIT, EV_KEY)");
        return -1;
    }
    ret = ioctl(fd, UI_SET_EVBIT, EV_SYN);
    if(ret == -1){
        perror("ioctl(UI_SET_EVBIT, EV_SYN)");
        return -1;
    }

    // enable all the keys
    for(size_t i = 1; i < 128; i++){ // up to and including KEY_COMPOSE
        ret = ioctl(fd, UI_SET_KEYBIT, i);
        if(ret == -1){
            perror("ioctl(UI_SET_KEYBIT, i)");
            return -1;
        }
    }

    struct uinput_user_dev uidev = {
        .id = {
           .bustype = BUS_USB,
           .vendor = 0x1111,
           .product = 0x0001,
           .version = 1,
        },
    };
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "test-io-kbd");

    ssize_t n = write(fd, &uidev, sizeof(uidev));
    if(n != sizeof(uidev)){
        perror("write(uidev)");
        return -1;
    }
    ret = ioctl(fd, UI_DEV_CREATE);
    if(ret == -1){
        perror("ioctl(UI_DEV_CREATE)");
        return -1;
    }

    return fd;
}

pid_t start_term(char **cmd){
    pid_t pid = fork();
    if(pid){
        // parent
        if(pid == 0) perror("fork");
        return pid;
    }

    // child
    execvp(cmd[0], cmd);
    perror("execvp");
    return -1;
}

int wait_for_input(int fd, suseconds_t usec, int *ok){
    *ok = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval tv = { .tv_usec = usec };

    int ret = select(fd+1, &rfds, NULL, NULL, &tv);
    if(ret == -1){
        perror("select()");
        return 1;
    }

    *ok = !!ret;

    return 0;
}

int read_expected(int ret, int conn, const char *bytes, size_t len){
    if(ret) return ret;
    char buf[1024];
    if(len > sizeof(buf)){
        fprintf(stderr,
            "read_expected len too long (%zu > %zu)\n", len, sizeof(buf)
        );
        return 1;
    }
    size_t total = 0;
    while(total < len){
        // wait 1s for input
        int ok;
        ret = wait_for_input(conn, 1000000, &ok);
        if(ret) return ret;
        if(!ok) break; // 1s without input
        ssize_t n = read(conn, buf + total, sizeof(buf) - total);
        if(n < 0){
            perror("read(conn)");
            return 1;
        }
        total += (size_t)n;
    }
    if(total != len || memcmp(buf, bytes, len) != 0){
        fprintf(stderr,   "expected: "); fstrlit(stderr, bytes, len);
        fprintf(stderr, "\nbut got : "); fstrlit(stderr, buf, total);
        fprintf(stderr, "\n");
        return 1;
    }
    return 0;
}

// write to the sock and wait for confirmation that all has been written to tty
int send_msg(int ret, int conn, const char *bytes, size_t len){
    if(ret) return ret;
    ssize_t n = write(conn, bytes, len);
    if(n < 0){
        perror("write");
        return 1;
    }
    if(n < len){
        fprintf(stderr, "incomplete write(conn): %zd < %zu\n", n, len);
        return 1;
    }
    size_t buf[32];
    size_t total = 0;
    while(total < len){
        // wait 1s for input
        int ok;
        ret = wait_for_input(conn, 1000000, &ok);
        if(ret) return ret;
        if(!ok){
            fprintf(stderr, "wrote %zu characters to conn (", len);
            fstrlit(stderr, bytes, len);
            fprintf(stderr, ") but only got %zu confirmations\n", total);
            return 1;
        }
        n = read(conn, buf + total, sizeof(buf) - total);
        if(n < 0){
            perror("read(conn)");
            return 1;
        }
        total += (size_t)n;
    }
    return 0;
}

int await_child(pid_t expect_pid){
    int w;
    pid_t pid = wait(&w);
    if(pid == -1){
        perror("wait");
        return 1;
    }
    if(pid != expect_pid){
        fprintf(stderr,
            "expected pid=%d but waited pid=%d instead\n", expect_pid, pid
        );
        return 1;
    }
    if(WIFEXITED(w)){
        int exitcode = WEXITSTATUS(w);
        if(exitcode){
            fprintf(stderr, "child exited %d\n", exitcode);
            return exitcode;
        }
    }else if(WIFSIGNALED(w)){
        int signum = WTERMSIG(w);
        fprintf(stderr, "child killed by signal %d\n", signum);
        return 128 + signum;
    }else{
        fprintf(stderr, "something weird happened to our child\n");
        return 1;
    }
    return 0;
}

#define KEY(x) do { \
    ret = keyev(ret, kbd, KEY_##x, 1); \
    ret = keyev(ret, kbd, KEY_##x, 0); \
} while(0)

#define MOD(m, x) do { \
    ret = keyev(ret, kbd, m, 1); \
    x; \
    ret = keyev(ret, kbd, m, 0); \
} while(0)

#define SHIFT(x) MOD(KEY_LEFTSHIFT, x)
#define CTL(x) MOD(KEY_LEFTCTRL, x)
#define ALT(x) MOD(KEY_LEFTALT, x)

#define EXPECT(x) ret = read_expected(ret, conn, x, strlen(x))
#define EXPECT_N(x, n) ret = read_expected(ret, conn, x, n)

// all alphabet keys
int alpha(int ret, int kbd){
    KEY(A); KEY(B); KEY(C); KEY(D); KEY(E); KEY(F); KEY(G); KEY(H); KEY(I);
    KEY(J); KEY(K); KEY(L); KEY(M); KEY(N); KEY(O); KEY(P); KEY(Q); KEY(R);
    KEY(S); KEY(T); KEY(U); KEY(V); KEY(W); KEY(X); KEY(Y); KEY(Z);
    return ret;
}
#define ALPHA ret = alpha(ret, kbd)

// all numeric keys
int num(int ret, int kbd){
    KEY(1); KEY(2); KEY(3); KEY(4); KEY(5);
    KEY(6); KEY(7); KEY(8); KEY(9); KEY(0);
    return ret;
}
#define NUM ret = num(ret, kbd)

int punc(int ret, int kbd){
    KEY(GRAVE); KEY(MINUS); KEY(BACKSLASH); KEY(SEMICOLON); KEY(APOSTROPHE);
    KEY(COMMA); KEY(DOT); KEY(SLASH); KEY(LEFTBRACE); KEY(RIGHTBRACE);
    return ret;
}
#define PUNC ret = punc(ret, kbd)

int arrows(int ret, int kbd){
    KEY(UP); KEY(DOWN); KEY(RIGHT); KEY(LEFT);
    return ret;
}
#define ARROWS ret = arrows(ret, kbd)

int special(int ret, int kbd){
    KEY(SPACE); KEY(TAB); KEY(BACKSPACE); KEY(ENTER);
    return ret;
}
#define SPECIAL ret = special(ret, kbd)

int insdel(int ret, int kbd){
    KEY(INSERT); KEY(HOME); KEY(PAGEUP);
    KEY(DELETE); KEY(END); KEY(PAGEDOWN);
    return ret;
}
#define INSDEL ret = insdel(ret, kbd)

// skip ins/pgup/pgdn due to their hardcoded xterm shifted behaviors
int insdel2(int ret, int kbd){
    KEY(DELETE); KEY(HOME); KEY(END);
    return ret;
}
#define INSDEL2 ret = insdel2(ret, kbd)


int kpnum(int ret, int kbd){
    KEY(KP7); KEY(KP8); KEY(KP9);
    KEY(KP4); KEY(KP5); KEY(KP6);
    KEY(KP1); KEY(KP2); KEY(KP3);
              KEY(KP0);
    return ret;
}
#define KPNUM ret = kpnum(ret, kbd)

int kppunc(int ret, int kbd){
    KEY(KPASTERISK);
    KEY(KPMINUS);
    KEY(KPPLUS);
    KEY(KPCOMMA); // KPCOMMA seems to be the '.' on the keypad
    KEY(KPSLASH);
    KEY(KPENTER);
    return ret;
}
#define KPPUNC ret = kppunc(ret, kbd)

int check_numlock(int ret, int kbd, int conn, int *numlock){
    if(ret) return ret;
    KEY(KP1);
    if(ret) return ret;
    char buf[32];
    ssize_t n = read(conn, buf, sizeof(buf));
    if(n < 0){
        perror("read conn");
        return 1;
    }
    *numlock = (n == 1 && buf[0] == '1');
    return 0;
}

int numlock_on(int ret, int kbd, int *numlock){
    if(ret) return ret;
    if(!*numlock){
        KEY(NUMLOCK);
        if(ret) return ret;
        *numlock = 1;
    }
    return 0;
}

int numlock_off(int ret, int kbd, int *numlock){
    if(ret) return ret;
    if(*numlock){
        KEY(NUMLOCK);
        if(ret) return ret;
        *numlock = 0;
    }
    return 0;
}

void testlog(int ret, char *msg){
    if(ret) return;
    fprintf(stderr, "- %s\n", msg);
}

int run_test(int kbd, int conn){
    int ret = 0;

    int numlock;
    ret = check_numlock(ret, kbd, conn, &numlock);

    // no modifiers
    testlog(ret, "no modifiers");
    ALPHA; EXPECT("abcdefghijklmnopqrstuvwxyz");
    NUM; EXPECT("1234567890");
    PUNC; EXPECT("`-\\;',./[]");
    ARROWS; EXPECT("\x1b[A\x1b[B\x1b[C\x1b[D");
    SPECIAL; EXPECT(" \t\b\r");
    INSDEL; EXPECT("\x1b[2~\x1b[H\x1b[5~"
                   "\x1b[3~\x1b[F\x1b[6~");
    ret = numlock_off(ret, kbd, &numlock);
    KPNUM; EXPECT("\x1b[H\x1b[A\x1b[5~"
                  "\x1b[D\x1b[E\x1b[C"
                  "\x1b[F\x1b[B\x1b[6~"
                        "\x1b[2~");
    KPPUNC; EXPECT("*-+./\r");
    ret = numlock_on(ret, kbd, &numlock);
    KPNUM; EXPECT("7894561230");
    KPPUNC; EXPECT("*-+./\r");

    // shift + everything
    testlog(ret, "shift + everything");
    SHIFT(ALPHA); EXPECT("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    SHIFT(NUM); EXPECT("!@#$%^&*()");
    SHIFT(PUNC); EXPECT("~_|:\"<>?{}");
    SHIFT(ARROWS); EXPECT("\x1b[1;2A\x1b[1;2B\x1b[1;2C\x1b[1;2D");
    SHIFT(SPECIAL); EXPECT(" \x1b[Z\b\r");
    SHIFT(INSDEL2); EXPECT("\x1b[3;2~\x1b[1;2H\x1b[1;2F");
    ret = numlock_off(ret, kbd, &numlock);
    SHIFT(KPNUM); EXPECT("\x1b[1;2H\x1b[1;2A\x1b[5;2~"
                         "\x1b[1;2D\x1b[1;2E\x1b[1;2C"
                         "\x1b[1;2F\x1b[1;2B\x1b[6;2~"
                                  "\x1b[2;2~");
    SHIFT(KPPUNC); EXPECT("*./\r");
    ret = numlock_on(ret, kbd, &numlock);
    SHIFT(KPNUM); EXPECT("\x1b[1;2H\x1b[1;2A\x1b[5;2~"
                         "\x1b[1;2D\x1b[1;2E\x1b[1;2C"
                         "\x1b[1;2F\x1b[1;2B\x1b[6;2~"
                                  "\x1b[2;2~");
    SHIFT(KPPUNC); EXPECT("*./\r");

    // ctrl+everything
    testlog(ret, "ctrl + everything");
    CTL(ALPHA); EXPECT("\x01\x02\x03\x04\x05\x06\x07\x08\x09"
                       "\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12"
                       "\x13\x14\x15\x16\x17\x18\x19\x1a");
    CTL(NUM); EXPECT_N("1\x00\x1b\x1c\x1d\x1e\x1f\x7f""90",10);
    CTL(PUNC); EXPECT_N("\x00-\x1c;',.\x1f\x1b\x1d", 10);
    CTL(ARROWS); EXPECT("\x1b[1;5A\x1b[1;5B\x1b[1;5C\x1b[1;5D");
    CTL(SPECIAL); EXPECT_N("\x00\t\x7f\r", 4);
    CTL(INSDEL);  EXPECT("\x1b[2;5~\x1b[1;5H\x1b[5;5~"
                         "\x1b[3;5~\x1b[1;5F\x1b[6;5~");
    ret = numlock_off(ret, kbd, &numlock);
    CTL(KPNUM); EXPECT("\x1b[1;5H\x1b[1;5A\x1b[5;5~"
                       "\x1b[1;5D\x1b[1;5E\x1b[1;5C"
                       "\x1b[1;5F\x1b[1;5B\x1b[6;5~"
                                "\x1b[2;5~");
    CTL(KPPUNC); EXPECT("*-+./\r");
    ret = numlock_on(ret, kbd, &numlock);
    CTL(KPNUM); EXPECT("7894561230");
    CTL(SHIFT((KPNUM))); EXPECT("\x1b[1;6H\x1b[1;6A\x1b[5;6~"
                                "\x1b[1;6D\x1b[1;6E\x1b[1;6C"
                                "\x1b[1;6F\x1b[1;6B\x1b[6;6~"
                                         "\x1b[2;6~");
    CTL(KPPUNC); EXPECT("*-+./\r");

    // alt+everything
    testlog(ret, "alt + everything");
    ALT(ALPHA); EXPECT("\xc3\xa1\xc3\xa2\xc3\xa3\xc3\xa4"
                       "\xc3\xa5\xc3\xa6\xc3\xa7\xc3\xa8"
                       "\xc3\xa9\xc3\xaa\xc3\xab\xc3\xac"
                       "\xc3\xad\xc3\xae\xc3\xaf\xc3\xb0"
                       "\xc3\xb1\xc3\xb2\xc3\xb3\xc3\xb4"
                       "\xc3\xb5\xc3\xb6\xc3\xb7\xc3\xb8"
                       "\xc3\xb9\xc3\xba");
    ALT(NUM); EXPECT("\xc2\xb1\xc2\xb2\xc2\xb3\xc2\xb4\xc2\xb5"
                     "\xc2\xb6\xc2\xb7\xc2\xb8\xc2\xb9\xc2\xb0");
    ALT(PUNC); EXPECT("\xc3\xa0\xc2\xad\xc3\x9c\xc2\xbb\xc2\xa7"
                      "\xc2\xac\xc2\xae\xc2\xaf\xc3\x9b\xc3\x9d");
    ALT(ARROWS); EXPECT("\x1b[1;3A\x1b[1;3B\x1b[1;3C\x1b[1;3D");
    ALT(SPECIAL); EXPECT("\xc2\xa0\xc2\x89\xc2\x88");
    ALT(INSDEL);  EXPECT("\x1b[2;3~\x1b[1;3H\x1b[5;3~"
                         "\x1b[3;3~\x1b[1;3F\x1b[6;3~");
    ret = numlock_off(ret, kbd, &numlock);
    ALT(KPNUM); EXPECT("\x1b[1;3H\x1b[1;3A\x1b[5;3~"
                       "\x1b[1;3D\x1b[1;3E\x1b[1;3C"
                       "\x1b[1;3F\x1b[1;3B\x1b[6;3~"
                                "\x1b[2;3~");
    ALT(KPPUNC); EXPECT("*-+./\r");
    ret = numlock_on(ret, kbd, &numlock);
    ALT(KPNUM); EXPECT("7894561230");
    ALT(KPPUNC); EXPECT("*-+./\r");

    // appcursor mode
    testlog(ret, "appcursor mode");
    ret = send_msg(ret, conn, APPCURSOR_ON, strlen(APPCURSOR_ON));
    #define ARRHE ARROWS; KEY(HOME); KEY(END)
    ARRHE; EXPECT("\x1bOA\x1bOB\x1bOC\x1bOD\x1bOH\x1bOF");
    SHIFT(ARRHE); EXPECT("\x1b[1;2A\x1b[1;2B\x1b[1;2C\x1b[1;2D\x1b[1;2H\x1b[1;2F");
    ALT(ARRHE); EXPECT("\x1b[1;3A\x1b[1;3B\x1b[1;3C\x1b[1;3D\x1b[1;3H\x1b[1;3F");
    ALT(SHIFT(ARRHE)); EXPECT("\x1b[1;4A\x1b[1;4B\x1b[1;4C\x1b[1;4D\x1b[1;4H\x1b[1;4F");
    CTL(ARRHE); EXPECT("\x1b[1;5A\x1b[1;5B\x1b[1;5C\x1b[1;5D\x1b[1;5H\x1b[1;5F");
    CTL(SHIFT(ARRHE)); EXPECT("\x1b[1;6A\x1b[1;6B\x1b[1;6C\x1b[1;6D\x1b[1;6H\x1b[1;6F");
    CTL(ALT(ARRHE)); EXPECT("\x1b[1;7A\x1b[1;7B\x1b[1;7C\x1b[1;7D\x1b[1;7H\x1b[1;7F");
    CTL(ALT(SHIFT(ARRHE))); EXPECT("\x1b[1;8A\x1b[1;8B\x1b[1;8C\x1b[1;8D\x1b[1;8H\x1b[1;8F");
    ret = send_msg(ret, conn, APPCURSOR_OFF, strlen(APPCURSOR_OFF));

    // appkeypad mode
    testlog(ret, "appkeypad mode");
    ret = numlock_on(ret, kbd, &numlock);
    ret = send_msg(ret, conn, APPKEYPAD_ON, strlen(APPKEYPAD_ON));
    ret = numlock_off(ret, kbd, &numlock);
    // XXX: test all modifier combinations here, because this shit is weird
    printf("---------\n");
    KPNUM; EXPECT("\x1b[H\x1b[A\x1b[5~"
                  "\x1b[D\x1b[E\x1b[C"
                  "\x1b[F\x1b[B\x1b[6~"
                        "\x1b[2~");
    printf("---------\n");
    KPPUNC; EXPECT("\x1bOj\x1bOm\x1bOk\x1bOn\x1bOo\x1bOM");
    ret = numlock_on(ret, kbd, &numlock);
    KPNUM; EXPECT("\x1bOw\x1bOx\x1bOy"
                  "\x1bOt\x1bOu\x1bOv"
                  "\x1bOq\x1bOr\x1bOs"
                        "\x1bOp");
    KPPUNC; EXPECT("\x1bOj\x1bOm\x1bOk\x1bOn\x1bOo\x1bOM");
    ret = send_msg(ret, conn, APPKEYPAD_OFF, strlen(APPKEYPAD_OFF));

    // match something simple to ensure we matched all input
    KEY(E);KEY(N);KEY(D);KEY(SPACE);
    KEY(O);KEY(F);KEY(SPACE);
    KEY(T);KEY(E);KEY(S);KEY(T);
    EXPECT("end of test");

    close(conn);
    return ret;
}

int test_against_term(int sock, int kbd){
    // wait for a connection from io process inside terminal
    int conn = accept(sock, NULL, 0);
    if(conn < 0){
        perror("accept");
        return 1;
    }
    usleep(500000);

    // TODO: why does xterm start with weird numlock resource on?
    int ret = 0;
    ret = send_msg(ret, conn, "\x1b[?1035l", 8);
    if(ret) return 1;

    int retval = run_test(kbd, conn);

    close(conn);

    return retval;
}

int main(int argc, char **argv){
    int ret;

    uid_t uidnow = getuid();
    gid_t gidnow = getgid();

    if(uidnow || gidnow){
        // re-invoke with sudo, configured to drop back to the current user
        char uid[32];
        char gid[32];
        snprintf(uid, sizeof(uid), "%d", getuid());
        snprintf(gid, sizeof(gid), "%d", getgid());
        execlp("sudo", "sudo", argv[0], uid, gid, NULL);
        perror("execlp");
        return 1;
    }

    //// now we are sudo ////

    int kbd = start_kbd();
    if(kbd < 0){
        return 1;
    }

    if(argc > 2){
        int newuid = atoi(argv[1]);
        int newgid = atoi(argv[2]);
        ret = setgid(newgid);
        if(ret){
            perror("setgid");
            return 1;
        }
        ret = setuid(newuid);
        if(ret){
            perror("setuid");
            return 1;
        }
    }

    //// now we are the original user ////

    // make a unix socket
    const char *sockpath = "test.sock";
    unlink(sockpath);
    int sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(sock < 0){
        perror("socket");
        return 1;
    }
    struct sockaddr_un sockaddr = { .sun_family = AF_UNIX };
    strncpy(sockaddr.sun_path, sockpath, sizeof(sockaddr.sun_path) - 1);
    ret = bind(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if(ret){
        perror("bind");
        return 1;
    }
    ret = listen(sock, 5);
    if(ret){
        perror("listen");
        return 1;
    }

    // run io test suite against xterm
    fprintf(stderr, "testing against xterm\n");
    char *xterm[] = {"xterm", "-fa", "Monospace", "-fs", "16", "./io", NULL};
    pid_t pid = start_term(xterm);
    if(pid < 0) return 1;
    int retval = test_against_term(sock, kbd);
    ret = await_child(pid);
    if(retval) return retval;
    if(ret) return ret;

    // run the same io test suite against nast
    fprintf(stderr, "testing against nast\n");
    char *nast[] = {"./nast", "./io", NULL};
    pid = start_term(nast);
    if(pid < 0) return 1;
    retval = test_against_term(sock, kbd);
    ret = await_child(pid);
    if(retval) return retval;
    if(ret) return ret;

    printf("PASS!\n");

    return 0;
}
