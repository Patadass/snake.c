/*
    Copyright © 2026 Ivan Ajdinski
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the “Software”), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const uint8_t HEIGHT = 20;
const uint8_t WIDTH  = 40;

/* struct for coordinates of snake and apple */
typedef struct scord_t{
    uint8_t x;
    uint8_t y;
} scord_t;

/* struct for moving direction of snake */
typedef struct mcord_t{
    int8_t x;
    int8_t y;
} mcord_t;

/* init snake body and starting apple position */
void init_snake(scord_t snake[], uint8_t* size, scord_t* apple){

    *size      = 4;
    apple->y   = HEIGHT / 2;
    apple->x   = WIDTH  / 4 + 5;

    snake[0].y = HEIGHT / 2;
    snake[0].x = WIDTH  / 4 - 5;

    uint8_t i;

    for(i = 1; i < *size;i++){
        snake[i].x = snake[i - 1].x - 1;
        snake[i].y = snake[i - 1].y;
    }

    srand(time(NULL)); // set seed for rand()
}


void draw_game(scord_t snake[], uint8_t size, scord_t apple){

    uint8_t i;
    uint8_t j;

    printf("\033[H"); // move cursor to 0, 0

    for(i = 0; i < HEIGHT; i++){
        for(j = 0; j < WIDTH; j++){
            if(j == 0){
                if(i == 0){
                    printf("┌");
                }else if(i + 1 >= HEIGHT){
                    printf("└");
                }else{
                    printf("│");
                }
            }else if(j + 1 >= WIDTH){
                if(i == 0){
                    printf("┐");
                }else if(i + 1 >= HEIGHT){
                    printf("┘");
                }else{
                    printf("│");
                }
            }else if(i == 0 || i + 1 >= HEIGHT){
                printf("─");
            }else{
                printf(" ");
            }
        }
        printf("\n");
    }

    printf("\nmove:\twasd or hjkl\n");
    printf("quit:\tq\n");
    printf("pause:\tp\n");

    printf("\033[0;2Hsnake.c");

    /* print apple */
    printf("\033[31m"); // color red
    printf("\033[%d;%dHO", apple.y + 1, apple.x * 2 + 1);

    /* print body then head, looks better when colliding head with body */
    printf("\033[32m"); // color green
    for(i = 1; i < size;i++){
        printf("\033[%d;%dH[]", snake[i].y + 1, snake[i].x * 2 + 1);
    }
    printf("\033[%d;%dH()", snake[0].y + 1, snake[0].x * 2 + 1);

    printf("\033[0m"); // reset color


    fflush(stdout);    // force flush stdout
}

uint8_t EXIT_STATUS = 0;
void input_handle(int signo, siginfo_t* info, void* context){

    EXIT_STATUS = 0;

    int stat;
    wait(&stat);                 // wait for child process that's listening for key press
    int key = WEXITSTATUS(stat); // get key pressed

    EXIT_STATUS = key;
}

uint8_t move_snake(scord_t snake[], uint8_t* size, scord_t* apple, mcord_t dir){

    uint8_t i;

    if(snake[0].x == 0){
        return 1;
    }
    if(snake[0].y == 0){
        return 1;
    }
    if(snake[0].x * 2 + 2 >= WIDTH){
        return 1;
    }
    if(snake[0].y + 1 >= HEIGHT){
        return 1;
    }

    if(snake[0].x == apple->x && snake[0].y == apple->y){


        // TODO: make getting random apple position faster
        bool in_snake;

        do{

            apple->x = rand() % (HEIGHT - 2) + 1;
            apple->y = rand() % (WIDTH / 2 - 2) + 1;
            in_snake = false;
            for(i = 0; i < *size; i++){
                if(snake[i].x == apple->x && snake[i].y == apple->y){
                    in_snake = true;
                    break;
                }
            }

        }while(in_snake);
        
        if(*size + 1 > HEIGHT * (WIDTH / 2)){
            return 1;
        }
        snake[*size].x = snake[*size - 1].x + dir.x;
        snake[*size].y = snake[*size - 1].y + dir.y;
        *size += 1;
    }

    for(i = *size - 1; i >= 1; i--){
        if(snake[0].x == snake[i].x && snake[0].y == snake[i].y){
            return 1; //collision with body
        }
        snake[i].x = snake[i - 1].x;
        snake[i].y = snake[i - 1].y;
    }

    snake[0].x += dir.x;
    snake[0].y += dir.y;
    return 0;
}

/* sleep for msec milliseconds */
int msleep(long msec){
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec  = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

/* set buffered input */
void set_buffered_input(bool enable){

    static bool    enabled = true;
    static struct  termios old;
    struct termios new;

    if(enable && !enabled){

        /* restore old setting */
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        enabled = true;

    }else if(!enable && enabled){

        /* get the terminal settings for standard input */
        tcgetattr(STDIN_FILENO, &new);
        old = new;

        /* disable canonical mode (buffered i/o) and local echo */
        new.c_lflag &= (~ICANON & ~ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new);

        enabled = false;
    }
}

// void input_handle(int signo, siginfo_t* info, void* context){
void signal_callback_handler(int signo, siginfo_t* info, void* context){
    printf("\033[%d;%dH", HEIGHT + 4, 0); // move cursor to end of board
    printf("\033[?25h\n");                // make cursor visible
    set_buffered_input(true);
    exit(signo);
}

int main(){

    /*
        from www.man7.org/linux/man-pages/man2/signal.2.html
        WARNING: the behavior of signal() varies across UNIX versions, and
        has also varied historically across different versions of Linux.
        Avoid its use: use sigaction(2) instead.
     */
    struct sigaction act_interupt = { 0 };
    act_interupt.sa_flags = SA_SIGINFO;
    act_interupt.sa_sigaction = &signal_callback_handler;
    if(sigaction(SIGINT, &act_interupt, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction act_input = { 0 };
    act_input.sa_flags = SA_SIGINFO;
    act_input.sa_sigaction = &input_handle;
    if(sigaction(SIGCHLD, &act_input, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("\033[?25l\033[2J"); // make cursor invisible and clear screen

    set_buffered_input(false);

    scord_t snake[HEIGHT * (WIDTH / 2)];
    scord_t apple;
    mcord_t dir;
    uint8_t size;

    dir.x = 1;
    dir.y = 0;

    init_snake(snake, &size, &apple);

    pid_t p;

    while(true){

        p = fork();

        if(p < 0){
            perror("fork fail");
            exit(1);
        }

        if(p == 0){
            exit(getchar());
        }

        uint8_t hit = 0;
        while(EXIT_STATUS == 0 && hit == 0){
            msleep(250);
            hit = move_snake(snake, &size, &apple, dir);
            draw_game(snake, size, apple);
        }


        bool leave = false;
        switch(EXIT_STATUS){
            case 'k':
            case 'w':
                if(dir.y != 1){
                    dir.x = 0;
                    dir.y = -1;
                }
            break;
            case 'j':
            case 's':
                if(dir.y != -1){
                    dir.x = 0;
                    dir.y = 1;
                }
            break;
            case 'l':
            case 'd':
                if(dir.x != -1){
                    dir.x = 1;
                    dir.y = 0;
                }
            break;
            case 'h':
            case 'a':
                if(dir.x != 1){
                    dir.x = -1;
                    dir.y = 0;
                }
            break;
            case 'p':
                printf("\033[%d;%dHPAUSE", HEIGHT / 2, WIDTH / 2 - 2);
                EXIT_STATUS = getchar();
                if(EXIT_STATUS == 'q'){
                    leave = true;
                }
            break;
            case 'q':
                leave = true;
            break;
        }

        if(leave || hit != 0){
            if(p != 0){
                kill(p, SIGKILL); // kill child process
            }
            break;
        }

        EXIT_STATUS = 0;
    }

    printf("\033[%d;%dH", HEIGHT + 4, 0); // move cursor to end
    printf("\033[?25h\033[2;0;0m\n");     // make cursor visible, reset color
    set_buffered_input(true);
}
