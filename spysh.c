#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pty.h>

int childpid = -1;

static void cleanup(int signum){
    if(childpid > 0){
        kill(childpid, SIGKILL);
        int trash;
        wait(&trash);
    }
    exit(0);
}

static const short rd_ev = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
static const short wr_ev = POLLOUT | POLLWRNORM | POLLWRBAND | POLLPRI;

static void start_read(struct pollfd *pfd){
    pfd->events |= rd_ev;
}

static void stop_read(struct pollfd *pfd){
    pfd->events &= ~rd_ev;
}

static void start_write(struct pollfd *pfd){
    pfd->events |= wr_ev;
}

static void stop_write(struct pollfd *pfd){
    pfd->events &= ~wr_ev;
}

int record(char *logfile, int ischild, char *buf, size_t len){
    FILE *f = fopen(logfile, "a");
    if(!f){
        perror(logfile);
        return -1;
    }

    int N = 12;
    int BUF = N * 4;

    // read in N-byte chunks
    for(size_t i = 0; i < len; i += N){
        if(i){
            // emit a newline every N-byte chunk
            fputc('\n', f);
        }
        // child gets a buffer before N-byte chunk
        if(ischild){
            for(int j = 0; j < BUF; j++){
                fputc(' ', f);
            }
        }

        // emit every byte, hex-encoded
        for(size_t j = 0; j < N; j++){
            if(i+j < len){
                fprintf(f, "%.2x ", buf[i+j]);
            }else{
                fprintf(f, "   ");
            }
        }

        // emit the ascii-encoded byte when possible.
        for(size_t j = 0; j < N && i+j < len; j++){
            char c = buf[i+j];
            if(c >= ' ' && c <= '~'){
                fputc(c, f);
            }else{
                fprintf(f, "·");
            }
        }
    }
    // finish with a newline
    fputc('\n', f);
    fflush(f);
    fclose(f);
    return 0;
}

int main(int argc, char** argv){
    if(argc < 3){
        fprintf(stderr, "usage: %s LOGFILE SHELL [ARGS...]\n", argv[0]);
        return 1;
    }
    char *logfile = argv[1];
    remove(logfile);

    if(!isatty(0)){
        fprintf(stderr, "stdin is not a tty!\n");
        return 2;
    }
    int parent_tty = 0;

    struct termios ttyattr;
    int ret = tcgetattr(parent_tty, &ttyattr);
    if(ret){
        perror("tcgetattr()");
        return 3;
    }

    struct winsize ttysize;
    ret = ioctl(parent_tty,TIOCGWINSZ,&ttysize);
    if(ret){
        perror("ioctl(TIOCGWINSZ)");
        return 4;
    }

    // fork subshell into it's own pty
    int child_tty;
    childpid = forkpty(&child_tty, NULL, &ttyattr, &ttysize);
    if(childpid == 0){
        execvp(argv[2], &argv[2]);
        perror("execv");
        usleep(1e6);
        return 1;
    }
    if(childpid < 0){
        perror("forkpty()");
        return 5;
    }

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGCHLD, cleanup);

    ///// copy loop /////

    struct pollfd pfds[2];
    struct pollfd *parent_pfd = &pfds[0];
    struct pollfd *child_pfd = &pfds[1];

    parent_pfd->fd = parent_tty;
    child_pfd->fd = child_tty;

    // start listening to either pfd
    start_read(parent_pfd);
    start_read(child_pfd);

    char from_parent[1024];
    size_t plen = 0;
    char from_child[1024];
    size_t clen = 0;

    while(1){
        ret = poll(pfds, 2, -1);
        if(ret < 0){
            perror("poll()");
            return 6;
        }

        // events I don't know how to handle
        if(parent_pfd->revents & POLLPRI){
            fprintf(stderr, "parent_pfd set POLLPRI... what do I do?\n");
            cleanup(0);
        }
        if(child_pfd->revents & POLLPRI){
            fprintf(stderr, "child_pfd set POLLPRI... what do I do?\n");
            cleanup(0);
        }

        // error events
        if(parent_pfd->revents & (POLLERR | POLLHUP)){
            fprintf(stderr, "parent_pfd failed\n");
            cleanup(0);
        }
        if(child_pfd->revents & (POLLERR | POLLHUP)){
            fprintf(stderr, "child_pfd failed\n");
            cleanup(0);
        }

        // read events
        if(parent_pfd->revents & (POLLIN | POLLRDNORM | POLLRDBAND)){
            ssize_t n = read(parent_tty, from_parent, sizeof(from_parent));
            if(n < 0){
                perror("read(parent_tty)");
                cleanup(0);
            }
            plen = (size_t)n;
            record(logfile, 0, from_parent, plen);
            // don't read any more until we empty the buffer
            stop_read(parent_pfd);
            start_write(child_pfd);
        }
        if(child_pfd->revents & (POLLIN | POLLRDNORM | POLLRDBAND)){
            ssize_t n = read(child_tty, from_child, sizeof(from_child));
            if(n < 0){
                perror("read(child_tty)");
                cleanup(0);
            }
            clen = (size_t)n;
            record(logfile, 1, from_child, clen);
            // don't read any more until we empty the buffer
            stop_read(child_pfd);
            start_write(parent_pfd);
        }

        // write events
        if(parent_pfd->revents & (POLLOUT | POLLWRNORM | POLLWRBAND)){
            ssize_t n = write(parent_tty, from_child, clen);
            if(n < 0){
                perror("write(child_tty)");
                cleanup(0);
            }
            clen -= n;
            if(clen > 0){
                // more left to write; leftshift the buffer
                memmove(from_child, from_child + n, clen);
            }else{
                // transition back to reading from the child
                stop_write(parent_pfd);
                start_read(child_pfd);
            }
        }
        if(child_pfd->revents & (POLLOUT | POLLWRNORM | POLLWRBAND)){
            ssize_t n = write(child_tty, from_parent, plen);
            if(n < 0){
                perror("write(parent_tty)");
                cleanup(0);
            }
            plen -= n;
            if(plen > 0){
                // more left to write; leftshift the buffer
                memmove(from_parent, from_parent + n, plen);
            }else{
                // transition back to reading from the parent
                stop_write(child_pfd);
                start_read(parent_pfd);
            }
        }
    }
    return 0;
}
