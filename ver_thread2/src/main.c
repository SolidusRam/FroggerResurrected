#include "game.h"
#include "buffer.h"
#include "player.h"
#include "utils.h"
#include "crocodile.h"

int main(){
    initscr();
    start_color();
    
    // Inizializzazione dei colori
    init_pair(1, COLOR_GREEN, COLOR_BLACK);    // Colore rana
    init_pair(2, COLOR_RED, COLOR_BLACK);      // Colore coccodrillo
    init_pair(3, COLOR_BLUE, COLOR_CYAN);      // Bordi fiume
    init_pair(5, COLOR_YELLOW, COLOR_RED);     // Terra (partenza/arrivo)
    init_pair(6, COLOR_GREEN, COLOR_BLACK);    // Tana vuota
    init_pair(7, COLOR_YELLOW, COLOR_GREEN);   // Tana occupata
    
    // Controllo dimensioni schermo
    if (LINES < GAME_HEIGHT || COLS < GAME_WIDTH) {
        endwin();
        fprintf(stderr, "Terminal too small. Needs at least %dx%d\n", GAME_WIDTH, GAME_HEIGHT);
        exit(1);
    }
    
    resize_term(GAME_HEIGHT, GAME_WIDTH);
    
    cbreak();          // Modalità di input raw
    keypad(stdscr, TRUE);
    noecho();
    nodelay(stdscr, TRUE);  // Input non bloccante
    curs_set(0);       // Nasconde il cursore
    
    srand(time(NULL));
    
    // Inizializzazione del buffer
    initBuffer(&buffer);
    pthread_mutex_init(&lock, NULL);
    
    // Inizializzazione dei threads
    pthread_t game_tid;
    pthread_t player_tid;
    pthread_t crocodile_tids[MAX_CROCODILES];
    
    int num_crocodiles = LANES * 2; // 2 coccodrilli per corsia
    
    // Crea e avvia il thread di gioco
    pthread_create(&game_tid, NULL, game_thread, NULL);
    
    // Crea e avvia il thread del giocatore
    pthread_create(&player_tid, NULL, player_thread, NULL);
    
    // Crea e avvia i thread dei coccodrilli
    for (int i = 0; i < num_crocodiles; i++) {
        CrocodileArg *arg = malloc(sizeof(CrocodileArg));
        arg->id = i;
        pthread_create(&crocodile_tids[i], NULL, crocodile_thread, arg);
    }
    
    // Attendi il termine del thread di gioco (il thread principale)
    pthread_join(game_tid, NULL);
    
    // Cancella gli altri thread
    pthread_cancel(player_tid);
    for (int i = 0; i < num_crocodiles; i++) {
        pthread_cancel(crocodile_tids[i]);
    }
    
    // Distruzione del buffer e del mutex
    pthread_mutex_destroy(&lock);
    destroyBuffer(&buffer);
    
    endwin();
    return 0;
}