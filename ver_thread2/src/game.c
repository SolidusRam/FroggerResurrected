#include "../include/game.h"
#include "../include/buffer.h"
#include "../include/utils.h"
#include <time.h>

// Sprite della rana (2 righe x 5 colonne)
char rana_sprite[2][5] = {
    {' ', ' ', 'O', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
};

void * game_thread(void * arg) {
    // Posizione assoluta della rana (gestita completamente da questo thread)
    int rana_x = GAME_WIDTH / 2;
    int rana_y = GAME_HEIGHT - 2;
    int rana_width = 5;
    int rana_height = 2;
    
    // Inizializzazione delle tane
    tana tane[NUM_TANE];
    init_dens(tane);
    int tane_occupate = 0;
    
    // Array per memorizzare le posizioni dei coccodrilli
    Position crocodile_positions[MAX_CROCODILES];
    
    
    // Inizializzazione dello score
    int score = 0;
    
    // Inizializzazione del tempo
    int max_time = 30; // 30 secondi per livello
    int remaining_time = max_time;
    time_t last_update = time(NULL);
    
    // Variabili per la gestione del tempo di sincronizzazione
    struct timespec last_sync_time;
    clock_gettime(CLOCK_MONOTONIC, &last_sync_time);
    const long SYNC_INTERVAL_NS = 100 * 1000000; // 100ms in nanosecondi
    
    // Buffer per messaggi di debug
    char debug_message[128] = "";
    
    while (!game_over) {
        // Consuma messaggi dal buffer
        Position pos = consume(&buffer);
        
        // Controlla il tempo trascorso per la sincronizzazione
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ns = (current_time.tv_sec - last_sync_time.tv_sec) * 1000000000 +
                         (current_time.tv_nsec - last_sync_time.tv_nsec);
        
        if (elapsed_ns >= SYNC_INTERVAL_NS) {
            // Sincronizzazione stati e controllo collisioni
            
            // Aggiorna il tempo rimanente
            time_t current_time_t = time(NULL);
            if (current_time_t - last_update >= 1) {
                last_update = current_time_t;
                remaining_time--;
                
                if (remaining_time <= 0) {
                    game_over = true;
                    mvprintw(GAME_HEIGHT/2, GAME_WIDTH/2 - 10, "TEMPO SCADUTO!");
                    refresh();
                    sleep(2);
                    break;
                }
            }
            
            // Aggiorno timestamp ultima sincronizzazione
            clock_gettime(CLOCK_MONOTONIC, &last_sync_time);
        }
        
        // Gestisco i messaggi in base al tipo
        switch(pos.type) {
        case 'F': // Messaggio dalla rana
            // Cancella la vecchia posizione della rana
            clear_frog_position(rana_x, rana_y, rana_width, rana_height);
            
            // Calcola la nuova posizione tentativa
            int new_x = rana_x + pos.dx;
            int new_y = rana_y + pos.dy;
            
            // Verifica i bordi dello schermo
            if (new_x >= 1 && new_x <= GAME_WIDTH - rana_width - 1 &&
                new_y >= 1 && new_y <= GAME_HEIGHT - rana_height - 1) {
                // Accetta il movimento solo se è nei limiti dello schermo
                rana_x = new_x;
                rana_y = new_y;
            }
            
            // Controllo se la rana ha raggiunto una tana
            if (rana_y <= 1) {
                if (check_den_collision(rana_x, rana_y, rana_width, rana_height, tane)) {
                    score += 100;
                    tane_occupate++;
                    
                    // Resetta la posizione della rana dopo aver riempito una tana
                    rana_x = GAME_WIDTH/2;
                    rana_y = GAME_HEIGHT-2;
                    
                    // Resetta il timer
                    remaining_time = max_time;
                    
                    // Aggiorna il messaggio di debug
                    snprintf(debug_message, sizeof(debug_message), "Tana occupata! %d/%d", tane_occupate, NUM_TANE);
                    
                    if (tane_occupate == NUM_TANE) {
                        clear();
                        mvprintw(GAME_HEIGHT/2, GAME_WIDTH/2-10, "HAI VINTO!");
                        refresh();
                        sleep(2);
                        game_over = true;
                        break;
                    }
                }
            }
            break;
            
        case 'C': // Messaggio da un coccodrillo
            // Aggiorna la posizione del coccodrillo corrispondente
            for (int i = 0; i < MAX_CROCODILES; i++) {
                if ( crocodile_positions[i].id == pos.id) {
                    crocodile_positions[i] = pos;
                    break;
                }
            }
            break;
        
        default:
            // Gestisci altri tipi di messaggi se necessario
            break;
        }
        
        // Controllo se il gioco è terminato
        if (pos.gameOver) {
            game_over = true;
            break;
        }
        
        // Inizio funzioni di disegno
        // Pulisco schermo
        clear();
        
        // Disegno i bordi e l'area di gioco
        box(stdscr, ACS_VLINE, ACS_HLINE);
        draw_river_borders();
        draw_game_borders();
        draw_dens(tane);
        
        // Disegno i coccodrilli
        attron(COLOR_PAIR(2));
        for (int i = 0; i < MAX_CROCODILES; i++) {
            // Verifica se il coccodrillo è completamente visibile
            if (crocodile_positions[i].x >= 1 && 
                crocodile_positions[i].x + crocodile_positions[i].width <= GAME_WIDTH - 1) {
                // Disegna normalmente
                for (int h = 0; h < crocodile_positions[i].height; h++) {
                    for (int w = 0; w < crocodile_positions[i].width; w++) {
                        mvaddch(crocodile_positions[i].y + h, 
                                crocodile_positions[i].x + w, 'C');
                    }
                }
            } else {
                // Disegna solo la parte visibile
                int start_x = crocodile_positions[i].x;
                int width = crocodile_positions[i].width;
                
                // Aggiusta per il bordo sinistro
                if (start_x < 1) {
                    width += start_x - 1;
                    start_x = 1;
                }
                
                // Aggiusta per il bordo destro
                if (start_x + width > GAME_WIDTH - 1) {
                    width = GAME_WIDTH - 1 - start_x;
                }
                
                // Disegna solo se c'è qualcosa da disegnare
                if (width > 0) {
                    for (int h = 0; h < crocodile_positions[i].height; h++) {
                        for (int w = 0; w < width; w++) {
                            mvaddch(crocodile_positions[i].y + h, 
                                    start_x + w, 'C');
                        }
                    }
                }
                
            }
        }
        attroff(COLOR_PAIR(2));
        
        // Disegno la barra del tempo e lo score
        draw_time_bar(remaining_time, max_time);
        draw_score(score);
        
        // Stampa informazioni di debug
        mvprintw(0, 2, "Pos: [%d, %d] | Tane: %d/%d", rana_x, rana_y, tane_occupate, NUM_TANE);
        if (strlen(debug_message) > 0) {
            mvprintw(0, 35, "%s", debug_message);
        }
        
        // Stampa rana con colori
        attron(COLOR_PAIR(1)); // Usa il colore definito per la rana
        for (int i = 0; i < rana_height; i++) {
            for (int j = 0; j < rana_width; j++) {
                mvaddch(rana_y + i, rana_x + j, rana_sprite[i][j]);
            }
        }
        attroff(COLOR_PAIR(1));
        
        // Aggiorna lo schermo
        refresh();
        
        // Breve pausa per evitare un consumo eccessivo di CPU
        usleep(10000);
    }
    
    // Messaggio di fine gioco
    clear();
    mvprintw(GAME_HEIGHT/2, GAME_WIDTH/2 - 10, "Game Over!");
    refresh();
    sleep(2);
    
    return NULL;
}
