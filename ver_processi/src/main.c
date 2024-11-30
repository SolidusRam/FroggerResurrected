#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "../include/game.h"
#include "../include/player.h"




int main(){

    //creo la pipe

    int pipefd[2];  //il campo 0 è per la lettura, il campo 1 è per la scrittura

    int pid_rana;
    int pid_coccodrillo;

    //inizializzo schermo
    initscr();
    if (LINES < GAME_HEIGHT || COLS < GAME_WIDTH) {
        endwin();
        fprintf(stderr, "Terminal too small. Needs at least %dx%d\n", GAME_WIDTH, GAME_HEIGHT);
        exit(1);
    }
    resize_term(GAME_HEIGHT, GAME_WIDTH);
    noecho();cbreak();nodelay(stdscr, TRUE);keypad(stdscr, TRUE);

    box(stdscr, ACS_VLINE, ACS_HLINE);

    //pavimento da ristrutturare
    for (int i = 0; i < ACS_VLINE; i++)
    {
        //disegno il pavimento parte inferiore
        mvaddch(FLOOR_HEIGHT, i, ACS_HLINE);

        //disegno il pavimento parte superiore
        mvaddch(3, i, ACS_HLINE);
    }

    
    refresh();

    //creazione dei processi e pipe
    //la rana si muove con le frecce e deve raggiungere il la parte alta dello schermo
    if (pipe(pipefd) == -1){
        perror("Errore nella creazione della pipe!\n");
        _exit(1);
    }

    pid_rana = fork();

    if (pid_rana == -1){
        perror("Errore nella creazione del processo figlio!\n");
        _exit(1);
    }else if(pid_rana == 0){
        close(pipefd[0]);
        rana(pipefd[1]);
    }

    pid_coccodrillo = fork();

    if (pid_coccodrillo == -1){
        perror("Errore nella creazione del processo figlio!\n");
        _exit(1);
    }else if(pid_coccodrillo == 0){
        close(pipefd[0]);
        coccodrillo(pipefd[1]);
    }

    close(pipefd[1]);
    game(pipefd[0]);
    kill(pid_rana, SIGTERM);
        

    endwin();
    return 0;
}