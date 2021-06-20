#include <stdio.h>
#include <unistd.h>
#include <termios.h>

void show_characters_from_tty(){
    struct termios tios;
    // store terminal settings
    tcgetattr(0, &tios);
    struct termios raw_tios = tios;
    cfmakeraw(&raw_tios);
    tcsetattr(0, TCSANOW, &raw_tios);

    char byte;
    while(read(0, &byte, 1)){
        if(byte == 'q') break;
        printf("dec = %d, hex = %x, char = %c\r\n", (int)byte, (int)byte, byte);
    }

    // reset terminal settings
    tcsetattr(0, TCSANOW, &tios);
}

int main(){
    printf("showing raw inputs (press q to quit)...\n");

    show_characters_from_tty();
    return 0;
}
