/*
   io: the inside-a-terminal helper process to test-io.c.

   io relays tty output from the terminal emulator to the test-io process, and
   relays tty inputs from the test-io process to the terminal emulator.
*/
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

int io(int sock){
    while(1){
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(sock, &rfds);

        int ret = select(sock+1, &rfds, NULL, NULL, NULL);
        if(ret < 0){
            perror("select");
            return 1;
        }
        char buf[32];
        char resp[32];
        memset(resp, '.', sizeof(resp));
        if(FD_ISSET(0, &rfds)){
            // stdin ready
            ssize_t n = read(0, buf, sizeof(buf));
            if(n < 0){
                perror("read(stdin)");
                return -1;
            }
            if(n == 0){
                fprintf(stderr, "EOF on stdin\n");
                return -1;
            }
            size_t len = (size_t)n;
            // copy to socket
            n = write(sock, buf, len);
            if(n < 0){
                perror("write(sock)");
                return -1;
            }
            if(n < len){
                fprintf(stderr, "incomplete write(sock): %zd < %zu\n", n, len);
                return -1;
            }
        }
        if(FD_ISSET(sock, &rfds)){
            // socket ready
            ssize_t n = read(sock, buf, sizeof(buf));
            if(n < 0){
                perror("read(sock)");
                return -1;
            }
            if(n == 0){
                // EOF on socket, means test is over
                return 0;
            }
            size_t len = (size_t)n;
            // copy to stdout
            n = write(1, buf, len);
            if(n < 0){
                perror("write(stdout)");
                return -1;
            }
            if(n < len){
                fprintf(
                    stderr, "incomplete write(stdout): %zd < %zu\n", n, len
                );
                return -1;
            }
            // respond with one '.' for every byte we wrote
            n = write(sock, resp, len);
            if(n < 0){
                perror("write(sock)");
                return -1;
            }
            if(n < len){
                fprintf(
                    stderr, "incomplete write(sock): %zd < %zu\n", n, len
                );
                return -1;
            }
        }
    }
}

int sock_main(int sock){
    // connect to unix socket
    const char *sockpath = "test.sock";
    struct sockaddr_un sockaddr = { .sun_family = AF_UNIX };
    strncpy(sockaddr.sun_path, sockpath, sizeof(sockaddr.sun_path) - 1);
    int ret = connect(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if(ret){
        perror("connect");
        return 1;
    }

    // make stdin nonblocking
    int flags = fcntl(0, F_GETFL);
    if(flags == -1){
        perror("fcntl(F_GETFL)");
        return 1;
    }
    ret = fcntl(0, F_SETFL, flags | O_NONBLOCK);
    if(ret == -1){
        perror("fcntl(F_SETFL)");
        return 1;
    }

    int retval = io(sock);

    close(sock);

    return retval;
}

int raw_main(void){
    int sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if(sock < 0){
        perror("socket");
        return 1;
    }

    int retval = sock_main(sock);

    close(sock);

    return retval;
}

int main(void){
    // log stderr to file
    int logfd = open("io.log", O_WRONLY | O_CREAT, 0666);
    if(logfd < 0){
        perror("open(io.log)");
        return 1;
    }
    close(2);
    dup(logfd);
    close(logfd);

    // make terminal raw
    struct termios tios;
    tcgetattr(0, &tios);
    struct termios raw_tios = tios;
    cfmakeraw(&raw_tios);
    tcsetattr(0, TCSANOW, &raw_tios);

    int retval = raw_main();

    tcsetattr(0, TCSANOW, &tios);

    return retval;
}
