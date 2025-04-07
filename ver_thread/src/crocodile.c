#include "../include/game.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>


void* crocodile_thread(void* arg) {
    crocodile_args* args = (crocodile_args*)arg;
    game_state* state = args->state;
    int id = args->id;
    
    // Copia posizione originale per il calcolo della velocità
    pthread_mutex_lock(&state->crocodiles[id].mutex);
    int lane = (state->crocodiles[id].id / 2) % LANES;
    int direction = (lane % 2 == 0) ? 1 : -1;
    int original_width = state->crocodiles[id].width;
    pthread_mutex_unlock(&state->crocodiles[id].mutex);
    
    // Seed random number generator uniquely for this thread
    srand(time(NULL) ^ id);
    
    // Calculate speed based on lane - lower values = faster movement
    int speed;
    switch(lane % 4) {
        case 0:
            speed = 250000; // lane 0, 4, 8 - medium speed
            break;
        case 1:
            speed = 300000; // lane 1, 5, 9 - slow speed
            break;
        case 2:
            speed = 180000; // lane 2, 6, 10 - fast speed
            break;
        case 3:
            speed = 220000; // lane 3, 7, 11 - medium-fast speed
            break;
        default:
            speed = 250000; // fallback
    }
    
    // Add some random variation to the speed (±10%)
    speed = speed * (90 + rand() % 21) / 100;
    
    while (!state->game_over) {
        // Check if game is paused
        if (state->game_paused) {
            usleep(100000); // Sleep briefly and check pause flag again
            continue;
        }
        
        // Lock position for update
        pthread_mutex_lock(&state->crocodiles[id].mutex);
        
        // Controlla se il player è su questo coccodrillo per garantire
        // movimento sincronizzato
        bool has_player = (state->player_on_crocodile && state->player_crocodile_id == id);
        
        // Update position
        state->crocodiles[id].x += direction;
        
        // Handle boundary wrapping
        if (direction > 0) {  // Moving right
            if (state->crocodiles[id].x + state->crocodiles[id].width > GAME_WIDTH - 1) {
                // Partially or fully off screen to the right
                int overflow = (state->crocodiles[id].x + state->crocodiles[id].width) - (GAME_WIDTH - 1);
                state->crocodiles[id].width = state->crocodiles[id].width - overflow;
                
                if (state->crocodiles[id].width <= 0) {
                    // Wrap around to left side
                    state->crocodiles[id].x = 1;
                    state->crocodiles[id].width = 1; // Start growing from left side
                    
                    // Se il player è sul coccodrillo che sta sparendo, cade in acqua
                    if (has_player) {
                        pthread_mutex_lock(&state->game_mutex);
                        state->player_on_crocodile = false;
                        state->player_crocodile_id = -1;
                        pthread_mutex_unlock(&state->game_mutex);
                    }
                }
            } else if (state->crocodiles[id].width < original_width) {
                // Keep growing when entering from left
                state->crocodiles[id].width++;
            }
        } else {  // Moving left
            if (state->crocodiles[id].x <= 0) {
                // Partially or fully off screen to the left
                state->crocodiles[id].width = state->crocodiles[id].width - 1;
                state->crocodiles[id].x = 1;
                
                if (state->crocodiles[id].width <= 0) {
                    // Wrap around to right side
                    state->crocodiles[id].x = GAME_WIDTH - 2;
                    state->crocodiles[id].width = 1; // Start growing from right side
                    
                    // Se il player è sul coccodrillo che sta sparendo, cade in acqua
                    if (has_player) {
                        pthread_mutex_lock(&state->game_mutex);
                        state->player_on_crocodile = false;
                        state->player_crocodile_id = -1;
                        pthread_mutex_unlock(&state->game_mutex);
                    }
                }
            } else if (state->crocodiles[id].width < original_width) {
                // Keep growing when entering from right
                state->crocodiles[id].width++;
                state->crocodiles[id].x--;
                
                // Se il player è sul coccodrillo, dobbiamo mantenerlo nella posizione relativa
                if (has_player) {
                    pthread_mutex_lock(&state->player.mutex);
                    state->player.x--;
                    pthread_mutex_unlock(&state->player.mutex);
                }
            }
        }
        
        // Don't shoot if the game is paused
        if (!state->game_paused) {
            // Randomly shoot bullet (3% chance)
            if (rand() % 100 < 3) {
                // Determine bullet position based on direction
                int bullet_x = (direction > 0) ? 
                    state->crocodiles[id].x + state->crocodiles[id].width - 1 : 
                    state->crocodiles[id].x;
                    
                create_bullet(
                    state,
                    bullet_x,               // x position
                    state->crocodiles[id].y, // y position
                    direction,               // same direction as crocodile
                    true                     // enemy bullet
                );
            }
        }
        
        // Modificare la parte in crocodile_thread che aggiorna lo stato
        // Al posto di aggiornare direttamente state->crocodiles[id]
        
        // Dopo aver calcolato la nuova posizione
        game_message msg;
        msg.type = MSG_CROCODILE;
        msg.id = id;
        msg.pos = state->crocodiles[id]; // Copiare i valori aggiornati
        msg.direction = direction;
        
        // Invia il messaggio invece di aggiornare direttamente
        buffer_put(&state->event_buffer, &msg);
        
        // Rimuovere la manipolazione del player qui, sarà gestita dal game_thread
        
        pthread_mutex_unlock(&state->crocodiles[id].mutex);
        
        // Sleep to control movement speed (using variable speed instead of fixed CROCODILE_SPEED)
        usleep(speed);
    }
    
    // Free allocated memory for arguments
    free(arg);
    return NULL;
}