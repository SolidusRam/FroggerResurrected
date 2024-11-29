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

    //la rana si muove con le frecce e deve raggiungere il la parte alta dello schermo
    if (pipe(pipefd) == -1){
        perror("Errore nella creazione della pipe!\n");
        _exit(1);
    }

    pid_rana = fork();

    if (pid_rana == -1){
        perror("Errore nell\'esecuzione della fork!\n");
        exit(1);
    }else if (pid_rana == 0){
        close(pipefd[0]);  //chiudo il descrittore di lettura
        rana(pipefd[1]);  //invoco la funzione rana
    }

    //avvio funzione controllo
    close(pipefd[1]);  //chiudo il descrittore di scrittura
    game(pipefd[0]);
    kill(pid_rana, SIGTERM);
    return 0;

}