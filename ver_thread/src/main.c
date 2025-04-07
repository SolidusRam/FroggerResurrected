#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <ncurses.h>
#include <signal.h>

#include "../include/game.h"
#include "../include/utils.h"

// Stato globale per gestire il segnale
game_state* global_state = NULL;

// Gestore del segnale per aver un'uscita più pulita
void cleanup_handler(int signo) {
    if (global_state) {
        global_state->game_over = true;
    }
}

int main() {
    // Inizializzazione del seed
    srand(time(NULL));
    
    // Inizializzazione di ncurses
    initscr();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Colore del player
    init_pair(2, COLOR_RED, COLOR_BLACK);    // Colore del coccodrillo
    init_pair(3, COLOR_BLUE, COLOR_CYAN);    // bordi del fiume
    init_pair(5, COLOR_YELLOW, COLOR_RED);   // Colore della terra
    init_pair(6, COLOR_GREEN, COLOR_BLACK);  // tana vuota
    init_pair(7, COLOR_YELLOW, COLOR_GREEN); // tana occupata
    
    // Controllo dimensione del terminale
    if (LINES < GAME_HEIGHT || COLS < GAME_WIDTH) {
        endwin();
        fprintf(stderr, "Terminal too small. Needs at least %dx%d but got %dx%d\n", 
                GAME_WIDTH, GAME_HEIGHT, COLS, LINES);
        exit(1);
    }
    
    // Setta il terminale
    resize_term(GAME_HEIGHT, GAME_WIDTH);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);  
    
    // Pulizia iniziale dello schermo per evitare caratteri casuali
    clear();
    box(stdscr, ACS_VLINE, ACS_HLINE);
    refresh();
    
    // Creazione dello stato di gioco
    // Inizializzazione dello stato di gioco
    game_state state;
    init_game_state(&state);
    global_state = &state;  
    
    // Segnali di terminazione
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    
    // Disegno iniziale dello stato di gioco
    draw_game_state(&state);
    
    // Creazione dei threads
    pthread_t player_tid, game_tid;
    pthread_t crocodile_tids[MAX_CROCODILES];
    crocodile_args* croc_args[MAX_CROCODILES];
    
    // Inizio game thread
    pthread_create(&game_tid, NULL, game_thread, &state);
    
    // Inizio player thread
    pthread_create(&player_tid, NULL, player_thread, &state);
    
    // Inizio threads dei coccodrilli
    for (int i = 0; i < MAX_CROCODILES; i++) {
        croc_args[i] = malloc(sizeof(crocodile_args));
        croc_args[i]->state = &state;
        croc_args[i]->id = i;
        pthread_create(&crocodile_tids[i], NULL, crocodile_thread, croc_args[i]);
    }
    
    // CHiusura del thread di gioco
    pthread_join(game_tid, NULL);
    
    // Game over, clean up
    for (int i = 0; i < MAX_CROCODILES; i++) {
        pthread_cancel(crocodile_tids[i]);
        pthread_join(crocodile_tids[i], NULL);
        free(croc_args[i]); // pulizia della memoria
    }
    pthread_cancel(player_tid);
    pthread_join(player_tid, NULL);
    
    
    // Clean up proiettili
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state.bullets[i].pos.active) {
            pthread_cancel(state.bullets[i].thread_id);
            pthread_join(state.bullets[i].thread_id, NULL);
        }
    }
    
    // Pulizia dello stato di gioco
    destroy_game_state(&state);
    global_state = NULL;
    
    clear();
    mvprintw(LINES/2, COLS/2 - 15, "Game Over - Score: %d", state.score);
    refresh();
    sleep(2);
    
    // End ncurses
    endwin();
    
    return 0;
}
