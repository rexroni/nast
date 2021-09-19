#include <stdio.h>
#include <unistd.h>
#include <termios.h>


int main(){
    if(!isatty(0)){
        fprintf(stderr, "stdin is not a tty!\n");
        return 1;
    }

    struct termios t;
    int ret = tcgetattr(0, &t);
    if(ret){
        perror("tcgetattr()");
        return 1;
    }

    printf("t.c_iflag: ");
    for(size_t i = 0; i < sizeof(t.c_iflag) * 8; i++){
        int v = !!(t.c_iflag & (1<<i));
        printf("%d ", v);
    }
    printf("\n");

    printf("t.c_oflag: ");
    for(size_t i = 0; i < sizeof(t.c_oflag) * 8; i++){
        int v = !!(t.c_oflag & (1<<i));
        printf("%d ", v);
    }
    printf("\n");

    printf("t.c_cflag: ");
    for(size_t i = 0; i < sizeof(t.c_cflag) * 8; i++){
        int v = !!(t.c_cflag & (1<<i));
        printf("%d ", v);
    }
    printf("\n");

    printf("t.c_lflag: ");
    for(size_t i = 0; i < sizeof(t.c_lflag) * 8; i++){
        int v = !!(t.c_lflag & (1<<i));
        printf("%d ", v);
    }
    printf("\n");

    printf("t.c_cc: ");
    for(size_t i = 0; i < sizeof(t.c_cc); i++){
        printf("%.2x ", (int)t.c_cc[i]);
    }
    printf("\n");

    return 0;
}
