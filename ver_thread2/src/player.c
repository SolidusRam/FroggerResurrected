#include "../include/player.h"
#include "../include/buffer.h"
#include <ncurses.h>
#include <unistd.h>

void *player_thread(void *arg) {
    // Variabili per la gestione del tempo
    struct timespec last_move_time;
    clock_gettime(CLOCK_MONOTONIC, &last_move_time);
    const long MIN_MOVE_INTERVAL_NS = 150 * 1000000; // 150ms in nanosecondi
    
    while (!game_over) {
        int ch = getch(); // Legge l'input dell'utente
        if (ch == ERR) {
            usleep(20000); // Breve attesa se non ci sono input
            continue;
        }
        
        // Controlla se è passato abbastanza tempo dall'ultimo movimento
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ns = (current_time.tv_sec - last_move_time.tv_sec) * 1000000000 +
                         (current_time.tv_nsec - last_move_time.tv_nsec);
        
        if (elapsed_ns < MIN_MOVE_INTERVAL_NS && ch != 'q') {
            continue; // Salta questo ciclo se è troppo presto per un altro movimento
        }
        
        int dx = 0, dy = 0;
        bool moved = false;
        
        switch (ch) {
            case KEY_UP:
                dy = -1; moved = true;
                break;
            case KEY_DOWN:
                dy = 1; moved = true;
                break;
            case KEY_LEFT:
                dx = -1; moved = true;
                break;
            case KEY_RIGHT:
                dx = 1; moved = true;
                break;
            case ' ': // Spazio per eventuale azione (es. saltare o sparare)
                // Implementa qui eventuali azioni speciali
                break;
            case 'q': // Termina il gioco
                game_over = true;
                break;
        }
        
        // Se c'è un movimento, invia un messaggio di spostamento relativo
        if (moved) {
            // Crea un messaggio con tipo 'F' (Frog/Rana) con solo spostamento relativo
            Position pos = { 
                .type = 'F',
                .dx = dx,
                .dy = dy,
                .id = 0, // ID della rana
                .gameOver = false,
                .round = 1  // Round corrente
            };
            
            // Invia il messaggio al buffer per il thread di gioco
            produce(&buffer, pos);
            
            // Aggiorna il timestamp dell'ultimo movimento
            clock_gettime(CLOCK_MONOTONIC, &last_move_time);
            
            // Svuota il buffer degli input
            flushinp();
        }
        
        // Breve attesa per evitare un consumo eccessivo di CPU
        usleep(10000);
    }
    
    return NULL;
}