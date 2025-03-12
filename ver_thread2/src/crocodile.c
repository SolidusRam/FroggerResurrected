#include "../include/crocodile.h"
#include "../include/buffer.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

// Mutex per generare ID univoci per i coccodrilli
static int next_croc_id = 0;
static pthread_mutex_t id_mutex = PTHREAD_MUTEX_INITIALIZER;

void *crocodile_thread(void *arg) {
    // Cast dell'argomento alla struttura CrocodileArg
    CrocodileArg *croc_arg = (CrocodileArg *)arg;
    int id = croc_arg->id;
    free(croc_arg);  // Libera memoria dell'argomento
    
    // Inizializza il generatore di numeri casuali con un seed basato sull'id
    srand(time(NULL) ^ id);
    
    int direction = 0;
    int original_width;
    
    // Inizializza la posizione del coccodrillo
    Position pos = {
        .type = 'C',
        .id = id,
        .width = (rand() % 2 + 2) * 5,  // Larghezza tra 10 e 15
        .height = 2,
        .gameOver = false
    };
    
    original_width = pos.width;
    
    // Calcola la corsia e la direzione
    int lane = (id / 2) % LANES;
    direction = (lane % 2 == 0) ? 1 : -1;
    
    // Posizione iniziale basata su ID pari/dispari
    int is_second = id % 2;
    if (is_second) {
        pos.x = (GAME_WIDTH/2) + (rand() % (GAME_WIDTH/4));
    } else {
        pos.x = rand() % (GAME_WIDTH/4);
    }
    pos.y = 4 + (lane * LANE_HEIGHT);
    
    while (!game_over) {
        // Gestione bordo destro
        if (direction > 0) {
            if (pos.x >= GAME_WIDTH - 1) {
                // Reset posizione a sinistra
                pos.x = 1 - original_width;
                pos.width = 0;  // Inizia invisibile
            }
            // Aumenta gradualmente la larghezza quando rientra
            if (pos.x < 1 && pos.width < original_width) {
                pos.width++;
            }
            pos.x++;
        }
        // Gestione bordo sinistro
        else {
            if (pos.x + pos.width <= 1) {
                // Reset posizione a destra
                pos.x = GAME_WIDTH - 1;
                pos.width = 0;
            }
            // Aumenta gradualmente la larghezza quando rientra
            if (pos.x > GAME_WIDTH - original_width && pos.width < original_width) {
                pos.x--;
                pos.width++;
            }
            pos.x--;
        }

        // Mantieni la larghezza entro i limiti dello schermo
        if (pos.x < 1) {
            int overlap = 1 - pos.x;
            pos.x = 1;
            pos.width = pos.width - overlap;
            if (pos.width <= 0) pos.width = 1;
        }
        else if (pos.x + pos.width > GAME_WIDTH - 1) {
            pos.width = GAME_WIDTH - 1 - pos.x;
            if (pos.width <= 0) pos.width = 1;
        }
        
        // Invia posizione aggiornata
        produce(&buffer, pos);
        
        // Attendi prima del prossimo movimento
        usleep(200000);  // 200ms tra ogni movimento
    }
    
    return NULL;
}