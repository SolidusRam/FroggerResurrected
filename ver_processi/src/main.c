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

    //inizializzo schermo
    initscr();noecho();cbreak();nodelay(stdscr, TRUE);keypad(stdscr, TRUE);

    box(stdscr, ACS_VLINE, ACS_HLINE);
    refresh();
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

    close(pipefd[1]);
    game(pipefd[0]);
    kill(pid_rana, SIGTERM);
        

    endwin();
    return 0;
}